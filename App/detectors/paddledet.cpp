// Disclaimer: This implementation is heavily inspired by PaddleOCR cpp_infer example's implementation

#include <opencv2/core.hpp>
#include <yaml-cpp/yaml.h>

#include "paddledet.h"

#include "image.h"

PaddleDet::PaddleDet(const PredictorConfig &config,
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

    if (model_yml.exists()) {
        try {
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
                    if (op["DecodeImage"]) {
                        const auto &img_mode = op["DecodeImage"]["img_mode"];
                        if (img_mode)
                            m_imgMode = img_mode.as<std::string>();
                    } else if (op["DetResizeForTest"]) {
                        const auto &resize_long = op["DetResizeForTest"]["resize_long"];
                        if (resize_long)
                            m_resizeLong = resize_long.as<int>();
                    } else if (op["NormalizeImage"]) {
                        YAML::Node sub_op = op["NormalizeImage"];
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
                const auto &threshold = yaml["PostProcess"]["thresh"];
                if (threshold)
                    m_threshold = threshold.as<double>();

                const auto &box_threshold = yaml["PostProcess"]["box_thresh"];
                if (box_threshold)
                    m_boxThresh = box_threshold.as<double>();

                const auto &unclip_ratio = yaml["PostProcess"]["unclip_ratio"];
                if (unclip_ratio)
                    m_unclipRatio = unclip_ratio.as<double>();

            } else {
                // Read earlier version
            }
        } catch(const YAML::Exception &e) {
            qFatal() << "Failed to parse" << model_yml.path() << "file:" << e.what();
        } catch(const std::exception &e) {
            qFatal() << "Failed to parse" << model_yml.path() << "file:" << e.what();
        }
    } else {
        qWarning() << "Model associated" << model_yml.path() << "file doesn't exist. This may cause problems as default values associated with PaddleOCR v4 and v5 will be used";
    }
}

std::vector<Vector3d<int>> PaddleDet::predict(const MatList &batch)
{
    if (batch.empty())
        return {};

    const auto input_tensor_shapes = m_inferSession.inputTensorShapes();
    Q_ASSERT(!input_tensor_shapes.empty());
    std::vector<int64_t> input_tensor_shape(input_tensor_shapes[0]);    // BCHW

    if (hasDynamicBatch())
        input_tensor_shape[0] = batch.size();
    else if (static_cast<int64_t>(batch.size()) != input_tensor_shape[0]) {
        qWarning() << "Batch mismatch for input tensor, ignoring the rest!";
        input_tensor_shape[0] = 1;  // Unnecessary. But anyway...
    }

    // Pre-Process each image    
    float ratio_h{};
    float ratio_w{};
    MatList preprocessed_images;

    for (int64_t i = 0; i < input_tensor_shape[0]; ++i) {
        cv::Mat preprocessed_image;

        m_resizeOp.Run(batch[i], preprocessed_image, m_limitType, m_resizeLong, ratio_h, ratio_w, false);
        m_normalizeOp.Run(preprocessed_image, m_mean, m_std, m_scale);

        preprocessed_images.emplace_back(preprocessed_image);
    }

    int uni_h = preprocessed_images[0].rows;
    int uni_w = preprocessed_images[0].cols;
    input_tensor_shape[2] = uni_h;
    input_tensor_shape[3] = uni_w;
    std::vector<float> img_data(Utils::vectorProduct(input_tensor_shape));

    for (int64_t i = 0; i < input_tensor_shape[0]; ++i) {
        float *offset_ptr = img_data.data() + i * (3 * uni_h * uni_w);
        m_permuteOp.Run(preprocessed_images[i], offset_ptr);
    }

    // Inference
    std::vector<Ort::Value> output_tensors = m_inferSession.predictRaw(img_data, input_tensor_shape);
    if (output_tensors.empty())
        return {};

    // Post-Process Results
    const Ort::Value &tensor0 = output_tensors[0];
    const std::vector<int64_t> shape0 = tensor0.GetTensorTypeAndShapeInfo().GetShape();
    const float* output0_data = tensor0.GetTensorData<float>(); // Extract raw output data from the first output tensor

    Q_ASSERT(shape0.size() == 4); // expect [N, 1, H, W] ['DynamicDimension.3', 1, 'DynamicDimension.4', 'DynamicDimension.5']

    const size_t out_batch_size = shape0.at(0);
    const size_t out_channels = shape0.at(1); // 1 (expected)
    const size_t output_h = shape0.at(2);
    const size_t output_w = shape0.at(3);

    Q_ASSERT(out_channels == 1);
    // out_batch_size can't be trusted, as the session might cache the previous batch
    // so it shouldn't be used for postprocessing
    Q_ASSERT(out_batch_size >= batch.size());

    std::vector<Vector3d<int>> boxes_results_list;

    // Process each output of the batch
    for (size_t b = 0; b < batch.size(); ++b) {
        const float *output0_data_offset = output0_data + b * (output_h * output_w);

        size_t output_area = output_h * output_w;

        // NOTICE: This is how PaddleOCR's example handles it
        std::vector<float> prob_data(output_area, 0.0f);        // Probability data
        std::vector<unsigned char> cbuff(output_area, ' '); // Binary/Bitmap data

        for (int i = 0; i < output_area; ++i) {
            prob_data[i] = static_cast<float>(output0_data_offset[i]);
            cbuff[i] = static_cast<unsigned char>((output0_data_offset[i]) * 255);
        }

        cv::Mat cbuff_map(output_h, output_w, CV_8UC1, static_cast<unsigned char *>(cbuff.data()));
        cv::Mat prob_map(output_h, output_w, CV_32F, static_cast<float *>(prob_data.data()));

        const double threshold = m_threshold * 255;
        const double maxValue = 255;

        cv::Mat bit_map;
        cv::threshold(cbuff_map, bit_map, threshold, maxValue, cv::THRESH_BINARY);
        if (m_useDilation) {
            cv::Mat dila_ele = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(2, 2));
            cv::dilate(bit_map, bit_map, dila_ele);
        }

        Vector3d<int> boxes = std::move(m_postProcessor.BoxesFromBitmap(
            prob_map, bit_map, m_boxThresh, m_unclipRatio,
            m_detDbScoreMode));

        m_postProcessor.FilterTagDetRes(boxes, ratio_h, ratio_w, batch[b]);
        // end NOTICE

        boxes_results_list.emplace_back(std::move(boxes));
    }

    return boxes_results_list;
}

const ONNXInference &PaddleDet::inferSession() const
{
    return m_inferSession;
}

bool PaddleDet::hasDynamicBatch()
{
    const auto &input_tshapes = m_inferSession.inputTensorShapes();
    return !input_tshapes.empty()
           && !input_tshapes.at(0).empty()
           &&  input_tshapes.at(0).at(0) == -1;
}
