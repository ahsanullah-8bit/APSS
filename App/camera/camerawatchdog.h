#pragma once

#include <QThread>

#include <tbb_patched.h>
#include "config/cameraconfig.h"
#include "utils/frame.h"

/*
        self.logger = logging.getLogger(f"watchdog.{camera_name}")
        // self.camera_name = camera_name
        self.config = config
        self.shm_frame_count = shm_frame_count
        // self.capture_thread = None
        self.ffmpeg_detect_process = None
        self.logpipe = LogPipe(f"ffmpeg.{self.camera_name}.detect")
        self.ffmpeg_other_processes: list[dict[str, any]] = []
        // self.camera_fps = camera_fps
        // self.skipped_fps = skipped_fps
        self.ffmpeg_pid = ffmpeg_pid
        // self.frame_queue = frame_queue
        self.frame_shape = self.config.frame_shape_yuv
        self.frame_size = self.frame_shape[0] * self.frame_shape[1]
        self.fps_overflow_count = 0
        self.frame_index = 0
        self.stop_event = stop_event
        self.sleeptime = self.config.ffmpeg.retry_interval

        self.config_subscriber = ConfigSubscriber(f"config/enabled/{camera_name}", True)
        self.was_enabled = self.config.enabled

*/


class CameraWatchdog : public QThread
{
    Q_OBJECT
    Q_PROPERTY(QString cameraName READ cameraName WRITE setCameraName NOTIFY cameraNameChanged FINAL)
    Q_PROPERTY(QSharedPointer<QThread> captureThread READ captureThread WRITE setCaptureThread NOTIFY captureThreadChanged FINAL)
    Q_PROPERTY(int cameraFPS READ cameraFPS WRITE setCameraFPS NOTIFY cameraFPSChanged FINAL)
    Q_PROPERTY(int skippedFPS READ skippedFPS WRITE setSkippedFPS NOTIFY skippedFPSChanged FINAL)
    Q_PROPERTY(QSharedPointer<SharedFrameBoundedQueue> inFrameQueue READ inFrameQueue WRITE setInFrameQueue NOTIFY inFrameQueueChanged FINAL)
    Q_PROPERTY(QSharedPointer<SharedFrameBoundedQueue> outFrameQueue READ outFrameQueue WRITE setOutFrameQueue NOTIFY outFrameQueueChanged FINAL)

public:
    explicit CameraWatchdog(const QString &cameraName,
                            const CameraConfig &config,
                            QSharedPointer<SharedFrameBoundedQueue> inFrameQueue,
                            QSharedPointer<SharedFrameBoundedQueue> outFrameQueue,
                            int cameraFPS,
                            int skippedFPS,
                            QObject *parent = nullptr);

    int cameraFPS() const;
    int skippedFPS() const;
    QString cameraName() const;
    QSharedPointer<QThread> captureThread() const;
    QSharedPointer<SharedFrameBoundedQueue> inFrameQueue() const;
    QSharedPointer<SharedFrameBoundedQueue> outFrameQueue() const;

public Q_SLOTS:
    void setCameraFPS(int newCameraFPS);
    void setSkippedFPS(int newSkippedFPS);
    void setCameraName(const QString &newCameraName);
    void setCaptureThread(QSharedPointer<QThread> newCaptureThread);
    void setInFrameQueue(QSharedPointer<SharedFrameBoundedQueue> newInFrameQueue);
    void setOutFrameQueue(QSharedPointer<SharedFrameBoundedQueue> newOutFrameQueue);

Q_SIGNALS:
    void cameraFPSChanged(int cameraFPS);
    void skippedFPSChanged(int skippedFPS);
    void cameraNameChanged(QString cameraName);
    void captureThreadChanged(QSharedPointer<QThread> captureThread);

    // QThread interface
    void inFrameQueueChanged(QSharedPointer<SharedFrameBoundedQueue> inFrameQueue);
    void outFrameQueueChanged(QSharedPointer<SharedFrameBoundedQueue> outFrameQueue);

protected:
    void run() override;

private:
    std::atomic_int m_cameraFPS;
    std::atomic_int m_skippedFPS;
    QString m_cameraName;
    QSharedPointer<QThread> m_captureThread;
    QSharedPointer<SharedFrameBoundedQueue> m_inFrameQueue;
    QSharedPointer<SharedFrameBoundedQueue> m_outFrameQueue;
};
