#pragma once

#include "yoloinference.h"

class YOLODetection : public YOLOInference
{
public:
    explicit YOLODetection(const std::string &modelPath, const std::string &labelsPath = std::string());

    // YOLOInference interface
    void drawPredictions(cv::Mat &image, const PredictionList &predictions, float maskAlpha = MODEL_MASK_ALPHA) const override;
    void drawPredictionsMask(cv::Mat &image, const PredictionList &predictions, float maskAlpha = MODEL_MASK_ALPHA) const override;
    MatList cropPredictions(const cv::Mat &image, const PredictionList &predictions) const override;

    // The pain, C++ hides all of them (inherited overloads) if one is overloaded in the derived class.
    // These just calls the inherited member functions.
    void drawPredictions(MatList &images, const std::vector<PredictionList> &predictionsList, float maskAlpha = MODEL_MASK_ALPHA) const;
    void drawPredictionsMask(MatList &images, const std::vector<PredictionList> &predictionsList, float maskAlpha = MODEL_MASK_ALPHA) const;
    std::vector<MatList> cropPredictions(const MatList &images, const std::vector<PredictionList> &predictionsList) const;

protected:
    std::vector<PredictionList> postprocess(const MatList &originalImages, const cv::Size &resizedImageShape, const std::vector<Ort::Value> &outputTensors, float confThreshold, float iouThreshold) override;
};
