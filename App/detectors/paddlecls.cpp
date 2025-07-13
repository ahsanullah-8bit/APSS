#include <yaml-cpp/yaml.h>

#include "paddlecls.h"

#include "licensed/utility.h"

PaddleCls::PaddleCls(const PredictorConfig &config,
                     const std::shared_ptr<Ort::Env> &env,
                     const std::shared_ptr<Ort::AllocatorWithDefaultOptions> &allocator,
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
        // TODO: Add a try-cathc block
        YAML::Node yaml = YAML::LoadFile(model_yml.absoluteFilePath().toStdString());

        bool is_new_model = true;
        const auto &global_node = yaml["Global"];
        const auto &model_node = global_node["model_name"];
        if (model_node) {
            QString model_name;
            model_name = QString::fromStdString(model_node.as<std::string>());

            // Do a proper check for cls model
            // if (!model_name.contains("OCRv4") && !model_name.contains("OCRv5"))
            //     is_new_model = false;
        }

        if (is_new_model) {
            // preprocess
            const auto &transform_ops = yaml["PreProcess"]["transform_ops"];

            for(const auto &op : transform_ops) {
                if (op["ResizeImage"]) {
                    const auto &resize_img_size = op["ResizeImage"]["size"];
                    if (resize_img_size)
                        m_resizeImgSize = resize_img_size.as<std::vector<int>>();
                } else if (op["NormalizeImage"]) {
                    YAML::Node sub_op = op["NormalizeImage"];
                    const auto &channel_num = sub_op["channel_num"];
                    if (channel_num)
                        m_channelNum = channel_num.as<int>();

                    const auto &mean = sub_op["mean"];
                    if (mean)
                        m_mean = mean.as<std::vector<float>>();

                    const auto &order = sub_op["order"];
                    if (order)
                        m_normImgOrder = order.as<std::string>();

                    const auto &scale = sub_op["scale"];
                    if (scale)
                        m_scale = scale.as<double>();

                    const auto &std = sub_op["std"];
                    if (std)
                        m_std = std.as<std::vector<float>>();
                }
            }

            // postprocess
            const auto &label_list = yaml["PostProcess"]["Topk"]["label_list"];
            if (label_list) {
                m_labels = label_list.as<std::vector<std::string>>();

                // read from the .txt file, if empty
                if (!m_labels.empty())
                    read_labels = false;
            }

        } else {
            // Read earlier version
        }
    } else {
        qWarning() << "Model associated" << model_yml.path() << "file doesn't exist. This may cause problems as default values associated with PaddleOCR x0.25 and x1.0 will be used";
    }

    if (read_labels) {
        if (!model_txt.exists()) {
            qWarning() << "Label file" << model_txt.path() << "doesn't exist, expecting exactly 2 classes";
        } else {
            try {
                m_labels = PaddleOCR::Utility::ReadDict(model_txt.absolutePath().toStdString());
            } catch(const std::exception &e) {
                qFatal() << "Failed to load dict text file:" << e.what();
            }
        }
    }
}

std::vector<std::pair<int, float>> PaddleCls::predict(const MatList &batch)
{
    if (batch.empty())
        return {};

    const int batch_size = batch.size();
    std::vector<std::pair<int, float>> cls_results(batch_size, {});

    // (expected) ['DynamicDimension.0', 3, 80, 160] (batch, channels, height, width) as reported by the model itself.
    const auto input_tensor_shapes = m_inferSession.inputTensorShapes();
    Q_ASSERT(!input_tensor_shapes.empty());
    std::vector<int64_t> cls_input_shape = input_tensor_shapes[0];
    const int64_t in_channels = cls_input_shape[1];
    const int64_t in_height = cls_input_shape[2];
    const int64_t in_width = cls_input_shape[3];

    for (int b = 0; b < batch_size; b += m_maxBatchSize) {
        // Batch sub-selection
        const int sel_batch_end = std::min(batch_size, b + m_maxBatchSize);
        const int sel_batch_size = sel_batch_end - b;

        // preprocess suv-batch
        std::vector<cv::Mat> sel_batch;
        for (int j = b; j < sel_batch_end; ++j) {
            cv::Mat resized_img;

            m_resizeOp.Run(batch[j], resized_img, cv::Size(in_width, in_height));
            m_normalizeOp.Run(resized_img, m_mean, m_std, m_scale);

            const int pad_hor = in_width - resized_img.cols;
            const int pad_ver = in_height - resized_img.rows;
            if (pad_hor > 0 || pad_ver > 0) {
                cv::copyMakeBorder(resized_img, resized_img,
                                   0, pad_ver, 0, pad_hor,
                                   cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0));
            }

            sel_batch.emplace_back(resized_img);
        }
        // Update the input tensor shape
        cls_input_shape[0] = sel_batch_size;

        const int input_size = sel_batch_size * in_channels * in_height * in_width;
        std::vector<float> input(input_size, 0.0f);

        m_permuteOp.Run(sel_batch, input.data());

        // inference on sub-batch
        const std::vector<Ort::Value> output_tensors = m_inferSession.predictRaw(input, cls_input_shape);
        if (output_tensors.empty())
            continue;

        // postprocess sub-batch results
        const Ort::Value &tensor0 = output_tensors[0];
        const std::vector<int64_t> shape0 = tensor0.GetTensorTypeAndShapeInfo().GetShape();
        const float* output0_data = tensor0.GetTensorData<float>(); // Extract raw output data from the first output tensor

        Q_ASSERT(shape0.size() == 2); // expect ['Reshape_233_o0__d0', 2]

        const size_t out_batch_size = shape0.at(0);
        // const size_t out_num_classes = shape0.at(1); // 2 (expected)

        Q_ASSERT(out_batch_size >= sel_batch_size);
        // Q_ASSERT(out_num_classes == 2); // expect [0, 180]

        for (size_t k = 0; k < sel_batch_size; ++k) {
            const int label = static_cast<int>(PaddleOCR::Utility::argmax(&output0_data[k * shape0[1]],
                                                                          &output0_data[(k + 1) * shape0[1]]));

            const float score = float(*std::max_element(&output0_data[k * shape0[1]], &output0_data[(k + 1) * shape0[1]]));
            cls_results[b + k].first = label;
            cls_results[b + k].second = score;
        }
    }

    return cls_results;
}

bool PaddleCls::readInCharDict(const std::string &filepath) noexcept
{
    bool is_yaml = filepath.ends_with(".yml") || filepath.ends_with(".yaml");
    if (!is_yaml && !filepath.ends_with(".txt"))
        return false;

    try {
        if (!is_yaml) {
            m_labels = PaddleOCR::Utility::ReadDict(filepath);
        } else {
            YAML::Node yaml = YAML::LoadFile(filepath);

            // check the model name
            auto model_name_node = yaml["Global"]["model_name"];
            if (model_name_node.IsDefined()) {
                const QString model_name = QString::fromStdString(model_name_node.as<std::string>());

                if (model_name.contains("OCRv5", Qt::CaseInsensitive)
                    && model_name.contains("OCRv4", Qt::CaseInsensitive)) {
                    qFatal() << "Unsupported yaml file provided for model" << model_name << ", model name should have either OCRv4 or OCRv5";
                }
            }

            // fetch the RecResizeImg
            auto cls_shape_node = yaml["PreProcess"]["transform_ops"][0]["size"];
            if (cls_shape_node.IsDefined())
                m_clsImageShape = cls_shape_node.as<std::vector<int>>();

            // fetch labels_list
            auto label_list_node = yaml["PostProcess"]["Topk"]["label_list"];
            if (label_list_node)
                m_labels = label_list_node.as<std::vector<std::string>>();
        }
    } catch(const std::exception &e) {
        qFatal() << "Failed to load dict text file:" << e.what();
    }

    return true;
}

double PaddleCls::clsThreshold() const
{
    return m_clsThreshold;
}

const ONNXInference &PaddleCls::inferSession() const
{
    return m_inferSession;
}
