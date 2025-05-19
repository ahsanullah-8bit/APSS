#pragma once

#include <QThread>
#include <QString>

#include <tbb_patched.h>
#include "frame.h"

class FFmpegFile : public QThread
{
    Q_OBJECT
    Q_PROPERTY(QString path READ path WRITE setPath NOTIFY pathChanged FINAL)
    Q_PROPERTY(QString id READ id FINAL)
public:
    explicit FFmpegFile(tbb::concurrent_bounded_queue<Frame> &frameQueue, const QString &path = QString(), QObject *parent = nullptr);
    QString path() const;
    void setPath(const QString &newPath);
    QString id() const;

signals:
    void pathChanged();

    // QThread interface
protected:
    void run() override;

private:
    QString m_path;
    QString m_id;
    tbb::concurrent_bounded_queue<Frame> &m_frameQueue;
};
