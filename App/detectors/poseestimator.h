#pragma once

#include "config/predictorconfig.h"
#include "predictor.h"
#include "apss.h"

class PoseEstimator : public Predictor
{
public:
    explicit PoseEstimator(const PredictorConfig& config,
                           const std::shared_ptr<Ort::Env> &env = nullptr,
                           const std::shared_ptr<CustomAllocator> &allocator = nullptr,
                           const std::shared_ptr<Ort::MemoryInfo> &memoryInfo = nullptr);

    // Predictor interface
    void draw(cv::Mat &image, const PredictionList &predictions, float maskAlpha) const override;
    void setPoseSkeleton(const std::vector<std::pair<int, int>> &poseSkeleton);

protected:
    std::vector<PredictionList> postprocess(const MatList &originalImages, const cv::Size &resizedImageShape, const std::vector<Ort::Value> &outputTensors, float confThreshold, float iouThreshold) override;

private:
    std::vector<std::pair<int, int>> m_skeleton;
    std::vector<cv::Scalar> m_classColors;
    std::vector<int> m_kptShape;
};
