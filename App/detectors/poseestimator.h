#pragma once

#include <apss.h>
#include <config/predictorconfig.h>
#include <detectors/predictor.h>

class PoseEstimator : public Predictor
{
public:
    explicit PoseEstimator(const PredictorConfig& config,
                           std::unique_ptr<ONNXInference> infer);

    // Predictor interface
    void draw(cv::Mat &image, const PredictionList &predictions, float maskAlpha) const override;
    void setPoseSkeleton(const std::vector<std::pair<int, int>> &poseSkeleton);

protected:
    std::vector<PredictionList> postprocess(const MatList &originalImages,
                                            const cv::Size &resizedImageShape,
                                            const std::vector<Ort::Value> &outputTensors,
                                            float confThreshold = POSE_MIN_CONF,
                                            float iouThreshold = POSE_MIN_IOU_THRESH) override;

private:
    std::vector<std::pair<int, int>> m_skeleton;
    std::vector<cv::Scalar> m_classColors;
    std::vector<int> m_kptShape = {17, 3}; // keypoints, dims
};
