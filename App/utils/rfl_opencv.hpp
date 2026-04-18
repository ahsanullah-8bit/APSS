#pragma once

#include <rfl.hpp>
#include <opencv2/core/types.hpp>

// Some reflectcpp type for OpenCV type.

namespace rfl {

// cv::Rect
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

// cv::Point2f
template<>
struct Reflector<cv::Point2f> {
    struct ReflType {
        float x, y, z;
    };

    static cv::Point2f to(const ReflType &v) {
        return { v.x, v.y };
    }

    static ReflType from(const cv::Point2f &v) {
        return { v.x, v.y };
    }
};

// cv::Point3f
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
