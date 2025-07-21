#pragma once

#include <QString>

#include <tbb_patched.h>
#include <opencv2/core/mat.hpp>

class FrameManager
{
public:
    static FrameManager &instance(int maxFramesPerCamera = 5);
    // id as camera_frameIndx
    void write(const QString &frameId, cv::Mat img);
    // id as camera_frameIndx
    std::optional<cv::Mat> get(const QString &frameId);
    // id as camera_frameIndx
    bool retire(const QString &frameId);

private:
    using InternalFrameStore_t = tbb::concurrent_hash_map<std::string, tbb::concurrent_vector<cv::Mat>>;
    explicit FrameManager(int maxFramesPerCamera);

private:
    InternalFrameStore_t m_frameStore;
    int m_maxFramesPerCamera;
};
