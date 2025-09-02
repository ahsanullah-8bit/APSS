#pragma once

#include <QThread>

#include <config/cameraconfig.h>
#include <camera/camerametrics.h>
#include <output/ffmpegrecorder.h>

class CameraCapture : public QThread
{
    Q_OBJECT
public:
    explicit CameraCapture(const QString &name,
                           SharedCameraMetrics metrics,
                           CameraConfig config,
                           QObject *parent = nullptr);
    QString name() const;

signals:
    void packet(AVPacket *packet);

    // QThread interface
protected:
    void run() override;

private:
    QString m_name;
    CameraConfig m_config;
    PacketRingBuffer m_ringBuffer;
    QSharedPointer<CameraMetrics> m_metrics;
};
