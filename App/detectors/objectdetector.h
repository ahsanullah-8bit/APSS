#pragma once

#include "predictor.h"
#include "config/predictorconfig.h"

class ObjectDetector : public Predictor
{
public:
    explicit ObjectDetector(const PredictorConfig &config,
                            const std::shared_ptr<Ort::Env> &env = nullptr,
                            const std::shared_ptr<CustomAllocator> &allocator = nullptr,
                            const std::shared_ptr<Ort::MemoryInfo> &memoryInfo = nullptr);

    // Predictor interface
    void draw(cv::Mat &image, const PredictionList &predictions, float maskAlpha) const override;

protected:
    std::vector<PredictionList> postprocess(const MatList &originalImages, const cv::Size &resizedImageShape, const std::vector<Ort::Value> &outputTensors, float confThreshold, float iouThreshold) override;

private:
    std::vector<cv::Scalar> m_classColors;
};
