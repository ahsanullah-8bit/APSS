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

#include <camera/cameracapture.h>
#include <db/sqlite/recording-odb.hxx>
#include <output/recordingsmanager.h>

using namespace std::chrono;
using namespace std::chrono_literals;

// manager
Q_STATIC_LOGGING_CATEGORY(logger, "apss.output.rm")
RecordingsManager::RecordingsManager(const APSSConfig &config,
                                     std::shared_ptr<odb::database> db,
                                     const QHash<QString, SharedCameraMetrics> &cameraMetrics)
    : m_apssConfig(config)
    , m_db(db)
    , m_cameraMetrics(cameraMetrics)
{}

void RecordingsManager::init()
{
    m_remuxerPool.assign(m_apssConfig.cameras.size() * 5,
                         Remuxer { .remuxer = new PerObjectRemuxer,
                                 .thread = new QThread(this) });

    for (const auto &remuxer : std::as_const(m_remuxerPool)) {
        auto *worker = remuxer.remuxer;
        auto *thread = remuxer.thread;

        worker->moveToThread(thread);
        // connect(thread, &QThread::started, worker, &PerObjectRemuxer::init);
        connect(worker, &PerObjectRemuxer::destroyed, thread, &QThread::quit);
        connect(thread, &QThread::finished, thread, &QThread::deleteLater);

        thread->start();
    }
}

void RecordingsManager::stop()
{
    for (const auto &remuxer : std::as_const(m_remuxerPool)) {
        QThread *thread = remuxer.thread;

        QMetaObject::invokeMethod(remuxer.remuxer, "close", Qt::BlockingQueuedConnection);
        thread->quit();
        if (!thread->wait(1000)) {
            qCWarning(logger) << "Gracefull termination failed for" << thread->objectName() + "." << "forcing termination.";
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
        qCWarning(logger) << "Invalid frame id" << frame_id + "." << "Skipping!";
        return;
    }
    const auto &[camera, _] = id_parts.value();
    QSharedPointer<CameraCapture> camera_capture = m_cameraMetrics[camera]->captureThread().dynamicCast<CameraCapture>();

    for (int id : activeEvents) {
        auto rec_it = std::find_if(m_remuxerPool.begin(), m_remuxerPool.end(),
                                   [&id](const Remuxer &rem) { return rem.assignedTo == id; });
        if (rec_it == m_remuxerPool.end()) {
            // id was not assigned
            // look for a free recorder
            rec_it = std::find_if(m_remuxerPool.begin(), m_remuxerPool.end(),
                                   [] (const Remuxer &remuxer) { return remuxer.isFree; });
            if (rec_it == m_remuxerPool.end()) {
                // couldn't find a free recorder
                // spawn a new one
                auto *worker = new PerObjectRemuxer;
                auto *thread = new QThread(this);

                worker->moveToThread(thread);
                connect(worker, &PerObjectRemuxer::destroyed, thread, &QThread::quit);
                // connect(thread, &QThread::started, worker, &VideoRecorder::init);
                connect(thread, &QThread::finished, thread, &QThread::deleteLater);

                thread->start();
                m_remuxerPool.emplaceBack(worker, thread, frame->timestamp(), false, id);

                // take the recorder's iterator
                rec_it = std::prev(m_remuxerPool.end());
            } else {
                // found a free recorder
                rec_it->isFree = false;
                rec_it->assignedTo = id;
                rec_it->startTime = frame->timestamp();
            }

            // start the recorder
            const QFileInfo file_info = makeRecordingPath(frame->timestamp(), camera, id);
            file_info.dir().mkpath(".");
            QMetaObject::invokeMethod(rec_it->remuxer,
                                      "openOutput",
                                      Qt::AutoConnection,
                                      Q_ARG(QString, file_info.filePath()),
                                      Q_ARG(const AVStream*, camera_capture->inStream()));

            // write header
            QMetaObject::invokeMethod(rec_it->remuxer, "writeHeader");

            // send the cached packets
            // const QSharedPointer<PacketRingBuffer> ring_buffer = m_cameraMetrics[camera]->packetRingBuffer();
            // QMetaObject::invokeMethod(rec_it->remuxer,
            //                           "writeCachedPackets",
            //                           Qt::AutoConnection,
            //                           Q_ARG(QSharedPointer<PacketRingBuffer>, ring_buffer),
            //                           Q_ARG(AVRational, in_stream->time_base));
        }

        connect(camera_capture.get(), SIGNAL(packetChanged(QSharedPointer<AVPacket>,AVRational)), rec_it->remuxer, SLOT(writePacket(QSharedPointer<AVPacket>,AVRational)));
    }

    // stop remuxers for non active events
    for (auto it = m_remuxerPool.begin(); it != m_remuxerPool.end(); ++it) {
        const int id = it->assignedTo;

        if (!activeEvents.contains(id)) {
            QMetaObject::invokeMethod(it->remuxer, "close");
            Recording recording;
            recording.setId(QString("%1_%2").arg(frame->camera(), it->startTime.toString(Qt::ISODateWithMs)));
            recording.setCamera(frame->camera());
            recording.setPath(it->remuxer->path());
            recording.setStartTime(it->startTime);
            recording.setEndTime(frame->timestamp());
            recording.setDuration(static_cast<float>(it->startTime.msecsTo(frame->timestamp())));

            qDebug(logger) << "Recording saved";
            // persist the recording to the database
            // odb::transaction t(m_db->begin());
            // try {
            //     m_db->persist(recording);
            //     t.commit();
            // }
            // catch (const odb::exception& e) {
            //     try { if (!t.finalized()) t.rollback(); } catch (...) {}
            //     qCCritical(logger) << e.what();
            // }
            // catch (...) {
            //     try { if (!t.finalized()) t.rollback(); } catch (...) {}
            //     qCCritical(logger) << "Unknown/Uncaught exception occurred.";
            // }

            // reset the flags
            it->isFree = true;
            it->assignedTo = -1;
            connect(camera_capture.get(), SIGNAL(packetChanged(SharedPacket,AVRational)), it->remuxer, SLOT(writePacket(QSharedPointer<AVPacket>,AVRational)));
        }
    }
}

/*
    TODO
        Start time
        CameraCapture.inStream
        CameraMetrics.m_packetRing
*/

// // --- Main pipeline ---
// int main(int argc, char **argv) {
//     AVStream *in_video_stream = ifmt_ctx->streams[video_stream_index];
//     AVRational in_timebase = in_video_stream->time_base;
//     std::cout << "Input timebase: " << in_timebase.num << "/" << in_timebase.den << "\n";

//     // open decoder for detection (we decode only for detection)
//     const AVCodec *dec = avcodec_find_decoder(in_video_stream->codecpar->codec_id);
//     if (!dec) {
//         std::cerr << "Decoder not found\n";
//         return 1;
//     }
//     AVCodecContext *dec_ctx = avcodec_alloc_context3(dec);
//     avcodec_parameters_to_context(dec_ctx, in_video_stream->codecpar);
//     if (avcodec_open2(dec_ctx, dec, nullptr) < 0) {
//         std::cerr << "Failed to open decoder\n";
//         return 1;
//     }

//     // ring buffer for recent compressed packets
//     PacketRingBuffer ringbuf(2.0); // keep ~2s of packets

//     // map of active objectID -> remuxer
//     std::map<int, std::unique_ptr<PerObjectRemuxer>> active_remuxers;

//     FakeTracker tracker;

//     AVPacket pkt;
//     av_init_packet(&pkt);

//     // We will keep pumping packets until EOF. For each packet:
//     // - If it's video: push to ring buffer, forward to active remuxers (after rescale)
//     // - Also send video packets to decoder so we get decoded frames (maybe not for all packets,
//     //   but the example decodes so we can do detection logic per decoded frame)
//     //
//     // Simpler approach: decode every video packet into frames and run tracker per decoded frame.
//     // We still forward the compressed packet to remuxers for storage.

//     bool eof = false;
//     while (!eof) {
//         int ret = av_read_frame(ifmt_ctx, &pkt);
//         if (ret == AVERROR_EOF) {
//             eof = true;
//             // flush decoder by sending empty packet?
//             av_packet_unref(&pkt);
//         } else if (ret < 0) {
//             char buf[256];
//             av_strerror(ret, buf, sizeof(buf));
//             std::cerr << "read frame error: " << buf << "\n";
//             break;
//         } else {
//             // process packet
//             if (pkt.stream_index == video_stream_index) {
//                 // Push into ring buffer (we store packets for use when new object appears)
//                 ringbuf.push(&pkt, in_timebase);

//                 // Forward packet to every active remuxer (we will rescale pts/dts to the remuxer's out tb)
//                 for (auto &kv : active_remuxers) {
//                     PerObjectRemuxer *r = kv.second.get();
//                     if (!r->header_written) r->write_header(); // on-demand header
//                     // out_stream time_base equals input time_base in our open procedure, so rescale is trivial
//                     r->write_packet(&pkt, in_timebase, r->out_stream->time_base, r->out_stream->index);
//                 }

//                 // Send packet to decoder for detection (we could optimize by decoding only when needed)
//                 ret = avcodec_send_packet(dec_ctx, &pkt);
//                 if (ret < 0) {
//                     // non-fatal for this example
//                 } else {
//                     AVFrame *frame = av_frame_alloc();
//                     while (avcodec_receive_frame(dec_ctx, frame) == 0) {
//                         // Run tracker/detector on decoded frame -> returns list of active track IDs for that frame
//                         std::vector<int> active_ids = tracker.update_and_get_active(frame);

//                         // Add newly started tracks (active in this frame but not in active_remuxers)
//                         for (int id : active_ids) {
//                             if (active_remuxers.find(id) == active_remuxers.end()) {
//                                 // create remuxer
//                                 std::string filename = "object_" + std::to_string(id) + ".mp4";
//                                 auto r = std::make_unique<PerObjectRemuxer>(id);
//                                 if (!r->open_output(filename, in_video_stream)) {
//                                     std::cerr << "Failed to open output for id " << id << "\n";
//                                     continue;
//                                 }
//                                 // write header BEFORE writing packets? We do header on-demand.
//                                 // First: write buffered packets so file starts with a recent GOP (including keyframe)
//                                 // Extract buffer
//                                 auto buffered = ringbuf.extract_all();
//                                 // write header
//                                 if (!r->write_header()) {
//                                     std::cerr << "Failed to write header for id " << id << "\n";
//                                 } else {
//                                     // write buffered packets
//                                     for (AVPacket *bp : buffered) {
//                                         r->write_packet(bp, in_timebase, r->out_stream->time_base, r->out_stream->index);
//                                         av_packet_free(&bp);
//                                     }
//                                 }
//                                 // add to active remuxers
//                                 active_remuxers[id] = std::move(r);
//                                 std::cout << "Started remuxer for id " << id << " -> " << filename << "\n";
//                             }
//                         }

//                         // Remove tracks that are not active anymore
//                         // active_ids contains currently active ones; any remuxer not in active_ids should be closed.
//                         std::vector<int> to_remove;
//                         for (auto &kv : active_remuxers) {
//                             int id = kv.first;
//                             if (std::find(active_ids.begin(), active_ids.end(), id) == active_ids.end()) {
//                                 to_remove.push_back(id);
//                             }
//                         }
//                         for (int id : to_remove) {
//                             std::cout << "Stopping remuxer for id " << id << std::endl;
//                             active_remuxers[id]->close();
//                             active_remuxers.erase(id);
//                         }

//                         av_frame_unref(frame);
//                     }
//                     av_frame_free(&frame);
//                 }
//             } else {
//                 // non-video streams: optionally forward to remuxers if you want audio copied too.
//                 // For simplicity, this example only handles video.
//             }

//             av_packet_unref(&pkt);
//         }

//         // small sleep in example loop to avoid busy spin in fake demo (remove in real app)
//         // std::this_thread::sleep_for(1ms);
//     }

//     // EOF: close all remuxers
//     for (auto &kv : active_remuxers) {
//         kv.second->close();
//     }
//     active_remuxers.clear();

//     avcodec_free_context(&dec_ctx);
//     avformat_close_input(&ifmt_ctx);

//     avformat_network_deinit();
//     std::cout << "Finished.\n";
//     return 0;
// }

// void RecordingsManager::onRecordFrame(SharedFrame frame, const QList<int> &activeEvents)
// {

//     const QString frame_id = frame->id();
//     const auto &id_parts = Frame::splitFrameId(frame_id);
//     if (!id_parts) {
//         qCWarning(output_rec_mngr) << "Invalid frame id" << frame_id + "." << "Skipping!";
//         return;
//     }

//     const auto &[camera, _] = id_parts.value();
//     Recorder &recorder = m_recorderPool[camera];

//     const int max_limit = 2;
//     const QDateTime &start_time = m_recorderPool[camera].startTime;
//     const qint64 recording_diff = qAbs(start_time.secsTo(frame->timestamp())) / 60; // in minutes

//     bool should_start = false; // start or restart
//     if (!recorder.isRecording) {
//         // start the recorder
//         should_start = true;
//         recorder.isRecording = true;
//     } else if (activeEvents.empty()
//                && start_time.isValid()
//                && max_limit > 5
//                && recording_diff > max_limit - 5) {     // that is 8.3% of max_limit (i.e. ~5mins of an hour)

//         // TODO: This needs to be thoroughly tested
//         // we reached the max video length
//         // we should restart recording
//         should_start = true;
//         QMetaObject::invokeMethod(recorder.recorder, "stop");

//         Recording recording;
//         recording.setId(QString("%1_%2").arg(frame->camera(), recorder.startTime.toString(Qt::ISODateWithMs)));
//         recording.setCamera(frame->camera());
//         recording.setPath(recorder.recorder->path());
//         recording.setStartTime(recorder.startTime);
//         recording.setEndTime(frame->timestamp());
//         recording.setDuration(static_cast<float>(start_time.msecsTo(frame->timestamp())));

//         qDebug(output_recorder) << "Recording saved";
//         // persist the recording to the database
//         odb::transaction t(m_db->begin());
//         try {
//             m_db->persist(recording);
//             t.commit();
//         }
//         catch (const odb::exception& e) {
//             try { if (!t.finalized()) t.rollback(); } catch (...) {}
//             qCCritical(output_recorder) << "DB Error:" << e.what();
//         }
//         catch (...) {
//             try { if (!t.finalized()) t.rollback(); } catch (...) {}
//             qCCritical(output_recorder) << "DB Error: Unknown/Uncaught exception occurred.";
//         }
//     }

//     if (should_start) {
//         const QFileInfo file_info = makeRecordingPath(frame->timestamp(), camera);
//         file_info.dir().mkpath(".");
//         QMetaObject::invokeMethod(recorder.recorder, "start", Qt::AutoConnection, Q_ARG(QString, file_info.filePath()));
//         recorder.startTime = frame->timestamp();
//     }

//     // a backup, to initialize startTime.
//     if (!recorder.startTime.isValid())
//         recorder.startTime = frame->timestamp();

//     // record the frame
//     cv::Mat bgr = frame->data();
//     QImage img(bgr.data, bgr.cols, bgr.rows, static_cast<int>(bgr.step), QImage::Format_BGR888);
//     QMetaObject::invokeMethod(recorder.recorder, "recordFrame",
//                               Qt::AutoConnection,
//                               Q_ARG(QVideoFrame, QVideoFrame(img)));
// }

#include "moc_recordingsmanager.cpp"
