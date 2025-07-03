#pragma once

#include <QThread>

#include <filesystem>
#include <tbb_patched.h>

#include "frame.h"
#include "yolopose.h"

class LPDetectorSession : public QThread
{
    Q_OBJECT
public:
    explicit LPDetectorSession(SharedFrameBoundedQueue &objDetectedFrameQueue,
                        SharedFrameBoundedQueue &lpDetectedFrameQueueOut,
                        const std::set<std::string> &filterClasses,
                        const std::string &modelPath = "models/yolo11n-pose-1700.onnx",
                        int maxBatchSize = 1,
                        float confThreshold = 0.4f,
                        float iouThreshold = 0.45f,
                        bool drawPredictions = true,
                        const std::string &labelsPath = std::string(),
                        QObject *parent = nullptr);

    YOLOPose& detector();
    bool emitProcessedFrames() const;
    void setEmitProcessedFrames(bool yes);

Q_SIGNALS:
    void frameChanged(const Frame& frame);

    // QThread interface
protected:
    void run() override;

private:
    YOLOPose m_keyPointDetector;
    SharedFrameBoundedQueue &m_objDetectedFrameQueue;
    SharedFrameBoundedQueue &m_lpDetectedFrameQueue;
    std::atomic_bool m_emitProcessedFrames;

    std::set<std::string> m_filterClasses;
    int m_maxBatchSize = 1;
    float m_confThreshold = 0.4f;
    float m_iouThreshold = 0.45f;
    bool m_drawPredictions = true;
};
