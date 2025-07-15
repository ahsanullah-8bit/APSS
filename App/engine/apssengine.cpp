#include "apssengine.h"
#include <opencv2/opencv.hpp>

#include <QDir>
#include <QVideoSink>

#include "apss.h"
#include "camera/cameraprocessor.h"
#include "camera/cameracapture.h"
#include "detectors/objectdetectorsession.h"
#include "detectors/lpdetectorsession.h"

APSSEngine::APSSEngine(APSSConfig *config, QObject *parent)
    : QObject{parent}
    , m_config(config)
{}

APSSEngine::~APSSEngine()
{}

SharedCameraMetricsModel APSSEngine::cameraMetricsModel() const
{
    return m_cameraMetricsModel;
}

QSharedPointer<QSettings> APSSEngine::apssSettings()
{
    return m_apssSettings;
}

void APSSEngine::start()
{
    qInfo() << "Starting APSSEngine";
    ensureDirs();
    initCameraMetrics();
    initQueues();
    initDatabase();
    initRecordingManager();
    startDetectors();
    initEmbeddingsManager();
    bindDatabase();
    initEmbeddingsClient();
    startVideoOutputProcessor();
    startDetectedFramesProcessor();
    startCameraProcessors();
    startCameraCaptureProcesses();
    startStorageMaintainer();
    startEventProcessor();
    startCleanupProcesses();
    startAPSSWatchdog();

    // Finally, start the UI or whatever
    m_cameraMetricsModel = SharedCameraMetricsModel(new CameraMetricsModel(m_cameraMetrics));
}

void APSSEngine::stop()
{
    qInfo() << "Stopping APSSEngine";
    // Stop the producer and consumer. Though only one will be stopped by this in case of
    // different frequency of production/consumption.
    try {

        const QList<QString> metrics_keys = m_cameraMetrics.keys();

        // Stop capture processes
        for (const auto &name : metrics_keys) {
            QSharedPointer<QThread> capture_thread = m_cameraMetrics[name]->captureThread();
            if (capture_thread) {
                qInfo() << "Requesting a gracefull interupt of camera capture thread" << name;
                capture_thread->requestInterruption();

                if (!capture_thread->wait(50)) {
                    qWarning() << "Gracefull termination timed-out for camera capture thread" << name << ", forcing termination";
                    capture_thread->terminate();
                    capture_thread->wait();
                }
            }
        }

        // Stop camera processors
        for (const auto &name : metrics_keys) {
            QSharedPointer<QThread> camera_thread = m_cameraMetrics[name]->thread();
            if (camera_thread) {
                qInfo() << "Requesting a gracefull interupt of camera processor thread" << name;
                camera_thread->requestInterruption();

                if (!camera_thread->wait(50)) {
                    qWarning() << "Gracefull termination timed-out for camera processor thread" << name << ", forcing termination";
                    camera_thread->terminate();
                    camera_thread->wait();
                }

                // TODO: Stop/Empty the queue (metrics.frame_queue)
            }
        }

        // Stop detectors
        const QList<QString> detector_keys = m_detectors.keys();
        for (const auto& name : detector_keys) {
            qInfo() << "Requesting a gracefull interupt of detector thread" << name;
            QSharedPointer<QThread> detector_thread = m_detectors[name];

            if (detector_thread) {
                detector_thread->requestInterruption();
                m_inUnifiedObjDetectorQ.abort();

                if (!detector_thread->wait(50)) {
                    qWarning() << "Gracefull termination timed-out for detector thread" << name << ", forcing termination";
                    detector_thread->terminate();
                    detector_thread->wait();
                }
            }
        }

        // lp
        m_lpdetector->requestInterruption();
        m_inUnifiedLPDetectorQ.abort();
        if (m_lpdetector->wait(50)) {
            qWarning() << "Gracefull termination timed-out for detector thread" << m_lpdetector->objectName() << ", forcing termination";
            m_lpdetector->terminate();
            m_lpdetector->wait();
        }

        // We can force each thread to a wait for tbb::user_abort,
        // by requesting interruption in the stage ahead
        // m_ffmpegFileStream.requestInterruption();
        // m_objectDetector.requestInterruption();
        // m_lprDetectorSession.requestInterruption();
        // m_frameInformer.requestInterruption();

        // // Stop the waiting threads/producers/consumers.
        // m_unProcessedFrameQueue.abort();
        // m_objDetectedFrameQueue.abort();
        // m_lpDetectedFrameQueue.abort();

        // // Wait on threads one by one.
        // m_ffmpegFileStream.wait();
        // m_objectDetector.wait();
        // m_lprDetectorSession.wait();
        // m_frameInformer.wait();
    }
    catch (const std::exception &e) {
        qInfo() << "Uncaught exception" << e.what();
    }
    catch (...) {
        qFatal() << "Uncaught/Uknown exception";
    }
}

void APSSEngine::onFrameChanged(SharedFrame frame)
{
    if (!m_cameraMetrics.contains(frame->cameraId())) {
        qWarning() << QString("No such camera as %1, skipping frame %2.").arg(frame->cameraId(), frame->frameId());
        return;
    }

    // Forward the output for that camera
    cv::Mat mat = frame->data();
    if (mat.type() != CV_8UC3 || mat.empty())
        return;

    cv::Mat rgb;
    cv::cvtColor(mat, rgb, cv::COLOR_BGR2RGB);
    QImage img(rgb.data, rgb.cols, rgb.rows, static_cast<int>(rgb.step), QImage::Format_RGB888);

    QVideoSink *output_sink = m_cameraMetrics[frame->cameraId()]->videoSink();
    Q_ASSERT(output_sink);

    output_sink->setVideoFrame(QVideoFrame(img.copy()));
}

void APSSEngine::ensureDirs()
{
    QList<QDir> dirs = { CONFIG_DIR
        , RECORD_DIR
        , THUMB_DIR
        , CLIPS_CACHE_DIR
        , CACHE_DIR
        , MODEL_CACHE_DIR
        , EXPORT_DIR
    };

    for (const auto &dir : dirs) {
        if (!dir.exists()) {
            qInfo() << "Creating directory" << dir.path();
            // Seems like a very poor way to create the current dir, but we'll see.
            dir.mkdir(dir.dirName());
        } else {
            qInfo() << "Skipping directory" << dir.path();
        }
    }
}

void APSSEngine::initCameraMetrics()
{
    for (const auto &[camera_name, config] : m_config->cameras) {
        QString name = QString::fromStdString(camera_name);
        m_cameraMetrics[name] = SharedCameraMetrics(new CameraMetrics(name, config.pull_based_order.value_or(false)));
    }
}

void APSSEngine::initQueues()
{
    // TODO: Change this to 2 * number_of_cameras enabled
    m_inUnifiedObjDetectorQ.set_capacity(4);
    m_inUnifiedLPDetectorQ.set_capacity(10);
    /*m_unProcessedFrameQueue.set_capacity(2);
    m_objDetectedFrameQueue.set_capacity(4);
    m_lpDetectedFrameQueue.set_capacity(6);*/
}

void APSSEngine::initDatabase()
{

}

void APSSEngine::initRecordingManager()
{
    // ...
}

void APSSEngine::startDetectors()
{
    for (const auto&[name, config] : m_config->cameras)
        m_cameraWaitConditions.emplace(QString::fromStdString(name), new QWaitCondition());

    // Determine how make the data flow. Because frigate communicates frames through Shared Memory and between processes. How do we do it?
    for (const auto &[name, detector_config] : m_config->predictors) {
        QString _name = QString::fromStdString(name);
        m_detectors[_name] = QSharedPointer<QThread>(new ObjectDetectorSession(_name,
                                                                               m_inUnifiedObjDetectorQ,
                                                                               m_cameraWaitConditions,
                                                                               detector_config));
        m_detectors[_name]->start();
        qInfo() << "Detector" << name << "has started:" << m_detectors[_name]->isRunning();
    }

    // lp detector
    PredictorConfig lpdetconfig;
    lpdetconfig.model = ModelConfig();
    lpdetconfig.model->path = "models/yolo11n-pose-1700_320.onnx";
    lpdetconfig.batch_size = 1;

    m_lpdetector = QSharedPointer<QThread>(new LPDetectorSession(m_inUnifiedLPDetectorQ,
                                                                 m_cameraWaitConditions,
                                                                 lpdetconfig,
                                                                 m_config->lpr.has_value() ? m_config->lpr.value() : LicensePlateConfig()));
    m_lpdetector->start();
}

void APSSEngine::initEmbeddingsManager()
{

}

void APSSEngine::bindDatabase()
{

}

void APSSEngine::initEmbeddingsClient()
{

}

void APSSEngine::startVideoOutputProcessor()
{

}

void APSSEngine::startDetectedFramesProcessor()
{

}

void APSSEngine::startCameraProcessors()
{
    for (const auto&[name, config] : m_config->cameras) {
        if (!config.enabled)
            qInfo() << std::format("Camera processor not started for disabled camera {}", name);

        QString cam_name = QString::fromStdString(name);
        QSharedPointer<CameraProcessor> camera_thread(new CameraProcessor(cam_name,
                                                                          config,
                                                                          m_config->model,
                                                                          m_config->model->labelmap,
                                                                          m_inUnifiedObjDetectorQ,
                                                                          m_inUnifiedLPDetectorQ,
                                                                          m_cameraWaitConditions[cam_name],
                                                                          m_cameraMetrics[cam_name]
                                                                          ));

        connect(camera_thread.get(), &CameraProcessor::frameChanged, this, &APSSEngine::onFrameChanged);
        m_cameraMetrics[cam_name]->setThread(camera_thread);
        camera_thread->start();
    }
}

void APSSEngine::startCameraCaptureProcesses()
{
    for(const auto &[name, config] : m_config->cameras) {
        if (!config.enabled) {
            qInfo() << std::format("Camera {} is disabled", name);
            continue;
        }

        SharedCameraMetrics metrics = m_cameraMetrics[QString::fromStdString(name)];
        QSharedPointer<QThread> capture_thread (new CameraCapture(QString::fromStdString(name), metrics, config));
        metrics->setCaptureThread(capture_thread);
        // TODO: Launch with a Higher Thread Priority
        capture_thread->start();

        qInfo() << std::format("{} camera capture started...", name);
    }
}

void APSSEngine::startStorageMaintainer()
{

}

void APSSEngine::startEventProcessor()
{

}

void APSSEngine::startCleanupProcesses()
{

}

void APSSEngine::startAPSSWatchdog()
{

}


#include "moc_apssengine.cpp"

