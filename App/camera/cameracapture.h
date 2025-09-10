#pragma once

#include <QThread>

extern "C" {
#include <libavformat/avformat.h>
}

#include <config/cameraconfig.h>
#include <camera/camerametrics.h>

class CameraCapture : public QThread
{
    Q_OBJECT
public:
    explicit CameraCapture(const QString &name,
                           SharedCameraMetrics metrics,
                           CameraConfig config,
                           QObject *parent = nullptr);
    QString name() const;
    AVStream *inStream();

signals:
    void packetChanged(QSharedPointer<AVPacket> pkt, AVRational inTimeBase);

    // QThread interface
protected:
    void run() override;

private:
    QString m_name;
    CameraConfig m_config;
    QSharedPointer<CameraMetrics> m_metrics;

    AVStream *m_videoStream;
};
