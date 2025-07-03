#pragma once

#include <onnxruntime_cxx_api.h>

#include "config/predictorconfig.h"
#include <apss.h>

class Predictor
{
public:
    explicit Predictor(const PredictorConfig &config);
    virtual ~Predictor();
    virtual std::vector<PredictionList> predict(const MatList &images);
    virtual void draw(MatList &images, const std::vector<PredictionList> &predictionsList, float maskAlpha = MODEL_MASK_ALPHA) const;
    virtual void draw(cv::Mat &image, const PredictionList &predictions, float maskAlpha = MODEL_MASK_ALPHA) const = 0;
    void printModelMetadata() const;

    const PredictorConfig& config() const;
    std::vector<int64_t> inputTensorShape() const;
    const Ort::Session& session() const;
    size_t numInputNodes() const;
    size_t numOutputNodes() const;
    const std::vector<std::string>& classNames() const;
    const std::vector<cv::Scalar>& classColors() const;
    const Ort::ModelMetadata &modelMetadata() const;
    bool hasDynamicBatch() const;
    bool hasDynamicShape() const;

protected:
    virtual cv::Mat preprocess(const cv::Mat &image, float *&imgData, cv::Size inputImageShape);
    virtual std::vector<PredictionList> postprocess(const MatList &originalImages,
                                                    const cv::Size &resizedImageShape,
                                                    const std::vector<Ort::Value> &outputTensors,
                                                    float confThreshold, float iouThreshold) = 0;

    Ort::MemoryInfo& memoryInfo() const;
    Ort::AllocatorWithDefaultOptions& allocator() const;

private:
    PredictorConfig m_config;
    Ort::Env m_env;
    Ort::Session m_session{nullptr};
    mutable Ort::AllocatorWithDefaultOptions m_allocator;
    mutable Ort::MemoryInfo m_memoryInfo;
    std::vector<int64_t> m_inputTensorShape;
    size_t m_numInputNodes = 1, m_numOutputNodes = 1;
    bool m_hasDynamicBatch = false;
    bool m_hasDynamicShape = false;
    int m_modelStride = 32;
    Ort::ModelMetadata m_modelMetadata;

    // Vectors to hold allocated input and output node names
    std::vector<Ort::AllocatedStringPtr> m_inputNodeNameAllocatedStrings;
    std::vector<Ort::AllocatedStringPtr> m_outputNodeNameAllocatedStrings;
    std::vector<const char *> m_inputNames;
    std::vector<const char *> m_outputNames;

    std::vector<std::string> m_classNames;            // Vector of class names loaded from file
    std::vector<cv::Scalar> m_classColors;            // Vector of colors for each class

    mutable std::mutex m_mtx;
};

using SharedPredictor = QSharedPointer<Predictor>;
