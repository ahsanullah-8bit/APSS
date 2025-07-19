#include <vector>
#include <memory>
#include <mutex>
#include <assert.h>

#include <yaml-cpp/yaml.h>

#include "predictor.h"
#include "image.h"

Predictor::Predictor(const PredictorConfig &config,
                     const std::shared_ptr<Ort::Env> &env,
                     const std::shared_ptr<CustomAllocator> &allocator,
                     const std::shared_ptr<Ort::MemoryInfo> &memoryInfo)
    : m_inferSession(config, env, allocator, memoryInfo)
{
    const auto &model_metadata = m_inferSession.modelMetadata();
    Ort::AllocatedStringPtr imgsz = model_metadata.LookupCustomMetadataMapAllocated("imgsz", m_inferSession.allocator());
    bool is_valid_imgsz = false;
    if (imgsz) {
        YAML::Node imgsz_yaml = YAML::Load(imgsz.get());
        if (imgsz_yaml) {
            std::vector<int> wh = imgsz_yaml.as<std::vector<int>>();
            if (!wh.empty()) {
                m_width = wh[0];
                m_height = wh[1];
                is_valid_imgsz = true;
            }
        }
    }

    if (!is_valid_imgsz) {
        if (config.model) {
            m_width = config.model->width.value_or(320);
            m_height = config.model->height.value_or(320);
        } else {
            qWarning() << "No imgsz found for model" << config.model->path << "assuming" << m_width << m_height << "instead.";
        }
    }
}

Predictor::~Predictor()
{}

std::vector<PredictionList> Predictor::predict(const MatList &images)
{
    if (images.empty())
        return {};

    std::lock_guard<std::mutex> lock_guard(m_mtx);

    const auto &input_tensor_shapes = m_inferSession.inputTensorShapes();
    Q_ASSERT(!input_tensor_shapes.empty());
    std::vector<int64_t> input_tensor_shape(input_tensor_shapes[0]);    // BCHW

    // Model doesn't have dynamic shape. Ignore user images sizes
    // Model have dynamic shape. Prefer user image sizes
    if (hasDynamicBatch())
        input_tensor_shape[0] = images.size();
    else if (static_cast<int64_t>(images.size()) != input_tensor_shape[0]) {
        qWarning() << "Batch mismatch for input tensor, ignoring the rest!" << input_tensor_shape[0] << " != " << images.size();
    }

    if (hasDynamicShape()) {
        // // Use dimensions of the biggest image for resize.
        // int64_t img_h = images.at(0).rows, img_w = images.at(0).cols;
        // for (size_t i = 1; i < images.size(); ++i) {
        //     if (img_h < images[i].rows)
        //         img_h = images[i].rows;

        //     if (img_w < images[i].cols)
        //         img_w = images[i].cols;
        // }

        // Ensuring the stride
        int model_stride = m_inferSession.modelStride();
        if (m_height % model_stride != 0)
            m_height = ((m_height / model_stride) + 1) * model_stride;
        if (m_width % model_stride != 0)
            m_width = ((m_width / model_stride) + 1) * model_stride;

        input_tensor_shape[2] = m_height;
        input_tensor_shape[3] = m_width;
    }
    cv::Size input_image_shape(input_tensor_shape[3], input_tensor_shape[2]);

    // Pre-Process each image
    std::vector<float> img_data(Utils::vectorProduct(input_tensor_shape));
    MatList preprocessed_images;
    preprocessed_images.reserve(images.size());

    for (int64_t i = 0; i < input_tensor_shape[0]; ++i) {
        float *offset_ptr = img_data.data() + i * (3 * input_image_shape.area());
        cv::Mat preprocessed_image = preprocess(images[i], offset_ptr, input_image_shape);
        preprocessed_images.emplace_back(preprocessed_image);
    }

    std::vector<Ort::Value> output_tensors = m_inferSession.predictRaw(img_data, input_tensor_shape);
    std::vector<PredictionList> predictions = postprocess(images, input_image_shape, output_tensors);

    return predictions; // Return the vector of detections
}

void Predictor::draw(MatList &images, const std::vector<PredictionList> &predictionsList, float maskAlpha) const
{
    Q_ASSERT(images.size() == predictionsList.size());

    for (size_t i = 0; i < images.size(); ++i) {
        draw(images[i], predictionsList[i], maskAlpha);
    }
}

bool Predictor::hasDynamicBatch() const
{
    const auto &input_tshapes = m_inferSession.inputTensorShapes();
    return !input_tshapes.empty()
           && !input_tshapes.at(0).empty()
           &&  input_tshapes.at(0).at(0) == -1;
}

bool Predictor::hasDynamicShape() const
{
    const auto &input_tshapes = m_inferSession.inputTensorShapes();
    return !input_tshapes.empty()
           && !input_tshapes.at(0).empty()
           && (input_tshapes.at(0).at(2) == -1 ||
               input_tshapes.at(0).at(3) == -1);
}

const ONNXInference &Predictor::inferSession() const
{
    return m_inferSession;
}

cv::Mat Predictor::preprocess(const cv::Mat &image, float *&imgData, cv::Size inputImageShape)
{
    cv::Mat resizedImage;
    // Resize and pad the image using letterBox utility
    Utils::letterBox(image, resizedImage, inputImageShape);

    // Convert image to float and normalize to [0, 1]
    resizedImage.convertTo(resizedImage, CV_32FC3, 1 / 255.0f);

    // Split the image into separate channels and store in the blob
    MatList chw(resizedImage.channels());
    for (int i = 0; i < resizedImage.channels(); ++i) {
        chw[i] = cv::Mat(resizedImage.rows, resizedImage.cols, CV_32FC1, imgData + i * resizedImage.cols * resizedImage.rows);
    }
    cv::split(resizedImage, chw); // Split channels into the blob

    return resizedImage;
}

int Predictor::height() const
{
    return m_height;
}

int Predictor::width() const
{
    return m_width;
}
