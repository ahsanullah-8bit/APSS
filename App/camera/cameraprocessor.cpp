#include "cameraprocessor.h"

#include "config/objectconfig.h"

CameraProcessor::CameraProcessor(const QString &cameraName,
                                 const CameraConfig &config,
                                 const std::optional<ModelConfig> &modelConfig,
                                 const std::optional<std::map<int, std::string> > &labelmap,
                                 SharedFrameBoundedQueue &inDetectorFrameQueue,
                                 QSharedPointer<QWaitCondition> waitCondition,
                                 SharedCameraMetrics cameraMetrics,
                                 QObject *parent)
    : QThread(parent)
    , m_cameraName(cameraName)
    , m_config(config)
    , m_modelConfig(modelConfig)
    , m_labelmap(labelmap)
    , m_inDetectorFrameQueue(inDetectorFrameQueue)
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

    while(!isInterruptionRequested()) {
        // We may do some other stuff like, Object Tracking, Motion Detection, etc. as well. But later...

        SharedFrame frame;
        // TODO: use try_pop for gracefull and non-bound pop. Because blocking pop may cause threading issues,
        // a sleeper while-loop seems much reasonable. Wish it had a timed one.
        frame_queue->pop(frame);
        if(!frame)
            continue;

        // Put the frame in the detect, using try_push, wait for 5 ms, if the push was successful
        bool frame_pushed = m_inDetectorFrameQueue.try_push(frame);
        if (!frame_pushed)
            continue;

        m_mtx.lock();
        bool detect_timed_out =
            m_config.image_detect_timeout ? !m_waitCondition->wait(&m_mtx, m_config.image_detect_timeout.value()) : !m_waitCondition->wait(&m_mtx);

        if (detect_timed_out) {
            frame->setHasExpired(true);
            qDebug() << std::format("Frame {} expired", frame->id().toStdString());
            m_mtx.unlock();
            continue;
        }
        m_mtx.unlock();

        emit frameChanged(frame);
    }

    // Empty the frame Queue
}

void CameraProcessor::trackCamera()
{

}

void CameraProcessor::processFrames()
{

}

#include "moc_cameraprocessor.cpp"
