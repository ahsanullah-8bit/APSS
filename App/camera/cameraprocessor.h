#pragma once

#include <QThread>

#include "cameraconfig.h"
#include "camerametrics.h"
#include "utils/frame.h"
#include "config/modelconfig.h"

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
                             QSharedPointer<QWaitCondition> waitCondition,
                             SharedCameraMetrics cameraMetrics,
                             QObject *parent = nullptr);

signals:
    void frameChanged(SharedFrame frame);

    // QThread interface
protected:
    void run() override;

private:
    void trackCamera();
    void processFrames();

private:
    QString m_cameraName;
    CameraConfig m_config;
    std::optional<ModelConfig> m_modelConfig;
    std::optional<std::map<int, std::string>> m_labelmap;
    // SharedFrameBoundedQueue &m_inFrameQueue;
    SharedFrameBoundedQueue &m_inDetectorFrameQueue;
    QSharedPointer<QWaitCondition> m_waitCondition;
    QMutex m_mtx;
    // SharedFrameBoundedQueue &m_outFrameQueue;
    SharedCameraMetrics m_cameraMetrics;
};

// CameraCapture -> CameraProcessor -> Detectors
// Metrics.framequeue -> detectorInputQueue
