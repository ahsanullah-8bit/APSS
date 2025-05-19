#pragma once

#include <QThread>
#include <QUrl>

#include <tbb_patched.h>

#include "frame.h"

class FFmpegCamera : public QThread
{
    Q_OBJECT
    Q_PROPERTY(QUrl url READ url WRITE setUrl NOTIFY urlChanged FINAL)
    Q_PROPERTY(QString id READ id FINAL)
public:
    explicit FFmpegCamera(tbb::concurrent_bounded_queue<Frame> &frameQueue, const QUrl &url = QUrl(), QObject *parent = nullptr);
    QUrl url() const;
    void setUrl(const QUrl &newUrl);
    QString id() const;

signals:
    void urlChanged();

    // QThread interface
protected:
    void run() override;

private:
    QUrl m_url;
    QString m_id;
    tbb::concurrent_bounded_queue<Frame> &m_frameQueue;
};
