#include <ranges>
#include <unordered_map>

#include <opencv2/imgcodecs.hpp>

#include <QDateTime>
#include <QLoggingCategory>

#include "cameraprocessor.h"
#include "config/objectconfig.h"
#include "detectors/image.h"
#include "utils/eventspersecond.h"
#include "track/tracker.h"
#include "utils/prediction.h"

Q_STATIC_LOGGING_CATEGORY(apss_camera_processor, "apss.camera.processor")

CameraProcessor::CameraProcessor(const QString &cameraName,
                                 const CameraConfig &config,
                                 SharedFrameBoundedQueue &inDetectorFrameQueue,
                                 SharedFrameBoundedQueue &inLPDetectorFrameQueue,
                                 QSharedPointer<QWaitCondition> waitCondition,
                                 SharedFrameBoundedQueue &trackedFrameQueue,
                                 SharedCameraMetrics cameraMetrics,
                                 QObject *parent)
    : QThread(parent)
    , m_cameraName(cameraName)
    , m_config(config)
    , m_inDetectorFrameQueue(inDetectorFrameQueue)
    , m_inLPDetectorFrameQueue(inLPDetectorFrameQueue)
    , m_waitCondition(waitCondition)
    , m_trackedFrameQueue(trackedFrameQueue)
    , m_cameraMetrics(cameraMetrics)
{
    setObjectName(QString("apss.thread:%1").arg(m_cameraName));
}

void CameraProcessor::run()
{
    QSharedPointer<SharedFrameBoundedQueue> frame_queue = m_cameraMetrics->frameQueue();
    const ObjectConfig &objects_config = m_config.objects ? m_config.objects.value() : ObjectConfig();
    Tracker tracker(objects_config.track);
    std::unordered_map<int, TrackedObject> objectsHistory;

    EventsPerSecond process_eps;
    process_eps.start();

    EventsPerSecond detectors_eps;
    detectors_eps.start();

    while(!isInterruptionRequested()) {
        SharedFrame frame;
        frame_queue->pop(frame);
        if(!frame)
            continue;

        if (!predict(frame, m_inDetectorFrameQueue))
            continue;

        // Track and Filter predictions
        PredictionList predictions = frame->predictions();
        if (objects_config.filters)
            predictions = filterObjectPredictions(predictions, objects_config.filters.value());

        tracker.track(predictions);
        estimateChangesInArea(predictions, objectsHistory);
        frame->setPredictions(predictions);

        // Detect license plate
        if (!predict(frame, m_inLPDetectorFrameQueue))
            continue;

        detectors_eps.update();
        m_cameraMetrics->setDetectionFPS(detectors_eps.eps());

        process_eps.update();
        m_cameraMetrics->setProcessFPS(process_eps.eps());

        // Send the frame to listensers.
        m_trackedFrameQueue.try_emplace(frame);
    }

    // TODO: Empty the frame Queue
}

bool CameraProcessor::predict(SharedFrame frame,
                              SharedFrameBoundedQueue &queue)
{
    static const bool is_pull_based = m_cameraMetrics->isPullBased();
    static QMutex mtx;

    if (is_pull_based) {
        static const int frame_timeout = m_config.pull_based_timeout
                                             ? m_config.pull_based_timeout.value()
                                             : 20;

        if (!queue.try_emplace(frame))
            return false;

        if (!frame->hasBeenProcessed()) {
            QMutexLocker<QMutex> lock(&mtx);
            if (!m_waitCondition->wait(&mtx, frame_timeout)) {
                frame->setHasExpired(true);
                qCWarning(apss_camera_processor) << std::format("Frame {} expired after {}ms. System seems to be overloaded.", frame->id().toStdString(), frame_timeout);
                return false;
            }
        }

        // set it back to false or other stages will skip
        frame->setHasBeenProcessed(false);
    } else {
        static const int frame_timeout = m_config.push_based_timeout
                                             ? m_config.push_based_timeout.value()
                                             : 100;

        queue.emplace(frame);

        if (!frame->hasBeenProcessed()) {
            QMutexLocker<QMutex> lock(&mtx);
            if (!m_waitCondition->wait(&mtx, frame_timeout)) {
                qCCritical(apss_camera_processor) << std::format("Frame {} expired after {}ms, in push based mode!!!", frame->id().toStdString(), frame_timeout);
                // return false;
            }
        }

        // set it back to false or other stages will skip
        frame->setHasBeenProcessed(false);
    }

    return true;
}

void CameraProcessor::estimateChangesInArea(PredictionList &predictions, std::unordered_map<int, TrackedObject> &objectsHistory)
{
    // Remove lost histories
    for (auto it = objectsHistory.begin(); it != objectsHistory.end(); ) {
        if (it->second.last_seen_at.secsTo(QTime::currentTime()) > TRACK_MAX_LOST_WAIT) {
            // The tracker won't re-track it, if it's lost for a second
            // but we shouldn't be greedy. Got lots of free RAM.
            it = objectsHistory.erase(it);
        } else {
            ++it;
        }    
    }

    for (int i = 0; i < predictions.size(); ++i) {
        auto& prediction = predictions.at(i);
        if (prediction.trackerId < 0) // Leave the untracked and lost alone.
            continue;

        // get or create history
        TrackedObject& history = objectsHistory[prediction.trackerId];
        history.last_seen_at = QTime::currentTime();

        const int box_area = prediction.box.area();
        history.max_observed_area = std::max(history.max_observed_area, box_area);
        
        // Skip if too small or side view
        const float aspect_ratio = static_cast<float>(prediction.box.width) / prediction.box.height;
        if (box_area < TRACK_MIN_AREA || aspect_ratio > TRACK_MAX_ASPECT_RATIO) {
            prediction.hasDeltas = false;
            continue;
        }

        bool is_approaching = false;
        bool is_departing = false;

        if (history.last_triggered_area != -1) {
            const int ref_area = history.last_triggered_area;
            is_approaching = (box_area >= ref_area * TRACK_APPROACH_THRESHOLD);
            is_departing = (box_area <= ref_area * TRACK_DEPART_THRESHOLD);
        }

        // trigger license plate detection when:
        // - first valid observation
        // - vehicle is approaching significantly
        // - new maximum area observed (even if stationary)
        if (history.last_triggered_area == -1 || is_approaching || box_area > history.max_observed_area) {
            prediction.hasDeltas = true;
            history.last_triggered_area = box_area;
            history.max_observed_area = box_area;  // Reset max for new approach
        }
        // Reset trigger if vehicle is departing
        else if (is_departing) {
            prediction.hasDeltas = false;
            history.last_triggered_area = -1;  // Reset to force re-trigger
        } else {
            prediction.hasDeltas = false;
        }
    }
}

PredictionList CameraProcessor::filterObjectPredictions(const PredictionList &results, const std::map<std::string, FilterConfig> &objectsToFilter)
{
    PredictionList filtered_results;
    for (const auto &prediction : results) {
        if (objectsToFilter.contains(prediction.className)) {
            const FilterConfig &config = objectsToFilter.at(prediction.className);

            const int box_area = prediction.box.area();
            if (box_area < config.min_area.value_or(0) || box_area > config.max_area.value_or(24000000))
                continue;

            // ratio

            if (prediction.conf < config.threshold.value_or(0.7))
                continue;

            // min_score
            // raw_mask

            filtered_results.emplace_back(prediction);
        }
    }

    return filtered_results;
}

#include "moc_cameraprocessor.cpp"