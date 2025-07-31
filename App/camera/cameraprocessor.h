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
                             // const std::optional<ModelConfig> &modelConfig,
                             // const std::optional<std::map<int, std::string>> &labelmap,
                             SharedFrameBoundedQueue &inDetectorFrameQueue,
                             SharedFrameBoundedQueue &inLPDetectorFrameQueue,
                             QSharedPointer<QWaitCondition> waitCondition,
                             SharedFrameBoundedQueue &trackedFrameQueue,
                             SharedCameraMetrics cameraMetrics,
                             QObject *parent = nullptr);

    // QThread interface
protected:
    void run() override;
    bool predict(SharedFrame frame,
                 SharedFrameBoundedQueue &queue);
    // returns true, if there were any deltas
    void trackAndEstimateDeltas(SharedFrame frame,
                                Tracker &tracker);
    void trackAndEstimateDeltas2(SharedFrame frame,
                                Tracker &tracker);
    PredictionList filterObjectPredictions(const PredictionList &results,
                                           const std::map<std::string, FilterConfig> &objectsToFilter);
    void recognizeLicensePlates(SharedFrame frame, std::vector<std::string> lp_classes);

private:
    QString m_cameraName;
    CameraConfig m_config;
    PaddleOCREngine m_ocrEngine;
    // std::optional<ModelConfig> m_modelConfig;
    // std::optional<std::map<int, std::string>> m_labelmap;
    SharedFrameBoundedQueue &m_inDetectorFrameQueue;
    SharedFrameBoundedQueue &m_inLPDetectorFrameQueue;
    QSharedPointer<QWaitCondition> m_waitCondition;
    SharedFrameBoundedQueue &m_trackedFrameQueue;
    SharedCameraMetrics m_cameraMetrics;

    // Counts lpr retries for an object with a tracker id
    // if the license plate has been confirmed a number of times,
    // don't detect license plate for that vehicle
    QHash<int, std::pair<QString, int>> m_lprRetries; // tracker_id, <prev_lpr_result, count>
};
