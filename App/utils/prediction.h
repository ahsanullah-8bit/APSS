#pragma once

#include <string>
#include <vector>

#include <opencv2/core/core.hpp>
#include <rfl.hpp>

/**
 * @brief Struct representing a Prediction of all sorts.
 *
 * @note Only relevent members should be set/get for the type of task at hand.
 */
struct Prediction {
    cv::Rect box;
    // cv::Mat mask;  // Single-channel (8UC1) mask in full resolution
    std::vector<cv::Point3f> points; // x, y, z (conf)
    std::string className;
    float conf = 0;
    int classId = -1;
    long long trackerId = -1;
    bool hasDeltas = false;
};

using PredictionList = std::vector<Prediction>;

namespace rfl {

template<>
struct Reflector<cv::Rect> {
    struct ReflType {
        int x, y, height, width;
    };

    static cv::Rect to(const ReflType &v) {
        return { v.x, v.y, v.width, v.height };
    }

    static ReflType from(const cv::Rect &v) {
        return { v.x, v.y, v.width, v.height };
    }
};

template<>
struct Reflector<cv::Point3f> {
    struct ReflType {
        float x, y, z;
    };

    static cv::Point3f to(const ReflType &v) {
        return { v.x, v.y, v.z };
    }

    static ReflType from(const cv::Point3f &v) {
        return { v.x, v.y, v.z };
    }
};

}
