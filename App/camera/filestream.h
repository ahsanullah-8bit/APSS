#pragma once

#include <QThread>

#include <opencv2/core/mat.hpp>

#include <opencv2/videoio.hpp>
#include <tbb_patched.h>

#include <frame.h>

// A Frame producer class, produces frames from Local video files
class FileStream : public QThread
{
public:
    explicit FileStream(tbb::concurrent_bounded_queue<Frame> &sharedBuffer, const QString& filePath = QString(), QObject *parent = nullptr);
    void setFilePath(const QString& path);

    // QThread interface
    QString id() const;

protected:
    void run() override;

private:
    QString m_id;
    QString m_filePath;
    tbb::concurrent_bounded_queue<Frame> &m_sharedQueue;
    bool m_killOnEnd = false;
};
