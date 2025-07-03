#pragma once

#include "config/predictorconfig.h"
#include "predictor.h"
#include "apss.h"

class PoseEstimator : public Predictor
{
public:
    explicit PoseEstimator(const PredictorConfig& config);

    // Predictor interface
    void draw(cv::Mat &image, const PredictionList &predictions, float maskAlpha) const override;

protected:
    std::vector<PredictionList> postprocess(const MatList &originalImages, const cv::Size &resizedImageShape, const std::vector<Ort::Value> &outputTensors, float confThreshold, float iouThreshold) override;
};
