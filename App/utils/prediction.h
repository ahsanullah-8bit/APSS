#pragma once

#include <string>
#include <vector>

#include <opencv2/core/core.hpp>

/**
 * @brief Struct representing a detected keypoint in pose estimation.
 *
 * Inherited from cv::Point, so it has all that.
 * Plus, a confidence score, indicating the model's certainty in the prediction.
 */
// class KeyPoint : public cv::Point2f {
// public:
//     float conf;

//     KeyPoint() : cv::Point2f(), conf(0.0f) {}
//     KeyPoint(float x_, float y_, float conf_ = 0.0f) : cv::Point2f(x_, y_), conf(conf_) {}
//     KeyPoint(const cv::Point2f& pt, float conf_ = 0.0f) : cv::Point2f(pt), conf(conf_) {}
//     KeyPoint(const cv::Size_<float>& sz, float conf_ = 0.0f) : cv::Point2f(sz), conf(conf_) {}
//     KeyPoint(const cv::Vec<float, 2>& v, float conf_ = 0.0f) : cv::Point2f(v), conf(conf_) {}
//     KeyPoint(const KeyPoint& pt) = default;
//     KeyPoint(KeyPoint&& pt) = default;

//     KeyPoint& operator=(const KeyPoint& pt) = default;
//     KeyPoint& operator=(const cv::Point2f& pt) {
//         cv::Point2f::operator=(pt);
//         conf = 0.0f;
//         return *this;
//     }
//     KeyPoint& operator=(KeyPoint&& pt) = default;
//     KeyPoint& operator=(cv::Point2f&& pt) {
//         cv::Point2f::operator=(std::move(pt));
//         conf = 0.0f;
//         return *this;
//     }

//     void set(float x_, float y_) {
//         x = x_;
//         y = y_;
//     }
//     void set(cv::Point2f point) {
//         x = point.x;
//         y = point.y;
//     }
//     void set(float conf_) {
//         conf = conf_;
//     }

//     static std::vector<cv::Point2f> toPoints(const std::vector<KeyPoint> &points) {
//         return std::vector<cv::Point2f>(points.begin(), points.end());
//     }
// };

/**
 * @brief Struct representing a Prediction of all sorts.
 *
 * @note Only relevent members should be set/get for the type of task at hand.
 */
struct Prediction {
    cv::Rect box;
    cv::Mat mask;  // Single-channel (8UC1) mask in full resolution
    std::vector<cv::Point3f> points; // x, y, z (conf)
    std::string className;
    float conf;
    int classId = -1;
    size_t trackerId = -1;
    bool hasDeltas = false;
};

using PredictionList = std::vector<Prediction>;

