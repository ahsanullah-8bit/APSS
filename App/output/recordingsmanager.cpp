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

    qDebug() << "Initialized recorder";
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



// manager
Q_STATIC_LOGGING_CATEGORY(output_rec_mngr, "apss.output.rm")
RecordingsManager::RecordingsManager(const APSSConfig &config, std::shared_ptr<odb::database> db)
    : m_apssConfig(config)
    , m_db(db)
{}

void RecordingsManager::init()
{
    for (const auto &[camera, _] : m_apssConfig.cameras) {
        auto *thread = new QThread(this);
        auto *worker = new VideoRecorder();

        worker->moveToThread(thread);
        connect(thread, &QThread::started, worker, &VideoRecorder::init);

        connect(worker, &VideoRecorder::destroyed, thread, &QThread::quit);
        connect(thread, &QThread::finished, thread, &QThread::deleteLater);

        const QString camera_ = QString::fromStdString(camera);
        m_recorderPool[camera_] = { worker, thread };

        thread->start();
        // start recording
        // qDebug() << "Emitting worker->start()";
        QMetaObject::invokeMethod(worker, "start",
                                  Qt::QueuedConnection,
                                  Q_ARG(QString, "test_rec.mkv"));
    }
}

void RecordingsManager::stop()
{
    for (const auto &[camera, item] : m_recorderPool.asKeyValueRange()) {
        auto [recorder, thread] = item;

        // emit recorder->recordFrame(QVideoFrame());
        QMetaObject::invokeMethod(recorder, "recordFrame",
                                  Qt::QueuedConnection,
                                  Q_ARG(QVideoFrame, QVideoFrame()));
        thread->quit();
        if (!thread->wait(1000)) {
            qCWarning(output_rec_mngr) << "Gracefull termination failed for" << thread->objectName() + "." << "forcing termination.";
            thread->terminate();
            thread->wait();
        }
    }
}

void RecordingsManager::onRecordFrame(SharedFrame frame)
{
    const auto &id_parts = Frame::splitFrameId(frame->id());
    if (!id_parts) {
        qCWarning(output_rec_mngr) << "Invalid frame id" << frame->id() + "." << "Skipping!";
        return;
    }

    const auto &[camera, frame_id] = id_parts.value();

    // see if we already have assigned a recorder
    if (!m_recorderPool.contains(camera)) {
        qCCritical(output_rec_mngr) << "Unexpected! Camera" << camera << "doesn't have a recorder. Skipping!";
        return;
    }

    cv::Mat bgr = frame->data();
    QImage img(bgr.data, bgr.cols, bgr.rows, static_cast<int>(bgr.step), QImage::Format_BGR888);

    auto [worker, _] = m_recorderPool[camera];
    QMetaObject::invokeMethod(worker, "recordFrame",
                              Qt::QueuedConnection,
                              Q_ARG(QVideoFrame, QVideoFrame(img)));

    // TODO: Save frame data to the database

    // odb::transaction t(m_db->begin());
    // try {

    //     // QList<Predictions> predictions_to_save;
    //     // for (const auto &prediction : frame->predictions()) {

    //     //     Predictions pred;
    //     //     pred.frame_id = frame->id();
    //     // }

    //     // m_db->persist(prediction);
    //     t.commit();
    // } catch (const odb::exception &e) {
    //     qCCritical(output_rec_mngr) << e.what();
    //     t.rollback();
    // }
}

#include "moc_recordingsmanager.cpp"


/*
    - We can store with timestamps
    - Then based on duration, we can determine detections.
*/
