#include <chrono>

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
#include "db/sqlite/recording-odb.hxx"

using namespace std::chrono;
using namespace std::chrono_literals;

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

QString VideoRecorder::path() const
{
    return m_path;
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
    for (const auto &[camera, _] : m_apssConfig.cameras) {
        auto *worker = new VideoRecorder;
        auto *thread = new QThread(this);

        worker->moveToThread(thread);
        connect(thread, &QThread::started, worker, &VideoRecorder::init);
        connect(worker, &VideoRecorder::destroyed, thread, &QThread::quit);
        connect(thread, &QThread::finished, thread, &QThread::deleteLater);

        m_recorderPool[QString::fromStdString(camera)] = Recorder { .recorder = worker,
                                                                  .thread = thread };
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

QFileInfo makeRecordingPath(const QDateTime &datetime, const QString &camera) {
    const QString yymmdd = datetime.toString("yyyy-MM-dd");
    const QString hh = datetime.toString("hh");
    const QString mmsszzz = datetime.toString("mm.ss.zzz");

    return QFileInfo(QString("%1/%2/%3/%4/%5.mkv")
                         .arg(RECORD_DIR.absolutePath(),
                              yymmdd, hh, camera, mmsszzz));
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
    Recorder &recorder = m_recorderPool[camera];

    const int max_limit = 2;
    const QDateTime &start_time = m_recorderPool[camera].startTime;
    const qint64 recording_diff = qAbs(start_time.secsTo(frame->timestamp())) / 60; // in minutes

    bool should_start = false; // start or restart
    if (!recorder.isRecording) {
        // start the recorder
        should_start = true;
        recorder.isRecording = true;
    } else if (activeEvents.empty()
               && start_time.isValid()
               && max_limit > 5
               && recording_diff > max_limit - 5) {     // that is 8.3% of max_limit (i.e. ~5mins of an hour)

        // TODO: This needs to be thoroughly tested
        // we reached the max video length
        // we should restart recording
        should_start = true;
        QMetaObject::invokeMethod(recorder.recorder, "stop");

        Recording recording;
        recording.setId(QString("%1_%2").arg(frame->camera(), recorder.startTime.toString(Qt::ISODateWithMs)));
        recording.setCamera(frame->camera());
        recording.setPath(recorder.recorder->path());
        recording.setStartTime(recorder.startTime);
        recording.setEndTime(frame->timestamp());
        recording.setDuration(static_cast<float>(start_time.msecsTo(frame->timestamp())));

        qDebug(output_recorder) << "Recording saved";
        // persist the recording to the database
        odb::transaction t(m_db->begin());
        try {
            m_db->persist(recording);
            t.commit();
        }
        catch (const odb::exception& e) {
            try { if (!t.finalized()) t.rollback(); } catch (...) {}
            qCCritical(output_recorder) << "DB Error:" << e.what();
        }
        catch (...) {
            try { if (!t.finalized()) t.rollback(); } catch (...) {}
            qCCritical(output_recorder) << "DB Error: Unknown/Uncaught exception occurred.";
        }
    }

    if (should_start) {
        const QFileInfo file_info = makeRecordingPath(frame->timestamp(), camera);
        file_info.dir().mkpath(".");
        QMetaObject::invokeMethod(recorder.recorder, "start", Qt::AutoConnection, Q_ARG(QString, file_info.filePath()));
        recorder.startTime = frame->timestamp();
    }

    // a backup, to initialize startTime.
    if (!recorder.startTime.isValid())
        recorder.startTime = frame->timestamp();

    // record the frame
    cv::Mat bgr = frame->data();
    QImage img(bgr.data, bgr.cols, bgr.rows, static_cast<int>(bgr.step), QImage::Format_BGR888);
    QMetaObject::invokeMethod(recorder.recorder, "recordFrame",
                              Qt::AutoConnection,
                              Q_ARG(QVideoFrame, QVideoFrame(img)));
}

#include "moc_recordingsmanager.cpp"
