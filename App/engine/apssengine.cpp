#include "apssengine.h"
#include <opencv2/opencv.hpp>

#include <QDir>
#include <QVideoSink>
#include <QLoggingCategory>

#include <odb/core.hxx>
#include <odb/database.hxx>
#include <odb/transaction.hxx>
#include <odb/session.hxx>
#include <odb/schema-catalog.hxx>
#include <odb/sqlite/database.hxx>

#include "db/sqlite/event-odb.hxx"

#include "apss.h"
#include "camera/cameraprocessor.h"
#include "camera/cameracapture.h"
#include "detectors/objectdetectorsession.h"
#include "detectors/lpdetectorsession.h"

Q_STATIC_LOGGING_CATEGORY(apss_engine, "apss.engine")

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
    qCInfo(apss_engine) << "Starting APSSEngine";
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
    qCInfo(apss_engine) << "Stopping APSSEngine";
    // Stop the producer and consumer. Though only one will be stopped by this in case of
    // different frequency of production/consumption.
    try {

        const QList<QString> metrics_keys = m_cameraMetrics.keys();

        // Stop capture processes
        for (const auto &name : metrics_keys) {
            QSharedPointer<QThread> capture_thread = m_cameraMetrics[name]->captureThread();
            if (capture_thread) {
                qCInfo(apss_engine) << "Requesting a gracefull interupt of camera capture thread" << name;
                capture_thread->requestInterruption();

                if (!capture_thread->wait(50)) {
                    qCWarning(apss_engine) << "Gracefull termination timed-out for camera capture thread" << name << ", forcing termination";
                    capture_thread->terminate();
                    capture_thread->wait();
                }
            }
        }

        // Stop camera processors
        for (const auto &name : metrics_keys) {
            QSharedPointer<QThread> camera_thread = m_cameraMetrics[name]->thread();
            if (camera_thread) {
                qCInfo(apss_engine) << "Requesting a gracefull interupt of camera processor thread" << name;
                camera_thread->requestInterruption();

                if (!camera_thread->wait(50)) {
                    qCWarning(apss_engine) << "Gracefull termination timed-out for camera processor thread" << name << ", forcing termination";
                    camera_thread->terminate();
                    camera_thread->wait();
                }

                // TODO: Stop/Empty the queue (metrics.frame_queue)
            }
        }

        // Stop detectors
        const QList<QString> detector_keys = m_detectors.keys();
        for (const auto& name : detector_keys) {
            qCInfo(apss_engine) << "Requesting a gracefull interupt of detector thread" << name;
            QSharedPointer<QThread> detector_thread = m_detectors[name];

            if (detector_thread) {
                detector_thread->requestInterruption();
                m_inUnifiedObjDetectorQ.abort();

                if (!detector_thread->wait(50)) {
                    qCWarning(apss_engine) << "Gracefull termination timed-out for detector thread" << name << ", forcing termination";
                    detector_thread->terminate();
                    detector_thread->wait();
                }
            }
        }

        // lp
        m_lpdetector->requestInterruption();
        m_inUnifiedLPDetectorQ.abort();
        if (m_lpdetector->wait(50)) {
            qCWarning(apss_engine) << "Gracefull termination timed-out for detector thread"
                       << m_lpdetector->objectName() << ", forcing termination";
            m_lpdetector->terminate();
            m_lpdetector->wait();
        }

        m_trackedObjectsProcessor->requestInterruption();
        m_trackedFramesQueue.abort();
        if (m_trackedObjectsProcessor->wait(50)) {
            qCWarning(apss_engine) << "Gracefull termination timed-out for tracked object processor thread"
                       << m_trackedObjectsProcessor->objectName() << ", forcing termination";
            m_trackedObjectsProcessor->terminate();
            m_trackedObjectsProcessor->wait();
        }
    }
    catch (const std::exception &e) {
        qCInfo(apss_engine) << "Uncaught exception" << e.what();
    }
    catch (...) {
       qCFatal(apss_engine) << "Uncaught/Uknown exception";
    }
}

void APSSEngine::onFrameChanged(SharedFrame frame)
{
    if (!m_cameraMetrics.contains(frame->cameraId())) {
        qCWarning(apss_engine) << QString("No such camera as %1, skipping frame %2.").arg(frame->cameraId(), frame->frameId());
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
            qCInfo(apss_engine) << "Creating directory" << dir.path();
            // Seems like a very poor way to create the current dir, but we'll see.
            dir.mkdir(dir.dirName());
        } else {
            qCInfo(apss_engine) << "Skipping directory" << dir.path();
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
    m_trackedFramesQueue.set_capacity(20);
    /*m_unProcessedFrameQueue.set_capacity(2);
    m_objDetectedFrameQueue.set_capacity(4);
    m_lpDetectedFrameQueue.set_capacity(6);*/
}

void APSSEngine::initDatabase()
{}

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
        qCInfo(apss_engine) << "Detector" << name << "has started:" << m_detectors[_name]->isRunning();
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
    const std::string path = "apss.db";
    std::string sqlite_uri = "sqlite://" + std::string(path) +
                             "?auto_vacuum=FULL&cache_size=-512000&synchronous=NORMAL";

    auto database = std::make_unique<odb::sqlite::database>(path, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE);

    odb::transaction t(database->begin());
    try {
        odb::schema_catalog::create_schema(*database);
        t.commit();
    } catch (const odb::exception& e) {
        t.rollback();
       qCFatal(apss_engine) << "Failed to create database schema" << e.what();
    }
}

void APSSEngine::initEmbeddingsClient()
{

}

void APSSEngine::startVideoOutputProcessor()
{

}

void APSSEngine::startDetectedFramesProcessor()
{
    QSharedPointer<TrackedObjectProcessor> processor(new TrackedObjectProcessor(m_trackedFramesQueue));
    m_trackedObjectsProcessor = processor;
    connect(m_trackedObjectsProcessor.get(), &TrackedObjectProcessor::frameChanged, this, &APSSEngine::onFrameChanged);
    m_trackedObjectsProcessor->start();
}

void APSSEngine::startCameraProcessors()
{
    for (const auto&[name, config] : m_config->cameras) {
        if (!config.enabled)
            qCInfo(apss_engine) << std::format("Camera processor not started for disabled camera {}", name);

        QString cam_name = QString::fromStdString(name);
        QSharedPointer<CameraProcessor> camera_thread(new CameraProcessor(cam_name,
                                                                          config,
                                                                          m_inUnifiedObjDetectorQ,
                                                                          m_inUnifiedLPDetectorQ,
                                                                          m_cameraWaitConditions[cam_name],
                                                                          m_trackedFramesQueue,
                                                                          m_cameraMetrics[cam_name]
                                                                          ));
        m_cameraMetrics[cam_name]->setThread(camera_thread);
        camera_thread->start();
    }
}

void APSSEngine::startCameraCaptureProcesses()
{
    for(const auto &[name, config] : m_config->cameras) {
        if (!config.enabled) {
            qCInfo(apss_engine) << std::format("Camera {} is disabled", name);
            continue;
        }

        SharedCameraMetrics metrics = m_cameraMetrics[QString::fromStdString(name)];
        QSharedPointer<QThread> capture_thread (new CameraCapture(QString::fromStdString(name), metrics, config));
        metrics->setCaptureThread(capture_thread);
        // TODO: Launch with a Higher Thread Priority
        capture_thread->start();

        qCInfo(apss_engine) << std::format("{} camera capture started...", name);
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

