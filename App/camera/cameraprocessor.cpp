#include "cameraprocessor.h"

#include "config/objectconfig.h"
#include "detectors/image.h"
#include "utils/eventspersecond.h"
#include "track/tracker.h"

CameraProcessor::CameraProcessor(const QString &cameraName,
                                 const CameraConfig &config,
                                 const std::optional<ModelConfig> &modelConfig,
                                 const std::optional<std::map<int, std::string> > &labelmap,
                                 SharedFrameBoundedQueue &inDetectorFrameQueue,
                                 SharedFrameBoundedQueue &inLPDetectorFrameQueue,
                                 QSharedPointer<QWaitCondition> waitCondition,
                                 SharedCameraMetrics cameraMetrics,
                                 QObject *parent)
    : QThread(parent)
    , m_cameraName(cameraName)
    , m_config(config)
    , m_modelConfig(modelConfig)
    , m_labelmap(labelmap)
    , m_inDetectorFrameQueue(inDetectorFrameQueue)
    , m_inLPDetectorFrameQueue(inLPDetectorFrameQueue)
    , m_waitCondition(waitCondition)
    , m_cameraMetrics(cameraMetrics)
{
    setObjectName(QString("apss.thread:%1").arg(m_cameraName));
}

void CameraProcessor::run()
{
    QSharedPointer<SharedFrameBoundedQueue> frame_queue = m_cameraMetrics->frameQueue();

    const ObjectConfig &objects_config = m_config.objects ? m_config.objects.value() : ObjectConfig();

    Tracker tracker(objects_config.track);

    // COCO class names
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

    // Unique scalar colors per class (RGB)
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
        trackAndEstimateDeltas(frame, tracker, Prediction::Objects);

        if (!predict(frame, m_inLPDetectorFrameQueue))
            continue;

        detectors_eps.update();
        m_cameraMetrics->setDetectionFPS(detectors_eps.eps());

        // draw results
        Frame::TypePredictionsHash predictions_list = frame->predictions();
        cv::Mat frame_mat = frame->data();
        const auto &objects = predictions_list[Prediction::Objects];
        const auto &lps = predictions_list[Prediction::LicensePlates];

        static const std::vector<std::pair<int, int>> skeleton = {{0, 1}, {1, 2}, {2, 3}, {3, 0}};

        Utils::drawBoundingBox(frame_mat, objects, class_names, class_colors);
        Utils::drawPoseEstimation(frame_mat, lps, skeleton, true);
        frame->setData(frame_mat);
        // -- draw results

        process_eps.update();
        m_cameraMetrics->setProcessFPS(process_eps.eps());

        // Send the frame to listensers (main GUI thread).
        emit frameChanged(frame);
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
                qWarning() << std::format("Frame {} expired after {}ms. System seems to be overloaded.", frame->id().toStdString(), frame_timeout);
                return false;
            }
        }
    } else {
        static const int frame_timeout = m_config.push_based_timeout
                                             ? m_config.push_based_timeout.value()
                                             : 100;

        queue.emplace(frame);

        if (!frame->hasBeenProcessed()) {
            QMutexLocker<QMutex> lock(&mtx);
            if (!m_waitCondition->wait(&mtx, frame_timeout)) {
                qCritical() << std::format("Frame {} expired after {}ms, in push based mode!!!", frame->id().toStdString(), frame_timeout);
            }
        }
    }

    return true;
}

bool CameraProcessor::trackAndEstimateDeltas(SharedFrame frame, Tracker &tracker, Prediction::Type predictionType)
{
    // TODO: Find a proper way to hold seen ids.
    // Don't mistaken this for the tracker remembering objects. This one is for us to avoid going to another
    // stage for the same object in multiple frames. It focuses on whether it's a new
    // object or an old with increased quality/size/area. This usually helps in cases
    // of camera being mounted on the ground.
    static QList<std::pair<size_t, int>> delta_objects(tracker.trackBuffer(), {}); // <id, area>
    bool has_deltas = false;

    // Filter predictions
    PredictionList &obj_predictions = frame->predictionsByRef(predictionType);
    std::vector<int> track_ids = tracker.track(obj_predictions);

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
            has_deltas = true;
        }
    }

    // To avoid running lp detector
    return has_deltas;
}

#include "moc_cameraprocessor.cpp"
