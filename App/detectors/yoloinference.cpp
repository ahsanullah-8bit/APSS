#include "yoloinference.h"

#include <algorithm>
#include <string>
#include <vector>
#include <memory>
#include <thread>
#include <mutex>
#include <assert.h>
#include <filesystem>
#include <format>

#include <opencv2/opencv.hpp>

#include <QDebug>
#include <QFile>

#include "utils.h"

YOLOInference::YOLOInference(const std::string &modelPath, const std::string &labelsPath)
    : m_env(ORT_LOGGING_LEVEL_WARNING, "YOLO_ENV")
    , m_memoryInfo(Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault))
    , m_modelMetadata(nullptr)
{
    if (!std::filesystem::exists(modelPath))
        throw std::runtime_error("Model is not available in the specified directory. Please download a model first and try again!");

    try {
        Ort::SessionOptions sessionOptions;
        int intraop_threads = 4; // std::min(6, static_cast<int>(std::thread::hardware_concurrency()));
        sessionOptions.SetIntraOpNumThreads(intraop_threads);
        sessionOptions.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);

        std::vector<std::string> exec_providers = Ort::GetAvailableProviders();
        qInfo() << "Available providers:" << exec_providers;

#ifdef APSS_USE_CUDA_EP
        if (std::find(exec_providers.begin(), exec_providers.end(), "CUDAExecutionProvider")
            != exec_providers.end()) {
            qInfo() << std::format("Inference device {} selected.", "CUDAExecutionProvider");

            OrtCUDAProviderOptions cudaOption;
            sessionOptions.AppendExecutionProvider_CUDA(cudaOption);
        } else {
            qWarning() << std::format("Inference device {} not available, using fall back {}", "CUDAExecutionProvider", "CPUExecutionProvider");
        }
#endif

#ifdef APSS_USE_OPENVINO_EP
        if (std::find(exec_providers.begin(), exec_providers.end(), "OpenVINOExecutionProvider")
            != exec_providers.end()) {
            qInfo() << std::format("Inference device {} selected.", "OpenVINOExecutionProvider");

            std::unordered_map<std::string, std::string> options;
            options["device_type"] = "AUTO:GPU,CPU";
            options["precision"] = "ACCURACY";
            options["num_of_threads"] = "4";
            sessionOptions.AppendExecutionProvider_OpenVINO_V2(options);
        }
        else {
            qWarning() << std::format("Inference device {} not available, using fall back {}", "OpenVINOExecutionProvider", "CPUExecutionProvider");
        }
#endif

#ifdef USE_CPU_EP
    qInfo() << std::format("Inference device {} selected.", "CPUExecutionProvider");
#endif

#ifdef _WIN32
        std::wstring w_modelPath(modelPath.begin(), modelPath.end());
        m_session = Ort::Session(m_env, w_modelPath.c_str(), sessionOptions);
#else
        m_session = Ort::Session(env, modelPath.c_str(), sessionOptions);
#endif

        m_numInputNodes = m_session.GetInputCount();
        m_numOutputNodes = m_session.GetOutputCount();

        // Input Nodes. Though all of them (YOLOs) have only a one input tensor.
        for (int i = 0; i < m_numInputNodes; ++i) {
            Ort::AllocatedStringPtr name_ptr = m_session.GetInputNameAllocated(i, m_allocator);
            m_inputNodeNameAllocatedStrings.emplace_back(std::move(name_ptr));
            m_inputNames.emplace_back(m_inputNodeNameAllocatedStrings.back().get());
        }

        // Output Nodes
        for (int i = 0; i < m_numOutputNodes; ++i) {
            Ort::AllocatedStringPtr name_ptr = m_session.GetOutputNameAllocated(i, m_allocator);
            m_outputNodeNameAllocatedStrings.emplace_back(std::move(name_ptr));
            m_outputNames.emplace_back(m_outputNodeNameAllocatedStrings.back().get());
        }

        // AFAIK, all the YOLO models have a single input tensor shape. So, we hardcoding.
        Ort::TypeInfo input_type_info = m_session.GetInputTypeInfo(0);
        m_inputTensorShape = input_type_info.GetTensorTypeAndShapeInfo().GetShape();

        if (m_inputTensorShape.size() != 4)
            throw std::runtime_error("Model input is not 4D! Expected [N, C, H, W].");


        // METADATA
        // Read the stride
        m_modelMetadata = m_session.GetModelMetadata();
        std::string model_stride = m_modelMetadata.LookupCustomMetadataMapAllocated("stride", m_allocator).get();
        m_modelStride = std::stoi(model_stride);

        // Names
        std::string model_names = m_modelMetadata.LookupCustomMetadataMapAllocated("names", m_allocator).get();
        // Load class names and generate corresponding colors
        if (!model_names.empty()){
            m_classNames = Utils::jsonToVecClassNames(model_names);
            m_classColors = Utils::generateColors(m_classNames);
            qDebug() << "Classes found on the model's metadata, skip reading from file.";
        } else if (!labelsPath.empty()) {
            m_classNames = Utils::readClassNames(labelsPath);
            m_classColors = Utils::generateColors(m_classNames);
        } else {
            qWarning() << "Class names weren't found, please provide a model with enough metadata or path to a file containing class names.";
        }

        qInfo() << "\n\nSession starting for model " << modelPath
                << "with \n   Stride:" << m_modelStride
                << "\n   Input Nodes:" << m_inputTensorShape
                << "\n   Output Nodes:" << m_session.GetOutputTypeInfo(0).GetTensorTypeAndShapeInfo().GetShape()
                << (m_numOutputNodes == 2 ? m_session.GetOutputTypeInfo(1).GetTensorTypeAndShapeInfo().GetShape() : std::vector<int64_t>())
                << "\n   Names:" << m_classNames << "\n\n";

    } catch (const Ort::Exception& e) {
        throw std::runtime_error(std::format("ONNX Runtime error {}: {}", static_cast<int>(e.GetOrtErrorCode()), e.what()));
    } catch (const std::exception& e) {
        throw std::runtime_error(std::format("Standard exception caught: {}", e.what()));
    }
}

std::vector<prediction_vec> YOLOInference::predict(const mat_vec &images, bool followBigDimensions, float confThreshold, float iouThreshold)
{
    if (images.empty())
        return {};

    std::lock_guard<std::mutex> lock_guard(m_mtx);

    std::vector<int64_t> input_tensor_shape(m_inputTensorShape);    // BCHW

    // Model doesn't have dynamic shape. Ignore user images sizes
    // Model have dynamic shape. Prefer user image sizes
    if (hasDynamicBatch())
        input_tensor_shape[0] = images.size();
    else if (images.size() != input_tensor_shape[0]) {
        qWarning() << "Batch mismatch for input tensor, ignoring the rest!";
    }

    if (hasDynamicShape()) {
        // Use dimensions of the biggest image for resize.
        int64_t img_h = images.at(0).rows, img_w = images.at(0).cols;
        for (size_t i = 1; followBigDimensions && i < images.size(); ++i) {
            if (img_h < images[i].rows)
                img_h = images[i].rows;

            if (img_w < images[i].cols)
                img_w = images[i].cols;
        }

        // Ensuring the stride
        if (img_h % m_modelStride != 0)
            img_h = ((img_h / m_modelStride) + 1) * m_modelStride;
        if (img_w % m_modelStride != 0)
            img_w = ((img_w / m_modelStride) + 1) * m_modelStride;

        input_tensor_shape[2] = img_h;
        input_tensor_shape[3] = img_w;
    }
    cv::Size input_image_shape(input_tensor_shape[3], input_tensor_shape[2]);

    // Pre-Process each image
    std::vector<float> img_data(Utils::vectorProduct(input_tensor_shape));
    mat_vec preprocessed_images;
    preprocessed_images.reserve(images.size());

    for (size_t i = 0; i < input_tensor_shape[0]; ++i) {
        float *offset_ptr = img_data.data() + i * (3 * input_image_shape.area());
        cv::Mat preprocessed_image = preprocess(images[i], offset_ptr, input_image_shape);
        preprocessed_images.emplace_back(preprocessed_image);
    }

    // Check if the data is correctly placed
    if (img_data.size() != Utils::vectorProduct(input_tensor_shape)) {
        qFatal() << "Wrong size of pre-processed images provided, Skipping.";
        return {};
    }

    Ort::Value input_tensor = Ort::Value::CreateTensor<float>(
        m_memoryInfo,
        img_data.data(),
        img_data.size(),
        input_tensor_shape.data(),
        input_tensor_shape.size()
        );

    std::vector<Ort::Value> output_tensors; // Should be
    {
        output_tensors = m_session.Run(
            Ort::RunOptions{nullptr},
            m_inputNames.data(),
            &input_tensor,
            m_numInputNodes,
            m_outputNames.data(),
            m_numOutputNodes
            );
    }

    // cv::Size resizedImageShape(static_cast<int>(inputTensorShape[3]), static_cast<int>(inputTensorShape[2]));
    std::vector<prediction_vec> predictions = postprocess(images, input_image_shape, output_tensors, confThreshold, iouThreshold);

    return predictions; // Return the vector of detections
}

void YOLOInference::drawPredictions(mat_vec &images, const std::vector<prediction_vec> &predictions, float maskAlpha) const
{
    // Both the size of images and set prediction vectors must be the same.
    Q_ASSERT(images.size() == predictions.size());

    for (size_t i = 0; i < images.size(); ++i) {
        drawPredictions(images[i], predictions[i], maskAlpha);
    }
}

void YOLOInference::drawPredictionsMask(mat_vec &images, const std::vector<prediction_vec> &predictionsList, float maskAlpha) const
{
    // Both the size of images and set prediction vectors must be the same.
    Q_ASSERT(images.size() == predictionsList.size());

    for (size_t i = 0; i < images.size(); ++i) {
        drawPredictionsMask(images[i], predictionsList[i], maskAlpha);
    }
}

std::vector<mat_vec> YOLOInference::cropPredictions(const mat_vec &images, const std::vector<prediction_vec> &predictionsList) const
{
    Q_ASSERT(images.size() == predictionsList.size());

    std::vector<mat_vec> results_list;
    results_list.reserve(images.size());

    for (int i = 0; i < images.size(); ++i) {
        mat_vec results = cropPredictions(images.at(i), predictionsList.at(i));
        results_list.emplace_back(results);
    }

    return results_list;
}

std::vector<int64_t> YOLOInference::inputTensorShape() const
{
    return m_inputTensorShape;
}

const Ort::Session &YOLOInference::session() const
{
    return m_session;
}

size_t YOLOInference::numInputNodes() const
{
    return m_numInputNodes;
}

size_t YOLOInference::numOutputNodes() const
{
    return m_numOutputNodes;
}

const std::vector<std::string> &YOLOInference::classNames() const
{
    return m_classNames;
}

const std::vector<cv::Scalar> &YOLOInference::classColors() const
{
    return m_classColors;
}

cv::Mat YOLOInference::preprocess(const cv::Mat &image, float *&imgData, cv::Size inputImageShape)
{
    cv::Mat resizedImage;
    // Resize and pad the image using letterBox utility
    Utils::letterBox(image, resizedImage, inputImageShape);

    // Convert image to float and normalize to [0, 1]
    resizedImage.convertTo(resizedImage, CV_32FC3, 1 / 255.0f);

    // Split the image into separate channels and store in the blob
    mat_vec chw(resizedImage.channels());
    for (int i = 0; i < resizedImage.channels(); ++i) {
        chw[i] = cv::Mat(resizedImage.rows, resizedImage.cols, CV_32FC1, imgData + i * resizedImage.cols * resizedImage.rows);
    }
    cv::split(resizedImage, chw); // Split channels into the blob

    return resizedImage;
}

Ort::MemoryInfo &YOLOInference::memoryInfo() const
{
    return m_memoryInfo;
}

bool YOLOInference::hasDynamicBatch() const
{
    return !m_inputTensorShape.empty() && m_inputTensorShape.at(0) == -1;
}

bool YOLOInference::hasDynamicShape() const
{
    return !m_inputTensorShape.empty()
    && (m_inputTensorShape.at(2) == -1
       || m_inputTensorShape.at(3) == -1);
}

Ort::AllocatorWithDefaultOptions &YOLOInference::allocator() const
{
    return m_allocator;
}

const Ort::ModelMetadata &YOLOInference::modelMetadata() const
{
    return m_modelMetadata;
}
