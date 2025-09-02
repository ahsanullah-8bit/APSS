#pragma once

#include <QThread>
#include <QHash>
#include <QQmlEngine>
#include <QSettings>
#include <QFile>
#include <QCamera>
#include <QVideoSink>
#include <QVideoFrame>
#include <QMediaDevices>
#include <QMediaCaptureSession>
#include <QMediaRecorder>

#include <odb/sqlite/database.hxx>
#include <tbb_patched.h>

#include <camera/camerametrics.h>
#include <config/apssconfig.h>
#include <events/zmqproxy.h>
#include <models/camerametricsmodel.h>
#include <output/recordingsmanager.h>
#include <output/trackedobjectprocessor.h>
#include <utils/frame.h>
// This class will handle most of the stuff
class APSSEngine : public QObject
{
    Q_OBJECT
    Q_PROPERTY(SharedCameraMetricsModel cameraMetricsModel READ cameraMetricsModel FINAL)

public:
    explicit APSSEngine(APSSConfig *config, QObject *parent = nullptr);
    ~APSSEngine();
    SharedCameraMetricsModel cameraMetricsModel() const;

public slots:
    void start();
    void stop();
    void onFrameChanged(SharedFrame frame);

private:
    void ensureDirs();
    void initCameraMetrics();
    void initQueues();
    void initDatabase();
    void initRecordingManager();
    // ...
    void startDetectors();
    // void initEmbeddingsManager();
    // void bindDatabase();
    // void initEmbeddingsClient();
    // void initIntraProcessComunicator();
    // void startVideoOutputProcessor();
    void startDetectedFramesProcessor();
    void startCameraProcessors();
    void startCameraCaptureProcesses();
    // void startStorageMaintainer();
    // void startEventProcessor();
    // void startCleanupProcesses();
    // void startAPSSWatchdog();

private:

    QHash<QString, QVideoSink*> m_cameraOutputFeeds;
    SharedFrameBoundedQueue m_inUnifiedObjDetectorQ;
    SharedFrameBoundedQueue m_inUnifiedLPDetectorQ;
    QHash<QString, QSharedPointer<QWaitCondition>> m_cameraWaitConditions;

    // New interface
    APSSConfig *m_config;
    std::atomic_bool m_stopEvent;
    // SharedFrameBoundedQueue m_detectionQueue;
    QHash<QString, QSharedPointer<QThread>> m_detectors;
    QSharedPointer<QThread> m_lpdetector;
    QHash<QString, SharedCameraMetrics> m_cameraMetrics;
    SharedCameraMetricsModel m_cameraMetricsModel;

    ZMQProxyThread *m_intraZMQProxy;
    std::shared_ptr<odb::database> m_db;
    SharedFrameBoundedQueue m_trackedFramesQueue;
    QSharedPointer<TrackedObjectProcessor> m_trackedObjectsProcessor;
    // QSharedPointer<VideoRecorder> m_recorder;

    QPair<RecordingsManager*, QThread*> m_recordingsManager;
};
