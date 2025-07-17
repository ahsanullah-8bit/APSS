#pragma once

#include <QObject>
#include <QThread>

#include <tbb_patched.h>

#include <config/detectorconfig.h>
#include "config/predictorconfig.h"
#include "detectors/objectdetector.h"
#include "utils/eventspersecond.h"
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
    const EventsPerSecond &eps() const;
    // This method will run in the thread it is called from.
    void stop();

protected:
    // QThread interface
    void run() override;

private:

    // New interface
    QString m_name;
    QSharedPointer<ObjectDetector> m_detector;
    QHash<QString, QSharedPointer<QWaitCondition>> &m_cameraWaitConditions;
    SharedFrameBoundedQueue &m_inFrameQueue;
    // SharedFrameBoundedQueue &m_outFrameQueue;
    std::atomic_int m_avgInferenceSpeed;
    PredictorConfig m_config;
    EventsPerSecond m_eps;
};

using SharedObjectDetectorSession = QSharedPointer<ObjectDetectorSession>;
