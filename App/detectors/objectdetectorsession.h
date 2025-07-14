#pragma once

#include <QObject>
#include <QThread>

#include <tbb_patched.h>

#include <config/detectorconfig.h>
#include "config/predictorconfig.h"
#include "detectors/objectdetector.h"
#include <utils/frame.h>

class ObjectDetectorSession : public QThread
{
    Q_OBJECT
public:
    explicit ObjectDetectorSession(const QString &name,
                                   SharedFrameBoundedQueue &inFrameQueue,
                                   QHash<QString, QSharedPointer<QWaitCondition>> &cameraWaitConditions,
                                   const PredictorConfig &config,
                                   QObject *parent = nullptr);
    QSharedPointer<ObjectDetector> detector();
    // This method will run in the thread it is called from.
    void stop();

protected:
    // QThread interface
    void run() override;

private:
    // int m_maxBatchSize = 1;
    // float m_confThreshold = 0.4f;
    // float m_iouThreshold = 0.45f;
    // bool m_drawPredictions = true;

    // New interface
    QString m_name;
    QSharedPointer<ObjectDetector> m_detector;
    QHash<QString, QSharedPointer<QWaitCondition>> &m_cameraWaitConditions;
    SharedFrameBoundedQueue &m_inFrameQueue;
    // SharedFrameBoundedQueue &m_outFrameQueue;
    std::atomic_int m_avgInferenceSpeed;
    PredictorConfig m_config;
};

using SharedObjectDetectorSession = QSharedPointer<ObjectDetectorSession>;
