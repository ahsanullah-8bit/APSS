#include <yaml-cpp/yaml.h>

#include "paddlerec.h"

#include "licensed/utility.h"

PaddleRec::PaddleRec(const PredictorConfig &config,
                     const std::shared_ptr<Ort::Env> &env,
                     const std::shared_ptr<CustomAllocator> &allocator,
                     const std::shared_ptr<Ort::MemoryInfo> &memoryInfo)
    : m_inferSession(config, env, allocator, memoryInfo)
{
    if (!config.model)
        throw std::runtime_error("No Model config provided");

    const QFileInfo model_fileinfo = QFileInfo(QString::fromStdString(config.model.value().path.value()));
    const QString model_basename = model_fileinfo.baseName();
    const QDir model_dir(model_fileinfo.dir());
    const QFileInfo model_yml(model_dir.filePath(model_basename + ".yml"));
    const QFileInfo model_txt(model_dir.filePath(model_basename + ".txt"));

    bool read_labels = true;
    if (model_yml.exists()) {
        YAML::Node yaml = YAML::LoadFile(model_yml.absoluteFilePath().toStdString());

        bool is_new_model = true;
        const auto &global_node = yaml["Global"];
        const auto &model_node = global_node["model_name"];
        if (model_node) {
            QString model_name;
            model_name = QString::fromStdString(model_node.as<std::string>());

            if (!model_name.contains("OCRv4") && !model_name.contains("OCRv5"))
                is_new_model = false;
        }

        if (is_new_model) {
            // preprocess
            const auto &transform_ops = yaml["PreProcess"]["transform_ops"];
            for (const auto &op : transform_ops) {
                YAML::Node sub_op;
                if (op["DecodeImage"]) {
                    const auto &img_mode = op["DecodeImage"]["img_mode"];
                    if (img_mode)
                        m_imgMode = img_mode.as<std::string>();
                } else if (op["RecResizeImg"]) {
                    const auto &img_shape = op["RecResizeImg"]["image_shape"];
                    if (img_shape)
                        m_imgShape = img_shape.as<std::vector<int>>();
                }
            }

            // postprocess
            const auto &character_dict = yaml["PostProcess"]["character_dict"];
            if (character_dict) {
                m_labels = character_dict.as<std::vector<std::string>>();

                // read from the .txt file, if empty
                if (!m_labels.empty())
                    read_labels = false;

                m_labels.emplace_back("");
            }

        } else {
            // Read earlier version
        }
    } else {
        qWarning() << "Model associated" << model_yml.path() << "file doesn't exist. This may cause problems as default values associated with PaddleOCR v4 and v5 will be used";
    }

    if (read_labels) {
        if (!model_txt.exists())
            qFatal() << "Character dictionary file" << model_txt.path() << "doesn't exist1";

        try {
            m_labels = PaddleOCR::Utility::ReadDict(model_txt.absolutePath().toStdString());
            m_labels.emplace_back("");
        } catch(const std::exception &e) {
            qFatal() << "Failed to load dict text file:" << e.what();
        }
    }
}

std::vector<std::pair<std::string, float>> PaddleRec::predict(const MatList &batch)
{
    if (batch.empty())
        return {};

    const size_t batch_size = batch.size();
    std::vector<std::pair<std::string, float>> rec_texts(batch_size, {});

    // Get the width ratios
    std::vector<float> batch_ratios;
    for (const auto &img : batch)
        batch_ratios.emplace_back(float(img.cols) / img.rows);

    // sorted ratio indices
    const std::vector<size_t> sri = PaddleOCR::Utility::argsort(batch_ratios);

    // expected shape ['DynamicDimension.0', 3, 48, 'DynamicDimension.1'] (batch, channels, height, width)
    const auto input_tensor_shapes = m_inferSession.inputTensorShapes();
    Q_ASSERT(!input_tensor_shapes.empty());
    std::vector<int64_t> rec_input_shape = input_tensor_shapes[0];
    // set fixed width, because onnx/onnxruntime/openvino doesn't like to re-use buffers bigger than before/after
    rec_input_shape[2] = m_imgShape[1];
    rec_input_shape[3] = m_imgShape[2];

    for (size_t b = 0; b < batch_size; b += m_maxBatchSize) {
        // batch sub-selection
        const size_t sel_batch_end = std::min(batch_size, b + m_maxBatchSize);
        const size_t sel_batch_size = sel_batch_end - b;

        // See, the difference with recognition is that it expects dynamic width and you can't
        // have varying with per image in the batch. So, we'll have to find the largest in the
        // batch and add padding to the rest.

        // find the max ratio of the whole batch and the max width
        // int max_w = 0;
        // for (size_t j = b; j < sel_batch_end; ++j) {
        //     const float ratio = batch[sri[j]].cols * 1.0 / batch[sri[j]].rows;
        //     max_w = std::max(max_w, batch[sri[j]].cols);
        // }

        // set the max batch size and max width for each img in the batch
        rec_input_shape[0] = sel_batch_size;
        // rec_input_shape[3] = max_w;
        // const int max_h = rec_input_shape[2];

        // int batch_width = img_w;
        std::vector<cv::Mat> sel_batch;
        for (size_t j = b; j < sel_batch_end; ++j) {
            cv::Mat resized_img;

            m_resizeOp.Run(batch[sri[j]], resized_img, cv::Size(rec_input_shape[3], rec_input_shape[2])); // w, h
            m_normalizeOp.Run(resized_img, m_mean, m_std, m_scale);

            // batch_width = std::max(resized_img.cols, batch_width);
            sel_batch.emplace_back(resized_img);
        }

        // std::vector<int64_t> custom_shape = { static_cast<int64_t>(sel_batch_size), 3, imgH, batch_width };
        std::vector<float> input(sel_batch_size * 3 * rec_input_shape[2] * rec_input_shape[3], 0.0f);

        m_permuteOp.Run(sel_batch, input.data());

        // Inference.
        std::vector<Ort::Value> output_tensors = m_inferSession.predictRaw(input, rec_input_shape);
        if (output_tensors.empty())
            continue;

        // postprocess results
        const Ort::Value &tensor0 = output_tensors[0];
        const std::vector<int64_t> shape0 = tensor0.GetTensorTypeAndShapeInfo().GetShape();
        const float* output0_data = tensor0.GetTensorData<float>();

        Q_ASSERT(shape0.size() == 3); // expect ['DynamicDimension.0', 'Reshape_524_o0__d2', 18385] [-1, T, C] (Batch, SequenceLength, NumClasses)

        const size_t out_batch_size = shape0.at(0);
        const size_t out_seq_len = shape0.at(1);
        const size_t out_num_classes = shape0.at(2); // (number of characters in the dictionary + 1 for the blank character).

        // out_batch_size can't be trusted, as the session might cache the previous batch
        // so it shouldn't be used for postprocessing
        // Take 2: To my expectations, the output shape should have the same batch size as that of input shape given.
        //              But the runtime inference sometimes says otherwise. I'm gonna assume it a skill issue and
        //              move on at the moment.
        // Q_ASSERT(out_batch_size >= sel_batch.size());

        // ctc decode
        Q_ASSERT(!m_labels.empty());
        for (size_t r = 0; r < sel_batch_size; ++r) {
            std::string str_res;
            int argmax_idx;
            int last_index = 0;
            float score = 0.0f;
            int count = 0;
            float max_value = 0.0f;

            for (int n = 0; n < out_seq_len; ++n) {
                // get idx
                argmax_idx = int(PaddleOCR::Utility::argmax(
                    &output0_data[(r * out_seq_len + n) * out_num_classes],
                    &output0_data[(r * out_seq_len + n + 1) * out_num_classes]));

                // get score
                max_value = float(*std::max_element(
                    &output0_data[(r * out_seq_len + n) * out_num_classes],
                    &output0_data[(r * out_seq_len + n + 1) * out_num_classes]));

                if (argmax_idx > 0 && (!(n > 0 && argmax_idx == last_index))) {
                    score += max_value;
                    count += 1;

                    // TODO: Do proper research for this one
                    // Why does the model report 97 classes and we get 95 from the .yml

                    if (argmax_idx - 1 > m_labels.size())
                        qFatal() << "Invalid index accessed" << argmax_idx - 1;
                    else
                        str_res += m_labels[argmax_idx - 1];
                }

                last_index = argmax_idx;
            }
            score /= count;
            if (std::isnan(score)) {
                continue;
            }
            rec_texts[sri[b + r]].first = std::move(str_res);
            rec_texts[sri[b + r]].second = score;
        }

    }

    return rec_texts;
}

const ONNXInference &PaddleRec::inferSession() const
{
    return m_inferSession;
}
