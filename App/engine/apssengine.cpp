#include "apssengine.h"

#include <opencv2/opencv.hpp>

APSSEngine::APSSEngine(QObject *parent)
    : QObject{parent}
    , m_fileStream(m_unProcessedFrameQueue)
    , m_ffmpegFileStream(m_unProcessedFrameQueue)
    , m_objectDetector(m_unProcessedFrameQueue
                       , m_objDetectedFrameQueue
                       , "models/yolo11n.onnx",
                       1,
                       0.5)
    , m_lprDetectorSession(m_objDetectedFrameQueue
                           , m_lpDetectedFrameQueue
                           , std::set<std::string>{"car", "motorcycle", "bus", "truck"}
                           , std::string("models/yolo11n-pose-1700.onnx")
                           , 1
                           , 0.8f)
    , m_frameInformer(m_lpDetectedFrameQueue, m_objectDetector.detector(), m_lprDetectorSession.detector())
{
    // Queues defaults
    m_unProcessedFrameQueue.set_capacity(2);
    m_objDetectedFrameQueue.set_capacity(4);
    m_lpDetectedFrameQueue.set_capacity(6);

    // Worker threads defaults
    m_fileStream.setObjectName("filestream");
    m_ffmpegFileStream.setObjectName("ffmpeg_file_stream");
    m_objectDetector.setObjectName("object_detector");
    m_lprDetectorSession.setObjectName("license_plate_detector");
    m_frameInformer.setObjectName("frame_informer");

    connect(&m_frameInformer, &BoundedInformer::frameChanged, this, &APSSEngine::onFrameChanged);

    // Starting the threads
    m_lprDetectorSession.start();
    m_frameInformer.start();
    m_objectDetector.start();
}

APSSEngine::~APSSEngine()
{
    // Stop the producer and consumer. Though only one will be stopped by this in case of
    // different frequency of production/consumption.
    try {
        m_fileStream.requestInterruption();
        m_ffmpegFileStream.requestInterruption();
        m_objectDetector.requestInterruption();
        m_lprDetectorSession.requestInterruption();
        m_frameInformer.requestInterruption();

        // Stop the waiting threads/producers/consumers.
        m_unProcessedFrameQueue.abort();
        m_objDetectedFrameQueue.abort();
        m_lpDetectedFrameQueue.abort();

        // Wait on threads one by one.
        m_fileStream.wait();
        m_objectDetector.wait();
        m_lprDetectorSession.wait();
        m_frameInformer.wait();
    }
    catch (const std::exception &e) {
        qInfo() << "Uncaught exception" << e.what();
    }
    catch (...) {
        qFatal() << "Uncaught/Uknown exception";
    }
}

void APSSEngine::openAFootage(const QString &path, QVideoSink *sink)
{
    Q_ASSERT(!path.isEmpty());

    setCameraFeedSink(m_fileStream.id(), sink);

    // qDebug() << "openFootage:" << path;
    if (path.startsWith("file:///"))
        const_cast<QString&>(path).slice(8);

    // // See if the File reader is running. If so, terminate it first
    // if (m_fileStream.isRunning()) {
    //     m_fileStream.requestInterruption();
    //     m_fileStream.wait();
    // }

    // m_fileStream.setFilePath(path);
    // m_fileStream.start();

    // See if the File reader is running. If so, terminate it first
    if (m_ffmpegFileStream.isRunning()) {
        m_ffmpegFileStream.requestInterruption();
        m_ffmpegFileStream.wait();
    }

    m_ffmpegFileStream.setPath(path);
    m_ffmpegFileStream.start();
}

void APSSEngine::addRemoteCamera(const QString &path)
{

}

void APSSEngine::setCameraFeedSink(const QString &camera_id, QVideoSink *sink)
{
    Q_ASSERT(sink);

    m_cameraOutputFeeds[camera_id] = sink;
}

QVideoSink *APSSEngine::getCameraFeed(const QString &camera_id, QVideoSink *defaultValue)
{
    return m_cameraOutputFeeds.value(camera_id, defaultValue);
}

QVideoSink *APSSEngine::getCameraFeedByPath(const QString &path, QVideoSink *defaultValue)
{
    return nullptr;
}

QVideoSink *APSSEngine::getCameraFeed(const QCamera &camera, QVideoSink *defaultValue)
{
    return nullptr;
}

void APSSEngine::start()
{
    qInfo() << "Starting APSSEngine";
    // Start the whole engine
    // Start the thread
}

void APSSEngine::stop()
{
    qInfo() << "Stopping APSSEngine";
    // Stop the the engine
    // Stop this thread
}

void APSSEngine::onFrameChanged(const Frame &frame)
{
    FrameId frame_id = frame.frameId();
    if (!m_cameraOutputFeeds.contains(frame_id.cameraId)) {
        qWarning() << std::format("No such camera as {}, skipping frame {}.", frame_id.cameraId.toStdString(), frame_id.frameIndex.toStdString());
        return;
    }

    // Forward the output for that camera
    cv::Mat mat = frame.data();
    if (mat.type() != CV_8UC3 || mat.empty())
        return;

    cv::Mat rgb;
    cv::cvtColor(mat, rgb, cv::COLOR_BGR2RGB);
    QImage img(rgb.data, rgb.cols, rgb.rows, static_cast<int>(rgb.step), QImage::Format_RGB888);

    QVideoSink *output_sink = m_cameraOutputFeeds[frame_id.cameraId];
    Q_ASSERT(output_sink);

    output_sink->setVideoFrame(QVideoFrame(img.copy()));
}
