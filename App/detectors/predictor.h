#pragma once

#include <onnxruntime_cxx_api.h>

#include <opencv2/core/mat.hpp>

#include "apss.h"
#include "utils/prediction.h"
#include "config/predictorconfig.h"
#include "onnxinference.h"

class Predictor
{
public:
    // explicit Predictor(const PredictorConfig &config);
    explicit Predictor(const PredictorConfig &config,
                       const std::shared_ptr<Ort::Env> &env,
                       const std::shared_ptr<CustomAllocator> &allocator,
                       const std::shared_ptr<Ort::MemoryInfo> &memoryInfo);
    virtual ~Predictor();
    virtual std::vector<PredictionList> predict(const MatList &images);
    virtual void draw(MatList &images, const std::vector<PredictionList> &predictionsList, float maskAlpha = 0.3f) const;
    virtual void draw(cv::Mat &image, const PredictionList &predictions, float maskAlpha = 0.3f) const = 0;
    int width() const;
    int height() const;
    bool hasDynamicBatch() const;
    bool hasDynamicShape() const;
    const ONNXInference &inferSession() const;

protected:
    virtual cv::Mat preprocess(const cv::Mat &image, float *&imgData, cv::Size inputImageShape);
    virtual std::vector<PredictionList> postprocess(const MatList &originalImages,
                                                    const cv::Size &resizedImageShape,
                                                    const std::vector<Ort::Value> &outputTensors,
                                                    float confThreshold = 0.4f, float iouThreshold = 0.4f) = 0;

private:
    ONNXInference m_inferSession;
    int m_width = 640;
    int m_height = 640;

    mutable std::mutex m_mtx;
};

using SharedPredictor = QSharedPointer<Predictor>;
