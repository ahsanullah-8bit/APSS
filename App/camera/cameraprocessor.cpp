#include <ranges>

#include <QLoggingCategory>

#include "cameraprocessor.h"

#include "config/objectconfig.h"
#include "detectors/image.h"
#include "utils/eventspersecond.h"
#include "track/tracker.h"

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

    // COCO class names and their colors
    const std::vector<std::string> class_names = {
        "person", "bicycle", "car", "motorcycle", "airplane", "bus", "train", "truck",
        "boat", "traffic light", "fire hydrant", "stop sign", "parking meter", "bench",
        "bird", "cat", "dog", "horse", "sheep", "cow", "elephant", "bear", "zebra",
        "giraffe", "backpack", "umbrella", "handbag", "tie", "suitcase", "frisbee",
        "skis", "snowboard", "sports ball", "kite", "baseball bat", "baseball glove",
        "skateboard", "surfboard", "tennis racket", "bottle", "wine glass", "cup",
        "fork", "knife", "spoon", "bowl", "banana", "apple", "sandwich", "orange",
        "broccoli", "carrot", "hot dog", "pizza", "donut", "cake", "chair", "couch",
        "potted plant", "bed", "dining table", "toilet", "tv", "laptop", "mouse",
        "remote", "keyboard", "cell phone", "microwave", "oven", "toaster", "sink",
        "refrigerator", "book", "clock", "vase", "scissors", "teddy bear", "hair drier",
        "toothbrush"
    };
    const std::vector<cv::Scalar> class_colors = {
        {255, 0, 0}, {255, 128, 0}, {255, 255, 0}, {128, 255, 0}, {0, 255, 0},
        {0, 255, 128}, {0, 255, 255}, {0, 128, 255}, {0, 0, 255}, {128, 0, 255},
        {255, 0, 255}, {255, 0, 128}, {128, 0, 0}, {128, 64, 0}, {128, 128, 0},
        {64, 128, 0}, {0, 128, 0}, {0, 128, 64}, {0, 128, 128}, {0, 64, 128},
        {0, 0, 128}, {64, 0, 128}, {128, 0, 128}, {128, 0, 64}, {64, 0, 0},
        {192, 64, 0}, {192, 192, 0}, {64, 192, 0}, {0, 192, 0}, {0, 192, 64},
        {0, 192, 192}, {0, 64, 192}, {0, 0, 192}, {64, 0, 192}, {192, 0, 192},
        {192, 0, 64}, {64, 0, 64}, {255, 64, 64}, {255, 128, 64}, {255, 255, 64},
        {128, 255, 64}, {64, 255, 64}, {64, 255, 128}, {64, 255, 255}, {64, 128, 255},
        {64, 64, 255}, {128, 64, 255}, {255, 64, 255}, {255, 64, 128}, {128, 64, 64},
        {128, 128, 64}, {64, 128, 64}, {64, 128, 128}, {64, 64, 128}, {128, 64, 128},
        {128, 64, 192}, {255, 192, 0}, {192, 255, 0}, {0, 255, 192}, {0, 192, 255},
        {0, 0, 255}, {255, 0, 192}, {192, 0, 255}, {255, 0, 255}, {255, 192, 192},
        {255, 255, 192}, {192, 255, 255}, {192, 192, 255}, {192, 128, 255}, {255, 128, 255},
        {255, 192, 128}, {192, 128, 128}, {128, 192, 128}, {128, 255, 128}, {128, 128, 255},
        {128, 128, 128}, {0, 0, 0}
    };

    const std::vector<std::string> lp_classes = { "license_plate" };
    const std::vector<std::pair<int, int>> lp_skeleton = {{0, 1}, {1, 2}, {2, 3}, {3, 0}};

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
        if (objects_config.filters)
            frame->setPredictions(filterObjectPredictions(frame->predictions(), objects_config.filters.value()));

        trackAndEstimateDeltas2(frame, tracker);

        if (!predict(frame, m_inLPDetectorFrameQueue))
            continue;

        detectors_eps.update();
        m_cameraMetrics->setDetectionFPS(detectors_eps.eps());

        recognizeLicensePlates(frame, lp_classes);

        process_eps.update();
        m_cameraMetrics->setProcessFPS(process_eps.eps());

        // Send the frame to listensers.
        m_trackedFrameQueue.try_emplace(frame);
    }

    // Empty the frame Queue
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

void CameraProcessor::trackAndEstimateDeltas(SharedFrame frame, Tracker &tracker)
{
    // TODO: Find a proper way to hold seen ids.
    // Don't mistaken this for the tracker remembering objects. This one is for us to avoid going to another
    // stage for the same object in multiple frames. It focuses on whether it's a new
    // object or an old with increased quality/size/area. This usually helps in cases
    // of camera being mounted on the ground.
    static QList<std::pair<size_t, int>> delta_objects(tracker.trackBuffer(), {}); // <id, area>
    bool has_deltas = false;

    // Filter predictions
    // TODO: This may have a bug, as we filter track_objects inside the tracker
    // and only return track ids for this selected objects
    PredictionList obj_predictions = frame->predictions();
    const auto track_ids = tracker.track(obj_predictions);

    for (int i = 0; i < track_ids.size(); ++i) {
        int id = track_ids.at(i);
        if (id == -1)
            continue;

        obj_predictions[i].trackerId = id;

        int delta_indx = id % (delta_objects.size());  // Resets when limit is reached
        int box_area = obj_predictions.at(i).box.area();

        // We proceed if the id is not seen before or old area is less than the current (box is much bigger). Then
        // we reconsider the other stages to re-process these predictions if they want to. i.e. LP was not visible, now is.
        int area_increase = (delta_objects.at(delta_indx).second * DET_RECONSIDER_AREA_INCREASE);
        if (delta_objects.at(delta_indx).first != id
            || delta_objects.at(delta_indx).second + area_increase < box_area) {

            delta_objects[delta_indx] = std::pair(id, box_area);
            obj_predictions[i].hasDeltas = true;
        }
    }

    frame->setPredictions(std::move(obj_predictions));
}

struct TrackedObjectHistory {
    int last_seen_frame;
    int last_triggered_area = -1;  // -1 = never triggered
    int max_observed_area = 0;
};

void CameraProcessor::trackAndEstimateDeltas2(SharedFrame frame, Tracker &tracker)
{
    static int frame_counter = 0;
    static std::unordered_map<int, TrackedObjectHistory> object_histories;
    frame_counter++;

    // remove old histories
    const int track_buffer = tracker.trackBuffer();
    for (auto it = object_histories.begin(); it != object_histories.end(); ) {
        if (frame_counter - it->second.last_seen_frame > track_buffer) {
            it = object_histories.erase(it);
        } else {
            ++it;
        }
    }

    PredictionList obj_predictions = frame->predictions();
    const auto track_ids = tracker.track(obj_predictions);

    const int MIN_AREA = 15'625;            // (125 x 125) minimum area to consider
    const float MAX_ASPECT_RATIO = 2.5f;    // max w/h for valid view
    const float APPROACH_THRESHOLD = 1.1f;  // 10% area increase
    const float DEPART_THRESHOLD = 0.8f;    // 20% area decrease

    for (int i = 0; i < track_ids.size(); ++i) {
        const int id = track_ids[i];
        if (id == -1) continue;

        auto& pred = obj_predictions[i];
        pred.trackerId = id;

        // get or create history
        TrackedObjectHistory& history = object_histories[id];
        history.last_seen_frame = frame_counter;

        const int box_area = pred.box.area();
        // qDebug() << id << box_area;
        const float aspect_ratio = static_cast<float>(pred.box.width) / pred.box.height;
        history.max_observed_area = std::max(history.max_observed_area, box_area);

        // skip if too small or side view
        if (box_area < MIN_AREA || aspect_ratio > MAX_ASPECT_RATIO) {
            pred.hasDeltas = false;
            continue;
        }

        bool is_approaching = false;
        bool is_departing = false;

        if (history.last_triggered_area != -1) {
            const int ref_area = history.last_triggered_area;
            is_approaching = (box_area >= ref_area * APPROACH_THRESHOLD);
            is_departing = (box_area <= ref_area * DEPART_THRESHOLD);
        }

        // trigger license plate detection when:
        // - first valid observation
        // - vehicle is approaching significantly
        // - new maximum area observed (even if stationary)
        if (history.last_triggered_area == -1 || is_approaching || box_area > history.max_observed_area) {
            pred.hasDeltas = true;
            history.last_triggered_area = box_area;
            history.max_observed_area = box_area;  // Reset max for new approach
        }
        // Reset trigger if vehicle is departing
        else if (is_departing) {
            pred.hasDeltas = false;
            history.last_triggered_area = -1;  // Reset to force re-trigger
        } else {
            pred.hasDeltas = false;
        }
    }

    frame->setPredictions(std::move(obj_predictions));
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

void CameraProcessor::recognizeLicensePlates(SharedFrame frame, std::vector<std::string> lp_classes)
{
    // crop images
    PredictionList predictions = frame->predictions();
    if (predictions.empty())
        return;

    MatList crop_batch;
    for (const auto &prediction : predictions) {
        if (std::find(lp_classes.begin(), lp_classes.end(), prediction.className) == lp_classes.end())
            continue;

        cv::Mat crop;
        Utils::perspectiveCrop(frame->data(), crop, prediction.points);
        crop_batch.emplace_back(crop);
    }

    std::vector<PaddleOCR::OCRPredictResultList> results_list = m_ocrEngine.predict(crop_batch);

    // OCR
    // Filter main plate number
    for (auto results : results_list) {
        std::sort(results.begin(), results.end(), [] (const PaddleOCR::OCRPredictResult &a, const PaddleOCR::OCRPredictResult &b) {
            return PaddleOCR::computeArea(a.box) > PaddleOCR::computeArea(b.box)
                && a.score > b.score;
        });

        std::vector<std::string> res;
        for (const auto &result : results) {
            res.push_back(result.text);
        }

        qDebug() << res;
    }

    frame->setOcrResults(std::move(results_list));
}

#include "moc_cameraprocessor.cpp"
