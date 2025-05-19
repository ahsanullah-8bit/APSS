#pragma once

#include "yoloinference.h"

class YOLOSegmentation : public YOLOInference
{
public:
    explicit YOLOSegmentation(const std::string &modelPath, const std::string &labelsPath = std::string());
    void drawPredictions(cv::Mat &image, const prediction_vec &predictions, float maskAlpha = MODEL_MASK_ALPHA) const override;
    void drawPredictionsMask(cv::Mat &image, const prediction_vec &predictions, float maskAlpha = MODEL_MASK_ALPHA) const override;
    mat_vec cropPredictions(const cv::Mat &image, const prediction_vec &predictions) const override;

    // The pain, C++ hides all of them (inherited overloads) if one is overloaded in the derived class.
    // These just calls the inherited member functions.
    void drawPredictions(mat_vec &images, const std::vector<prediction_vec> &predictionsList, float maskAlpha = MODEL_MASK_ALPHA) const;
    void drawPredictionsMask(mat_vec &images, const std::vector<prediction_vec> &predictionsList, float maskAlpha = MODEL_MASK_ALPHA) const;
    std::vector<mat_vec> cropPredictions(const mat_vec &images, const std::vector<prediction_vec> &predictionsList) const;

    // YOLOInference interface
protected:
    std::vector<prediction_vec> postprocess(const mat_vec &originalImages,
                                                     const cv::Size &resizedImageShape,
                                                     const std::vector<Ort::Value> &outputTensors,
                                                     float confThreshold,
                                                     float iouThreshold) override;

};
