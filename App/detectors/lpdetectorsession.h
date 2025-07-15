#pragma once

#include <QThread>

#include <tbb_patched.h>

#include "utils/eventspersecond.h"
#include "utils/frame.h"
#include "config/predictorconfig.h"
#include "config/licenseplateconfig.h"
#include "detectors/poseestimator.h"

class LPDetectorSession : public QThread
{
    Q_OBJECT
public:
    explicit LPDetectorSession(SharedFrameBoundedQueue &inFrameQueue,
                        QHash<QString, QSharedPointer<QWaitCondition>> &cameraWaitConditions,
                        const PredictorConfig &config,
                        const LicensePlateConfig &lpConfig,
                        QObject *parent = nullptr);
    PoseEstimator& detector();
    const EventsPerSecond &eps() const;

protected:
    // QThread interface
    void run() override;

private:
    PredictorConfig m_config;
    LicensePlateConfig m_lpConfig;

    PoseEstimator m_keyPointDetector;
    SharedFrameBoundedQueue &m_inFrameQueue;
    QHash<QString, QSharedPointer<QWaitCondition>> &m_cameraWaitConditions;
    EventsPerSecond m_eps;
};
