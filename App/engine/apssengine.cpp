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
#include "db/sqlite/frameprediction-odb.hxx"
#include "db/sqlite/recording-odb.hxx"

#include "apss.h"
#include "camera/cameraprocessor.h"
#include "camera/cameracapture.h"
#include "detectors/objectdetectorsession.h"
#include "detectors/lpdetectorsession.h"
#include "utils/framemanager.h"

Q_STATIC_LOGGING_CATEGORY(logger, "apss.engine")

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

void APSSEngine::start()
{
    qCInfo(logger) << "Starting APSSEngine";

    m_intraZMQProxy = new ZMQProxyThread(this);

    ensureDirs();
    initCameraMetrics();
    initQueues();
    initDatabase();
    initRecordingManager();
    startDetectors();
    // initEmbeddingsManager();
    // bindDatabase();
    // initEmbeddingsClient();
    // initIntraProcessComunicator();
    // startVideoOutputProcessor();
    startDetectedFramesProcessor();
    startCameraProcessors();
    startCameraCaptureProcesses();
    // startStorageMaintainer();
    // startEventProcessor();
    // startCleanupProcesses();
    // startAPSSWatchdog();

    // Finally, start the UI or whatever
    m_cameraMetricsModel = SharedCameraMetricsModel(new CameraMetricsModel(m_cameraMetrics));
}

void APSSEngine::stop()
{
    qCInfo(logger) << "Stopping APSSEngine";
    // Stop the producer and consumer. Though only one will be stopped by this in case of
    // different frequency of production/consumption.
    try {
        const QList<QString> metrics_keys = m_cameraMetrics.keys();

        // Stop capture processes
        for (const auto &name : metrics_keys) {
            QSharedPointer<QThread> capture_thread = m_cameraMetrics[name]->captureThread();
            if (capture_thread) {
                qCInfo(logger) << "Requesting a gracefull interupt of camera capture thread" << name;
                capture_thread->requestInterruption();

                if (!capture_thread->wait(500)) {
                    qCWarning(logger) << "Gracefull termination timed-out for camera capture thread" << name << ", forcing termination";
                    capture_thread->terminate();
                    capture_thread->wait();
                }
            }
        }

        // Stop camera processors
        for (const auto &name : metrics_keys) {
            QSharedPointer<QThread> camera_thread = m_cameraMetrics[name]->thread();
            if (camera_thread) {
                qCInfo(logger) << "Requesting a gracefull interupt of camera processor thread" << name;
                camera_thread->requestInterruption();

                if (!camera_thread->wait(500)) {
                    qCWarning(logger) << "Gracefull termination timed-out for camera processor thread" << name << ", forcing termination";
                    camera_thread->terminate();
                    camera_thread->wait();
                }

                // TODO: Stop/Empty the queue (metrics.frame_queue)
            }
        }

        // Stop detectors
        const QList<QString> detector_keys = m_detectors.keys();
        for (const auto& name : detector_keys) {
            qCInfo(logger) << "Requesting a gracefull interupt of detector thread" << name;
            QSharedPointer<QThread> detector_thread = m_detectors[name];

            if (detector_thread) {
                detector_thread->requestInterruption();
                m_inUnifiedObjDetectorQ.abort();

                if (!detector_thread->wait(500)) {
                    qCWarning(logger) << "Gracefull termination timed-out for detector thread" << name << ", forcing termination";
                    detector_thread->terminate();
                    detector_thread->wait();
                }
            }
        }

        // lp
        m_lpdetector->requestInterruption();
        m_inUnifiedLPDetectorQ.abort();
        if (m_lpdetector->wait(500)) {
            qCWarning(logger) << "Gracefull termination timed-out for detector thread"
                       << m_lpdetector->objectName() << ", forcing termination";
            m_lpdetector->terminate();
            m_lpdetector->wait();
        }

        m_trackedObjectsProcessor->requestInterruption();
        m_trackedFramesQueue.abort();
        if (m_trackedObjectsProcessor->wait(500)) {
            qCWarning(logger) << "Gracefull termination timed-out for tracked object processor thread"
                       << m_trackedObjectsProcessor->objectName() << ", forcing termination";
            m_trackedObjectsProcessor->terminate();
            m_trackedObjectsProcessor->wait();
        }

        auto [manager, thread] = m_recordingsManager;
        // Queued call into the worker’s thread
        QMetaObject::invokeMethod(manager, "stop",
                                  Qt::QueuedConnection);
        // manager->stop();
        thread->quit();
        if (thread->wait(1000)) {
            qCWarning(logger) << "Gracefull termination timed-out for recording manager thread"
                                   << thread->objectName() << ", forcing termination";
            thread->terminate();
            thread->wait();
        }

        m_intraZMQProxy->stop();
    }
    catch (const std::exception &e) {
        qCInfo(logger) << "Uncaught exception" << e.what();
    }
    catch (...) {
       qCFatal(logger) << "Uncaught/Uknown exception";
    }
}

void APSSEngine::onFrameChanged(SharedFrame frame)
{
    const QString camera_name = frame->camera();
    if (!m_cameraMetrics.contains(camera_name)) {
        qCWarning(logger) << QString("No such camera as %1, skipping frame %2.").arg(camera_name, frame->frameIndx());
        return;
    }

    // Forward the output for that camera
    cv::Mat mat = frame->data();
    if (mat.type() != CV_8UC3 || mat.empty())
        return;

    cv::Mat rgb = mat;
    // cv::cvtColor(mat, rgb, cv::COLOR_BGR2RGB);
    QImage img(rgb.data, rgb.cols, rgb.rows, static_cast<int>(rgb.step), QImage::Format_BGR888);
    QVideoFrame videoframe(img);

    QVideoSink *output_sink = m_cameraMetrics[camera_name]->videoSink();
    if (!output_sink)
        return;

    output_sink->setVideoFrame(videoframe);
}

void APSSEngine::ensureDirs()
{
    QList<QDir> dirs = { APSS_DIR,
                        CONFIG_DIR,
                        RECORD_DIR,
                        THUMB_DIR,
                        CLIPS_CACHE_DIR,
                        CACHE_DIR,
                        MODEL_CACHE_DIR,
                        EXPORT_DIR
    };

    for (const auto &dir : dirs) {
        if (!dir.exists()) {
            qCInfo(logger) << "Creating directory" << dir.path();
            // Seems like a very poor way to create the current dir, but we'll see.
            dir.mkpath(dir.path());
        } else {
            qCInfo(logger) << "Skipping directory" << dir.path();
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
}

void APSSEngine::initDatabase()
{
    const std::string path = "apss.db";
    std::string sqlite_uri = "sqlite://" + std::string(path) +
                             "?auto_vacuum=FULL&cache_size=-512000&synchronous=NORMAL";

    m_db = std::make_shared<odb::sqlite::database>(path, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE);

    if (!odb::schema_catalog::exists(*m_db)) {
        odb::transaction t(m_db->begin());
        try {
            odb::schema_catalog::create_schema(*m_db);
            t.commit();
        } catch (const odb::exception& e) {
            t.rollback();
            qCFatal(logger) << "Failed to create database schema" << e.what();
        }
    }
}

void APSSEngine::initRecordingManager()
{
    auto *thread = new QThread(this);
    auto *worker = new RecordingsManager(*m_config, m_db);

    connect(thread, &QThread::started, worker, &RecordingsManager::init);
    connect(worker, &RecordingsManager::destroyed, thread, &QThread::quit);
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);

    m_recordingsManager = { worker, thread };
    thread->start();
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
        qCInfo(logger) << "Detector" << name << "has started:" << m_detectors[_name]->isRunning();
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

// void APSSEngine::bindDatabase()
// {}

void APSSEngine::startDetectedFramesProcessor()
{
    QSharedPointer<TrackedObjectProcessor> processor(new TrackedObjectProcessor(m_trackedFramesQueue, m_db));
    m_trackedObjectsProcessor = processor;

    connect(m_trackedObjectsProcessor.get(), &TrackedObjectProcessor::frameChanged, this, &APSSEngine::onFrameChanged);
    connect(m_trackedObjectsProcessor.get(), &TrackedObjectProcessor::frameChangedWithEvents, m_recordingsManager.first, &RecordingsManager::onRecordFrame);

    m_trackedObjectsProcessor->start();
}

void APSSEngine::startCameraProcessors()
{
    for (const auto&[name, config] : m_config->cameras) {
        if (!config.enabled) {
            qCInfo(logger) << std::format("Camera processor not started for disabled camera {}", name);
            continue;
        }

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
    // frame manager with max frames it should hold for a camera
    FrameManager &frame_manager = FrameManager::instance();

    for(const auto &[name, config] : m_config->cameras) {
        if (!config.enabled) {
            qCInfo(logger) << std::format("Camera {} is disabled", name);
            continue;
        }

        const QString camera_name = QString::fromStdString(name);
        const int max_frames = 5;

        // set max frames for each camera
        frame_manager.setMaxFramesPerCamera(camera_name, max_frames);

        SharedCameraMetrics metrics = m_cameraMetrics[camera_name];
        const_cast<CameraConfig&>(config).name = name;

        QSharedPointer<QThread> capture_thread (new CameraCapture(camera_name, metrics, config));
        metrics->setCaptureThread(capture_thread);
        // TODO: Launch with a Higher Thread Priority
        capture_thread->start();

        qCInfo(logger) << std::format("{} camera capture started...", name);
    }
}


#include "moc_apssengine.cpp"

