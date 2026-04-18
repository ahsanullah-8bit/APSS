#pragma once

#include <QThread>

#include <tbb_patched.h>
#include <config/predictorconfig.h>
#include <config/licenseplateconfig.h>
#include <utils/eventspersecond.h>
#include <utils/frame.h>
#include <detectors/poseestimator.h>

class LPDetectorSession : public QThread
{
    Q_OBJECT
public:
    explicit LPDetectorSession(SharedFrameBoundedQueue &inFrameQueue,
                        QHash<QString, QSharedPointer<QWaitCondition>> &cameraWaitConditions,
                        const PredictorConfig &config,
                        const LicensePlateConfig &lpConfig,
                        std::shared_ptr<Ort::Env> env = nullptr,
                        QObject *parent = nullptr);
    const EventsPerSecond &eps() const;
    void stop();

protected:
    // QThread interface
    void run() override;
    PredictionList filterLicensePlates(const PredictionList &predictions);

private:
    std::shared_ptr<Ort::Env> m_env;

    PredictorConfig m_config;
    LicensePlateConfig m_lpConfig;

    QSharedPointer<PoseEstimator> m_keyPointDetector;
    SharedFrameBoundedQueue &m_inFrameQueue;
    QHash<QString, QSharedPointer<QWaitCondition>> &m_cameraWaitConditions;
    EventsPerSecond m_eps;
};
