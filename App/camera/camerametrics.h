#pragma once

#include <QObject>
#include <QVideoSink>

#include <atomic>

#include "utils/frame.h"
#include <tbb_patched.h>

class CameraMetrics : QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name FINAL)
    Q_PROPERTY(double cameraFPS READ cameraFPS WRITE setCameraFPS NOTIFY cameraFPSChanged FINAL)
    Q_PROPERTY(double detectionFPS READ detectionFPS WRITE setDetectionFPS NOTIFY detectionFPSChanged FINAL)
    Q_PROPERTY(int detectionFrame READ detectionFrame WRITE setDetectionFrame NOTIFY detectionFrameChanged FINAL)
    Q_PROPERTY(double processFPS READ processFPS WRITE setProcessFPS NOTIFY processFPSChanged FINAL)
    Q_PROPERTY(double skippedFPS READ skippedFPS WRITE setSkippedFPS NOTIFY skippedFPSChanged FINAL)
    Q_PROPERTY(int readStart READ readStart WRITE setReadStart NOTIFY readStartChanged FINAL)
    // Q_PROPERTY(std::atomic_int audioRMS READ audioRMS WRITE setAudioRMS NOTIFY audioRMSChanged FINAL)
    // Q_PROPERTY(std::atomic_int audiodBFS READ audiodBFS WRITE setAudiodBFS NOTIFY audiodBFSChanged FINAL)

    Q_PROPERTY(QVideoSink* videoSink READ videoSink WRITE setVideoSink NOTIFY videoSinkChanged FINAL)
    Q_PROPERTY(QSharedPointer<SharedFrameBoundedQueue> frameQueue READ frameQueue WRITE setFrameQueue NOTIFY frameQueueChanged FINAL)
    Q_PROPERTY(QSharedPointer<QThread> thread READ thread WRITE setThread NOTIFY threadChanged FINAL)
    Q_PROPERTY(QSharedPointer<QThread> captureThread READ captureThread WRITE setCaptureThread NOTIFY captureThreadChanged FINAL)

public:
    CameraMetrics(const QString &name, bool isPullBased = false, QObject *parent = nullptr);

    QString name() const;
    double cameraFPS() const;
    double detectionFPS() const;
    double processFPS() const;
    double skippedFPS() const;
    int detectionFrame() const;
    int readStart() const;
    QVideoSink *videoSink() const;
    QSharedPointer<SharedFrameBoundedQueue> frameQueue() const;
    QSharedPointer<QThread> thread() const;
    QSharedPointer<QThread> captureThread() const;
    bool isPullBased() const;

public slots:
    void setCameraFPS(double newCameraFPS);
    void setDetectionFPS(double newDetectionFPS);
    void setProcessFPS(double newProcessFPS);
    void setSkippedFPS(double newSkippedFPS);
    void setDetectionFrame(int newDetectionFrame);
    void setReadStart(int newReadStart);
    void setVideoSink(QVideoSink *newVideoSink);
    void setFrameQueue(QSharedPointer<SharedFrameBoundedQueue> newFrameQueue);
    void setThread(QSharedPointer<QThread> newThread);
    void setCaptureThread(QSharedPointer<QThread> newCaptureThread);

signals:
    void cameraFPSChanged(double);
    void detectionFPSChanged(double);
    void processFPSChanged(double);
    void skippedFPSChanged(double);
    void detectionFrameChanged(int detectionFrame);
    void readStartChanged(int readStart);
    void videoSinkChanged(QVideoSink *videoSink);
    void frameQueueChanged(QSharedPointer<SharedFrameBoundedQueue> frameQueue);
    void threadChanged(QSharedPointer<QThread> thread);
    void captureThreadChanged(QSharedPointer<QThread> captureThread);

private:
    QString m_name;
    std::atomic<double> m_cameraFPS;
    std::atomic<double> m_detectionFPS;
    std::atomic<double> m_processFPS;
    std::atomic<double> m_skippedFPS;
    std::atomic_int m_detectionFrame;
    std::atomic_int m_readStart;
    std::atomic<QVideoSink *> m_videoSink = nullptr;
    QSharedPointer<SharedFrameBoundedQueue> m_frameQueue;
    QSharedPointer<QThread> m_thread;
    QSharedPointer<QThread> m_captureThread;

    bool m_isPullBased = false;
};

using SharedCameraMetrics = QSharedPointer<CameraMetrics>;
