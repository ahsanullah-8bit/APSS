#pragma once

#include <vector>
#include <opencv2/core/mat.hpp>
#include <detectors/prediction.h>

// Some common things APSS depends on

// Includes
#include "exports.h"

// Typedefs
using prediction_vec = std::vector<Prediction>;
using mat_vec = std::vector<cv::Mat>;
using PredictionList = std::vector<Prediction>;
using MatList = std::vector<cv::Mat>;


// Detection related
constexpr float MODEL_OBJECTS_CONFIDENDCE_THRESHOLD = 0.7f;
constexpr float MODEL_LP_CONFIDENDCE_THRESHOLD = 0.4f;
constexpr float MODEL_IOU_THRESHOLD = 0.4f;
constexpr float MODEL_MASK_ALPHA = 0.4f;
constexpr float MODEL_CROP_GAIN = 0.4f;
constexpr float DET_RECONSIDER_AREA_INCREASE = 0.30f;   // Percentage, Reconsider sending a seen object to go through the pipeline again, if area is increase by the %.
constexpr int   TRACKER_DELTA_OBJECT_LIMIT = 40 * 24;   // 40 secs * 24 FPS, 960 ids at the moment

