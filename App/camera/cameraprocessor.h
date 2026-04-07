#pragma once
#include <unordered_map>

#include <QThread>
#include <QDateTime>

#include "camerametrics.h"
#include "config/cameraconfig.h"
#include "config/modelconfig.h"
#include "detectors/paddleocr.h"
#include "track/tracker.h"
#include "utils/frame.h"
#include "utils/prediction.h"

// This class handles object detection and tracking on frames coming from a single camera
// feed. The detection is done through several stages:
//      * Object Detection: Pull a frame from the camerametrics->frameQueue, push it to the unified detectors
//          queue, wait for results.
//      * Object Tracking: 2 types of tracking, one is tracking objects through out the frames
//          and another is tracking vehicles' how-far-are-you from the camera, to shade unnecessary
//          inference of license plate detection.
//
// Notice: It is worth noting that the tracking of objects/events is happening in two major stages of this system,
//      the CameraProcessor (separate thread, per camera) and TrackedObjectProcessor (single thread, unified).
//      The first is necessary to shade some resource usage and the second is the main tracking system. Combining 
//      them both through some common queue/messaging may reduce the RAM usage but is not worth it, I guess.
class CameraProcessor : public QThread
{
    Q_OBJECT
public:
    const int TRACK_MAX_LOST_WAIT = 5;              // Makes cache wait for seconds until lost object is removed.
    const int TRACK_MIN_AREA = 15'625;              // (125 x 125) minimum area to consider for tracking
    const float TRACK_MAX_ASPECT_RATIO = 2.5f;      // max w/h for valid view
    const float TRACK_APPROACH_THRESHOLD = 1.1f;    // 10% area increase
    const float TRACK_DEPART_THRESHOLD = 0.8f;      // 20% area decrease

    struct TrackedObject {
        int last_seen_frame;
        QTime last_seen_at;
        int last_triggered_area = -1;  // -1 = never triggered
        int max_observed_area = 0;     // The biggest we've seen so far
    };

    explicit CameraProcessor(const QString &cameraName,
                             const CameraConfig &config,
                             SharedFrameBoundedQueue &inDetectorFrameQueue,
                             SharedFrameBoundedQueue &inLPDetectorFrameQueue,
                             QSharedPointer<QWaitCondition> waitCondition,
                             SharedFrameBoundedQueue &trackedFrameQueue,
                             SharedCameraMetrics cameraMetrics,
                             QObject *parent = nullptr);

    // QThread interface
protected:
    void run() override;
    bool predict(SharedFrame frame, SharedFrameBoundedQueue &queue);
    void estimateChangesInArea(PredictionList &predictions, std::unordered_map<int, TrackedObject> &objectsHistory);
    PredictionList filterObjectPredictions(const PredictionList &results,
                                           const std::map<std::string, FilterConfig> &objectsToFilter);

private:
    QString m_cameraName;
    CameraConfig m_config;
    PaddleOCREngine m_ocrEngine;
    SharedFrameBoundedQueue &m_inDetectorFrameQueue;
    SharedFrameBoundedQueue &m_inLPDetectorFrameQueue;
    QSharedPointer<QWaitCondition> m_waitCondition;
    SharedFrameBoundedQueue &m_trackedFrameQueue;
    SharedCameraMetrics m_cameraMetrics;
};
