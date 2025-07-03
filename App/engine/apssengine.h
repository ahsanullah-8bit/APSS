#pragma once

#include <QThread>
#include <QVideoSink>
#include <QVideoFrame>
#include <QHash>
#include <QQmlEngine>
#include <QFile>
#include <QCamera>
#include <QMediaDevices>
#include <QSettings>

#include "camera/camerametrics.h"
#include "config/apssconfig.h"
#include "models/camerametricsmodel.h"
#include "utils/frame.h"

#include <tbb_patched.h>

// This class will handle most of the stuff
class APSSEngine : public QObject
{
    Q_OBJECT
    QML_SINGLETON
    Q_PROPERTY(QSharedPointer<QSettings> apssSettings READ apssSettings FINAL)
    Q_PROPERTY(SharedCameraMetricsModel cameraMetricsModel READ cameraMetricsModel FINAL)

public:
    explicit APSSEngine(APSSConfig *config, QObject *parent = nullptr);
    ~APSSEngine();
    SharedCameraMetricsModel cameraMetricsModel() const;
    QSharedPointer<QSettings> apssSettings();

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
    void initEmbeddingsManager();
    void bindDatabase();
    void initEmbeddingsClient();
    void startVideoOutputProcessor();
    void startDetectedFramesProcessor();
    void startCameraProcessors();
    void startCameraCaptureProcesses();
    void startStorageMaintainer();
    void startEventProcessor();
    void startCleanupProcesses();
    void startAPSSWatchdog();

private:

    // FFmpegFile m_ffmpegFileStream;
    QHash<QString, QVideoSink*> m_cameraOutputFeeds;
    // SharedFrameBoundedQueue m_unProcessedFrameQueue;
    // BoundedInformer m_frameInformer;
    // SharedFrameBoundedQueue m_objDetectedFrameQueue;
    // SharedFrameBoundedQueue m_lpDetectedFrameQueue;
    SharedFrameBoundedQueue m_inUnifiedObjDetectorQ;
    // SharedFrameBoundedQueue m_outUnifiedObjDetectorQ;
    QHash<QString, QSharedPointer<QWaitCondition>> m_cameraWaitConditions;

    // New interface
    APSSConfig *m_config;
    QSharedPointer<QSettings> m_apssSettings;
    std::atomic_bool m_stopEvent;
    SharedFrameBoundedQueue m_detectionQueue;
    QHash<QString, QSharedPointer<QThread>> m_detectors;
    QHash<QString, SharedCameraMetrics> m_cameraMetrics;
    SharedCameraMetricsModel m_cameraMetricsModel;
};
