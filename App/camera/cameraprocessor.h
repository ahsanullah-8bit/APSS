#pragma once

#include <QThread>

#include "cameraconfig.h"
#include "config/modelconfig.h"
#include "camerametrics.h"
#include "track/tracker.h"
#include "utils/frame.h"

// The one responsible for all the processing, in Frigate.
class CameraProcessor : public QThread
{
    Q_OBJECT
public:
    explicit CameraProcessor(const QString &cameraName,
                             const CameraConfig &config,
                             const std::optional<ModelConfig> &modelConfig,
                             const std::optional<std::map<int, std::string>> &labelmap,
                             SharedFrameBoundedQueue &inDetectorFrameQueue,
                             SharedFrameBoundedQueue &inLPDetectorFrameQueue,
                             QSharedPointer<QWaitCondition> waitCondition,
                             SharedCameraMetrics cameraMetrics,
                             QObject *parent = nullptr);

signals:
    void frameChanged(SharedFrame frame);

    // QThread interface
protected:
    void run() override;
    bool predict(SharedFrame frame,
                 SharedFrameBoundedQueue &queue);
    // returns true, if there were any deltas
    bool trackAndEstimateDeltas(SharedFrame frame, Tracker &tracker, Prediction::Type predictionType);

private:
    QString m_cameraName;
    CameraConfig m_config;
    std::optional<ModelConfig> m_modelConfig;
    std::optional<std::map<int, std::string>> m_labelmap;
    SharedFrameBoundedQueue &m_inDetectorFrameQueue;
    SharedFrameBoundedQueue &m_inLPDetectorFrameQueue;
    QSharedPointer<QWaitCondition> m_waitCondition;
    SharedCameraMetrics m_cameraMetrics;
};

/*

        if (!m_cameraMetrics->isPullBased()) {
            // push or wait if the queue is full
            m_inDetectorFrameQueue.push(frame);
        } else {
            // put the frame in the detect, using try_push, wait for 5 ms, if the push was successful
            bool frame_pushed = m_inDetectorFrameQueue.try_push(frame);
            if (!frame_pushed)
                continue;
        }

        static const int time_out = m_config.image_detect_timeout
                                        ? m_config.image_detect_timeout.value()
                                        : 20;

        m_mtx.lock();
        if (!m_cameraMetrics->isPullBased()) {
            // wait forever
            m_waitCondition->wait(&m_mtx);
        } else if (!m_waitCondition->wait(&m_mtx, time_out)) {
            // waited for (config timeout or) 20ms and failed, expire the frame.
            frame->setHasExpired(true);
            qWarning() << std::format("Frame {} expired after {}ms. System seems to be overloaded.", frame->id().toStdString(), time_out);
            m_mtx.unlock();
            continue;
        }
        m_mtx.unlock();

        // LP detection
        if (!m_cameraMetrics->isPullBased()) {
            m_inLPDetectorFrameQueue.push(frame);
        } else {
            bool frame_pushed = m_inLPDetectorFrameQueue.try_push(frame);
            if (!frame_pushed)
                continue;
        }

        m_mtx.lock();
        if (!m_cameraMetrics->isPullBased()) {
            m_waitCondition->wait(&m_mtx);
        } else if (!m_waitCondition->wait(&m_mtx, time_out)) {
            frame->setHasExpired(true);
            qWarning() << std::format("Frame {} expired after {}ms. System seems to be overloaded.", frame->id().toStdString(), time_out);
            m_mtx.unlock();
            continue;
        }
        m_mtx.unlock();

        if (!frame->hasExpired())
            detectors_eps.update();
            m_cameraMetrics->setDetectionFPS(detectors_eps.eps());


*/
