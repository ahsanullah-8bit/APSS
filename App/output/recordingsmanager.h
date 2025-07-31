#pragma once

#include <QObject>
#include <QMediaCaptureSession>
#include <QMediaRecorder>
#include <QVideoFrameInput>

#include <opencv2/core/mat.hpp>
#include <opencv2/videoio.hpp>

#include <odb/sqlite/database.hxx>

#include "config/apssconfig.h"
#include "frame.h"
#include "tbb_patched.h"

class VideoRecorder : public QObject {
    Q_OBJECT
public:
    explicit VideoRecorder(QObject *parent = nullptr);

public slots:
    void init();
    void start(const QString &path);
    void stop();
    void recordFrame(const QVideoFrame &frame);

private:
    bool m_readyToSendAFrame = false;

    QString m_path;
    QMediaCaptureSession *m_captureSession;
    QMediaRecorder *m_recorder;
    QVideoFrameInput *m_videoFrameInput;
};

// manager
class RecordingsManager : public QObject
{
    Q_OBJECT
public:
    explicit RecordingsManager(const APSSConfig &config, std::shared_ptr<odb::database> db);

public slots:
    void init();
    void stop();
    void onRecordFrame(SharedFrame frame, const QList<int> &activeEvents);

private:
    struct Recorder {
        VideoRecorder *recorder = nullptr;
        QThread *thread = nullptr;
        bool isFree = true;
        int assignedTo = -1;
    };

    const APSSConfig &m_apssConfig;
    std::shared_ptr<odb::database> m_db;

    // isFree, <recorder, thread>
    QList<Recorder> m_recorderPool;
    // QHash<int, Recorder*> m_assignedRecorders;
    // recorder_indx, tracker_id
    // QHash<int, int> m_assignedRecorders;
    QHash<QString, bool> m_eventsToRecord;
};

