#include <QLoggingCategory>
#include <QMediaCaptureSession>
#include <QMediaRecorder>
#include <QVideoFrameInput>
#include <QMediaFormat>
#include <QImage>
#include <QVideoFrame>
#include <QThread>
#include <QJsonObject>

#include <opencv2/videoio/videoio.hpp>

#include "recordingsmanager.h"

// recorder
Q_STATIC_LOGGING_CATEGORY(output_recorder, "apss.output.povr")

VideoRecorder::VideoRecorder(QObject *parent)
    : QObject(parent)
{}

void VideoRecorder::init()
{
    m_captureSession = new QMediaCaptureSession(this);
    m_recorder = new QMediaRecorder(this);
    m_videoFrameInput = new QVideoFrameInput(this);

    QMediaFormat format;
    format.setFileFormat(QMediaFormat::Matroska);
    format.setVideoCodec(QMediaFormat::VideoCodec::H264);

    m_recorder->setMediaFormat(format);
    m_recorder->setQuality(QMediaRecorder::NormalQuality);
    m_recorder->setVideoFrameRate(0);
    m_recorder->setAutoStop(true);

    m_captureSession->setRecorder(m_recorder);
    m_captureSession->setVideoFrameInput(m_videoFrameInput);

    connect(m_videoFrameInput, &QVideoFrameInput::readyToSendVideoFrame, this, [this] () { m_readyToSendAFrame = true; });
    connect(m_recorder, &QMediaRecorder::errorOccurred, this, [this] (QMediaRecorder::Error error, const QString &errorString) {
        qCCritical(output_recorder) << "Failure for path" << m_path << errorString << error;
    });
}

void VideoRecorder::recordFrame(const QVideoFrame &frame)
{
    if (!m_readyToSendAFrame)
        return;

    m_videoFrameInput->sendVideoFrame(frame);
    m_readyToSendAFrame = false;
}

void VideoRecorder::start(const QString &path)
{
    if (m_recorder->recorderState() == QMediaRecorder::RecordingState) {
        qCCritical(output_recorder) << "Recorder was still running. Restarting it...";
        m_recorder->stop();
    }

    m_path = path;
    m_recorder->setOutputLocation(QUrl::fromLocalFile(m_path));
    m_recorder->record();
    qCDebug(output_recorder) << "Recording started for" << QUrl::fromLocalFile(m_path) << "and recorder is available:" << m_recorder->isAvailable();
}

void VideoRecorder::stop()
{
    qCDebug(output_recorder) << "Stopping a Recorder";
    recordFrame(QVideoFrame());
    m_recorder->stop();
}



// manager
Q_STATIC_LOGGING_CATEGORY(output_rec_mngr, "apss.output.rm")
RecordingsManager::RecordingsManager(const APSSConfig &config, std::shared_ptr<odb::database> db)
    : m_apssConfig(config)
    , m_db(db)
{}

void RecordingsManager::init()
{
    m_recorderPool.assign(m_apssConfig.cameras.size() * 5,
                          Recorder { .recorder = new VideoRecorder,
                                     .thread = new QThread(this) });

    for (const auto &recorder : std::as_const(m_recorderPool)) {
        auto *worker = recorder.recorder;
        auto *thread = recorder.thread;

        worker->moveToThread(thread);
        connect(thread, &QThread::started, worker, &VideoRecorder::init);
        connect(worker, &VideoRecorder::destroyed, thread, &QThread::quit);
        connect(thread, &QThread::finished, thread, &QThread::deleteLater);

        thread->start();
    }
}

void RecordingsManager::stop()
{
    for (const auto &recorder : std::as_const(m_recorderPool)) {
        QThread *thread = recorder.thread;

        QMetaObject::invokeMethod(recorder.recorder, "stop");
        thread->quit();
        if (!thread->wait(1000)) {
            qCWarning(output_rec_mngr) << "Gracefull termination failed for" << thread->objectName() + "." << "forcing termination.";
            thread->terminate();
            thread->wait();
        }
    }
}

QFileInfo makeRecordingPath(const QDateTime &datetime, const QString &camera, int trackerId) {
    const QString yymmdd = datetime.toString("yyyy-MM-dd");
    const QString hh = datetime.toString("hh");
    const QString mmsszzz = datetime.toString("mm.ss.zzz");

    return QFileInfo(QString("%1/%2/%3/%4/%5_%6.mkv")
                         .arg(RECORD_DIR.absolutePath(),
                              yymmdd, hh, camera, mmsszzz,
                              QString::number(trackerId)));
}

void RecordingsManager::onRecordFrame(SharedFrame frame, const QList<int> &activeEvents)
{
    const QString frame_id = frame->id();
    const auto &id_parts = Frame::splitFrameId(frame_id);
    if (!id_parts) {
        qCWarning(output_rec_mngr) << "Invalid frame id" << frame_id + "." << "Skipping!";
        return;
    }
    const auto &[camera, _] = id_parts.value();

    for (int id : activeEvents) {
        auto rec_it = std::find_if(m_recorderPool.begin(), m_recorderPool.end(), [&id](const Recorder &rec) { return rec.assignedTo == id; });
        if (rec_it == m_recorderPool.end()) {
            // id was not assigned
            // look for a free recorder
            auto it = std::find_if(m_recorderPool.begin(), m_recorderPool.end(), [] (const Recorder &recorder) { return recorder.isFree; });
            if (it == m_recorderPool.end()) {
                continue;
                // couldn't find a free recorder
                // spawn a new one
                auto *worker = new VideoRecorder;
                auto *thread = new QThread(this);

                worker->moveToThread(thread);
                connect(worker, &VideoRecorder::destroyed, thread, &QThread::quit);
                connect(thread, &QThread::started, worker, &VideoRecorder::init);
                connect(thread, &QThread::finished, thread, &QThread::deleteLater);

                m_recorderPool.emplaceBack(worker, thread, false, id);
                thread->start();

                // take the recorder's iterator
                it = m_recorderPool.begin() + (m_recorderPool.size() - 1);
            } else {
                // found a free recorder
                it->isFree = false;
                it->assignedTo = id;
            }

            // start the recorder
            const QFileInfo file_info = makeRecordingPath(frame->timestamp(), camera, id);
            file_info.dir().mkpath(".");
            QMetaObject::invokeMethod(it->recorder, "start", Qt::AutoConnection, Q_ARG(QString, file_info.filePath()));
            rec_it = it;
        }

        // push frame
        cv::Mat bgr = frame->data();
        QImage img(bgr.data, bgr.cols, bgr.rows, static_cast<int>(bgr.step), QImage::Format_BGR888);
        QMetaObject::invokeMethod(rec_it->recorder, "recordFrame",
                                  Qt::AutoConnection,
                                  Q_ARG(QVideoFrame, QVideoFrame(img)));
    }

    // stop recorders for non active events
    for (auto it = m_recorderPool.begin(); it != m_recorderPool.end(); ++it) {
        int id = it->assignedTo;

        if (!activeEvents.contains(id)) {
            QMetaObject::invokeMethod(it->recorder, "stop");
            // reset the flags
            it->isFree = true;
            it->assignedTo = -1;
        }
    }
}

#include "moc_recordingsmanager.cpp"
