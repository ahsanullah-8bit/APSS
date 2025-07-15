#include "cameraprocessor.h"

#include "config/objectconfig.h"
#include "detectors/image.h"
#include "utils/eventspersecond.h"

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
    auto objects_to_track = m_config.objects->track;
    auto object_filters = m_config.objects->filters;;


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

    const int frame_time_out = m_config.image_detect_timeout
                                   ? m_config.image_detect_timeout.value()
                                   : 20;

    while(!isInterruptionRequested()) {
        SharedFrame frame;
        frame_queue->pop(frame);
        if(!frame)
            continue;

        if (predict(frame, m_inDetectorFrameQueue, frame_time_out)
            && predict(frame, m_inLPDetectorFrameQueue, frame_time_out)) {
            detectors_eps.update();
            m_cameraMetrics->setDetectionFPS(detectors_eps.eps());
        } else {
            continue;
        }

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
                              SharedFrameBoundedQueue &queue,
                              const int frameTimeOut)
{
    static const bool is_pull_based = m_cameraMetrics->isPullBased();

    if (is_pull_based) {
        if (!queue.try_emplace(frame))
            return false;

        m_mtx.lock();
        if (!m_waitCondition->wait(&m_mtx, frameTimeOut)) {
            frame->setHasExpired(true);
            qWarning() << std::format("Frame {} expired after {}ms. System seems to be overloaded.", frame->id().toStdString(), frameTimeOut);
            m_mtx.unlock();
            return false;
        }
        m_mtx.unlock();
    } else {
        queue.emplace(frame);

        m_mtx.lock();
        m_waitCondition->wait(&m_mtx);
        m_mtx.unlock();
    }

    return true;
}

#include "moc_cameraprocessor.cpp"
