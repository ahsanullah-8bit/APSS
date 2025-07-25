#pragma once

#include <QString>
#include <QHash>

#include <tbb_patched.h>
#include <opencv2/core/mat.hpp>

class FrameManager
{
public:
    static FrameManager &instance();
    void setMaxFramesPerCamera(const QString &camera, int maxFrames = 5);
    // id as camera_frameIndx
    void write(const QString &frameId, cv::Mat img);
    // id as camera_frameIndx
    std::optional<cv::Mat> get(const QString &frameId);
    // id as camera_frameIndx
    bool retire(const QString &frameId);

private:
    using InternalFrameStore_t = tbb::concurrent_hash_map<std::string, std::vector<cv::Mat>>;
    explicit FrameManager();

private:
    InternalFrameStore_t m_frameStore;
    QHash<QString, int> m_maxFramesPerCamera;
};
