#pragma once

#include <QThread>

#include "camerametrics.h"
#include "config/cameraconfig.h"
#include "config/modelconfig.h"
#include "detectors/paddleocr.h"
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
    bool trackAndEstimateDeltas(SharedFrame frame,
                                Tracker &tracker,
                                Prediction::Type predictionType);
    PredictionList filterResults(const PredictionList &results, const std::map<std::string, FilterConfig> &objectsToFilter);
    void recognizeLicensePlates(SharedFrame frame);

private:
    QString m_cameraName;
    CameraConfig m_config;
    PaddleOCREngine m_ocrEngine;
    std::optional<ModelConfig> m_modelConfig;
    std::optional<std::map<int, std::string>> m_labelmap;
    SharedFrameBoundedQueue &m_inDetectorFrameQueue;
    SharedFrameBoundedQueue &m_inLPDetectorFrameQueue;
    QSharedPointer<QWaitCondition> m_waitCondition;
    SharedCameraMetrics m_cameraMetrics;
};
