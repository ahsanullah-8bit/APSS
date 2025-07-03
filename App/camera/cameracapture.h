#pragma once

#include <QThread>

#include "cameraconfig.h"
#include "camerametrics.h"
#include "utils/frame.h"

/*
        // self.name = f"capture:{config.name}"
        self.config = config
        self.shm_frame_count = shm_frame_count
        self.frame_index = frame_index
        self.frame_shape = frame_shape
        self.frame_queue = frame_queue
        // self.fps = fps
        self.stop_event = stop_event
        // self.skipped_fps = skipped_fps
        self.frame_manager = SharedMemoryFrameManager()
        self.ffmpeg_process = ffmpeg_process
        self.current_frame = mp.Value("d", 0.0)
        self.last_frame = 0
*/

class CameraCapture : public QThread
{
    Q_OBJECT
public:
    explicit CameraCapture(const QString &name,
                           SharedCameraMetrics metrics,
                           CameraConfig config,
                           QObject *parent = nullptr);
    QString name() const;

    // QThread interface
protected:
    void run() override;

private:
    QString m_name;
    CameraConfig m_config;
    QSharedPointer<CameraMetrics> m_metrics;
};
