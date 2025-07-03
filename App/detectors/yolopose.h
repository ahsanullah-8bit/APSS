#pragma once

#include "yoloinference.h"

// Hate to break it, but this class is bounded to License Plates
// specifically (drawPredictions()). If you with to use it otherwise, inherit it and override drawPredictions().
class YOLOPose : public YOLOInference
{
public:
    explicit YOLOPose(const std::string &modelPath, const std::string &labelsPath = std::string());

    // YOLOInference interface
    void drawPredictions(cv::Mat &image, const PredictionList &predictions, float maskAlpha = MODEL_MASK_ALPHA) const override;
    void drawPredictionsMask(cv::Mat &image, const PredictionList &predictions, float maskAlpha = MODEL_MASK_ALPHA) const override;
    MatList cropPredictions(const cv::Mat &image, const PredictionList &predictions) const override;
    MatList cropPredictions(const cv::Mat &image, const PredictionList &predictions, bool perspectively = true) const;

    void drawPredictions(MatList &images, const std::vector<PredictionList> &predictions, float maskAlpha = MODEL_MASK_ALPHA) const;
    void drawPredictionsMask(MatList &images, const std::vector<PredictionList> &predictionsList, float maskAlpha = MODEL_MASK_ALPHA) const;
    std::vector<MatList> cropPredictions(const MatList &images, const std::vector<PredictionList> &predictionsList) const;

protected:
    std::vector<PredictionList> postprocess(const MatList &originalImages,
                                            const cv::Size &resizedImageShape,
                                            const std::vector<Ort::Value> &outputTensors,
                                            float confThreshold, float iouThreshold) override;
};
