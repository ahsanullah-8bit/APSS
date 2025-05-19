#pragma once

#include <QObject>
#include <QThread>

#include <tbb_patched.h>

#include <detectors/yolodetection.h>
#include <utils/frame.h>

#include "exports.h"
#include "yolopose.h"

class ObjectDetectorSession : public QThread
{
    Q_OBJECT
public:
    explicit ObjectDetectorSession(tbb::concurrent_bounded_queue<Frame> &frameQueue,
                                   tbb::concurrent_bounded_queue<Frame> &processedFrameQueue,
                                   const QString& modelPath,
                                   int maxBatchSize = 1,
                                   float confThreshold = 0.4f,
                                   float iouThreshold = 0.45f,
                                   bool drawPredictions = true,
                                   const std::string &labelsPath = std::string(),
                                   QObject *parent = nullptr);
    YOLODetection &detector();

signals:
    void frameChanged(const cv::Mat &mat);

protected:
    // QThread interface
    void run() override;

private:
    YOLODetection m_detector;
    tbb::concurrent_bounded_queue<Frame> &m_frameQueue;
    tbb::concurrent_bounded_queue<Frame> &m_processedFrameQueue;
    int m_maxBatchSize = 1;
    float m_confThreshold = 0.4f;
    float m_iouThreshold = 0.45f;
    bool m_drawPredictions = true;
};
