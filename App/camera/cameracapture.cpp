#include "cameracapture.h"

#include <QThread>
#include <QDebug>
#include <QLoggingCategory>

#include <opencv2/opencv.hpp>

#include <tbb_patched.h>
#include "utils/frame.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/log.h>
#include <libswscale/swscale.h>
}

Q_STATIC_LOGGING_CATEGORY(apss_camera_capture, "apss.camera.capture")

CameraCapture::CameraCapture(const QString &name,
                             SharedCameraMetrics metrics,
                             CameraConfig config,
                             QObject *parent)
    : QThread{parent}
    , m_name(name)
    , m_metrics(metrics)
    , m_config(config)
{
    setObjectName(name);
}

QString CameraCapture::name() const
{
    return m_name;
}

void CameraCapture::run()
{
    QSharedPointer<SharedFrameBoundedQueue> frame_queue = m_metrics->frameQueue();
    Q_ASSERT(frame_queue);

    // TODO: Do a proper settup of this
    std::string path = m_config.ffmpeg.inputs[0].path;
    const char *filename = path.c_str();
    AVFormatContext *fmt_ctx = nullptr;
    AVCodecContext *video_codec_ctx = nullptr;
    AVStream *video_stream = nullptr;
    int video_stream_index = -1;
    AVFrame *frame = nullptr;
    AVFrame *rgb_frame = nullptr;
    AVPacket *packet = nullptr;
    SwsContext *sws_ctx = nullptr;
    uint8_t *buffer = nullptr;

    int err_res = 0;
    char errbuf[AV_ERROR_MAX_STRING_SIZE];  // MSVC doesn't support compound literals: https://forum.lvgl.io/t/visual-studio-2019-compile-errors/1379/4

    try {
        // 1. Initialize FFmpeg
        av_log_set_level(AV_LOG_ERROR);
        avformat_network_init();

        // Allocate format context and set interrupt callback
        // fmt_ctx = avformat_alloc_context();
        // fmt_ctx->interrupt_callback.callback = [] (void *ctx) -> int { return QThread::currentThread()->isInterruptionRequested(); };

        // 2. Open input file
        if ((err_res = avformat_open_input(&fmt_ctx, filename, nullptr, nullptr)) < 0) {
            throw std::runtime_error(std::format("Failed to open input stream, {}, url {}", av_make_error_string(errbuf, sizeof(errbuf), err_res), filename));
        }

        if ((err_res = avformat_find_stream_info(fmt_ctx, nullptr)) < 0) {
            throw std::runtime_error(std::format("Failed to find stream info, {}", av_make_error_string(errbuf, sizeof(errbuf), err_res)));
        }

        // 3. Find video stream
        for (unsigned int i = 0; i < fmt_ctx->nb_streams; ++i) {
            if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                video_stream_index = i;
                video_stream = fmt_ctx->streams[i];
                break;
            }
        }

        if (video_stream_index == -1) {
            throw std::runtime_error("Could not find video stream");
        }

        // 4. Get codec context
        const AVCodecParameters *codec_params = video_stream->codecpar;
        const AVCodec *video_codec = avcodec_find_decoder(codec_params->codec_id);
        if (!video_codec) {
            throw std::runtime_error("Unsupported codec");
        }

        video_codec_ctx = avcodec_alloc_context3(video_codec);
        if (!video_codec_ctx) {
            throw std::runtime_error("Failed to allocate codec context");
        }

        if ((err_res = avcodec_parameters_to_context(video_codec_ctx, codec_params)) < 0) {
            throw std::runtime_error(std::format("Failed to copy codec parameters to codec context, {}", av_make_error_string(errbuf, sizeof(errbuf), err_res)));
        }

        if ((err_res = avcodec_open2(video_codec_ctx, video_codec, nullptr)) < 0) {
            throw std::runtime_error(std::format("Failed to open video codec, {}", av_make_error_string(errbuf, sizeof(errbuf), err_res)));
        }

        // 5. Read frames
        packet = av_packet_alloc();
        if (!packet) {
            throw std::runtime_error("Failed to allocate packet");
        }

        frame = av_frame_alloc();
        if (!frame) {
            throw std::runtime_error("Failed to allocate frame");
        }

        rgb_frame = av_frame_alloc();
        if (!rgb_frame) {
            throw std::runtime_error("Failed to allocate RGB frame");
        }

        sws_ctx = sws_getContext(video_codec_ctx->width, video_codec_ctx->height, video_codec_ctx->pix_fmt,
                                 video_codec_ctx->width, video_codec_ctx->height, AV_PIX_FMT_BGR24,
                                 SWS_BILINEAR, nullptr, nullptr, nullptr);
        if (!sws_ctx) {
            throw std::runtime_error("Failed to initialize SwsContext");
        }

        int buffer_size = av_image_get_buffer_size(AV_PIX_FMT_BGR24, video_codec_ctx->width, video_codec_ctx->height, 1);
        buffer = static_cast<uint8_t *>(av_malloc(buffer_size));
        if (!buffer) {
            throw std::runtime_error("Failed to allocate buffer for RGB frame");
        }
        av_image_fill_arrays(rgb_frame->data, rgb_frame->linesize, buffer, AV_PIX_FMT_BGR24, video_codec_ctx->width, video_codec_ctx->height, 1);

        // FPS syncronization
        // AVRational time_base = video_stream->time_base;
        // int64_t last_pts = AV_NOPTS_VALUE;  // presentation time
        // qint64 last_frame_time = 0;
        double avg_frame_duration = 0.0;

        // Calculate average FPS (fallback for missing PTS)
        double fps = av_q2d(av_guess_frame_rate(fmt_ctx, video_stream, nullptr));
        if (fps > 0.01)
            avg_frame_duration = 1000.0 / fps; // ms per frame

        // 6. Read and Decode Frames
        size_t frame_index = 0;
        while (!QThread::currentThread()->isInterruptionRequested()) {
            int read_result = av_read_frame(fmt_ctx, packet);
            // Check if interrupted by callback, break cleanly
            if (read_result == AVERROR_EXIT) {
                break;
            }

            // Check for normal EOF or error
            if (read_result < 0) {
                if (read_result != AVERROR_EOF) {
                    av_strerror(read_result, errbuf, sizeof(errbuf));
                    qCWarning(apss_camera_capture) << "Read error:" << errbuf;
                }
                break;
            }

            // Skip if stream index isn't the same
            if (packet->stream_index != video_stream_index)
                continue;

            // Send packet to the decoder
            int send_result = avcodec_send_packet(video_codec_ctx, packet);
            if (send_result < 0 && send_result != AVERROR(EAGAIN)) {
                av_strerror(send_result, errbuf, sizeof(errbuf));
                qCWarning(apss_camera_capture) << "Decoder error:" << errbuf;
                av_packet_unref(packet);
                continue;
            }

            // qInfo() << "Sent frame to decoder";

            // Receive frames from the decoder
            while (true) {
                qint64 ms_at_start = QDateTime::currentMSecsSinceEpoch();

                int recv_result = avcodec_receive_frame(video_codec_ctx, frame);
                if (recv_result == AVERROR(EAGAIN) || recv_result == AVERROR(EOF)) {
                    break;
                }

                if (recv_result < 0) {
                    av_strerror(recv_result, errbuf, sizeof(errbuf));
                    qCWarning(apss_camera_capture) << "Frame error:" << errbuf;
                    break;
                }

                // 7. Convert Frame to OpenCV Format
                sws_scale(sws_ctx, frame->data, frame->linesize, 0, video_codec_ctx->height,
                          rgb_frame->data, rgb_frame->linesize);

                cv::Mat cv_frame(video_codec_ctx->height, video_codec_ctx->width, CV_8UC3, rgb_frame->data[0], rgb_frame->linesize[0]);
                SharedFrame final_frame(new Frame(m_name, QString::number(frame_index), cv_frame.clone()));

                // Non-blocking queue insertion with abort detection
                try {
                    if (!m_metrics->isPullBased()) {
                        frame_queue->emplace(final_frame);
                    } else if (!frame_queue->try_emplace(final_frame)) {
                        // Queue full but not aborted - wait with timeout
                        if (QThread::currentThread()->isInterruptionRequested())
                            break;

                        qCWarning(apss_camera_capture) << "Queues overloaded, Skipping frame" << frame_index << "pts" << frame->pts;
                        // if (!m_frameQueue.wait_for(std::chrono::milliseconds(100))) {}   // error: no member named tbb::concurrent_bounded_queue::wait_for(..._
                    }
                } catch (const tbb::user_abort&) {
                    // Queue was aborted, exit immediately
                    break;
                }
                av_frame_unref(frame);

                // qInfo() << "Received frame from decoder";
                frame_index++;

                qint64 ms_frame_took = QDateTime::currentMSecsSinceEpoch() - ms_at_start;
                qint64 ms_pts_diff = avg_frame_duration - ms_frame_took;

                // qInfo() << "Avg frame duration" << avg_frame_duration << "time frame took" << ms_frame_took << "diff" << ms_to_sleep;
                if (ms_pts_diff > 10) // Because around 10ms are taken by the calculations and sleep/wake of the thread in such conditions
                    QThread::currentThread()->msleep(ms_pts_diff);
            }
            av_packet_unref(packet);

            // ms_frame_took = QDateTime::currentMSecsSinceEpoch() - ms_at_start;
            // ms_to_sleep = avg_frame_duration - ms_frame_took;
            // qInfo() << "Aft: Avg frame duration" << avg_frame_duration << "time frame took" << ms_frame_took << "diff" << ms_to_sleep;
            // Calculate PTS
            // double pts_ms = 0.0;
            // if (frame->best_effort_timestamp != AV_NOPTS_VALUE) {
            //     pts_ms = static_cast<qint64>(frame->best_effort_timestamp * av_q2d(time_base) * 1000);
            // } else if (last_pts != AV_NOPTS_VALUE) {
            //     pts_ms = last_pts + static_cast<qint64>(avg_frame_duration);
            // }

            // Frame syncronization
            // if (last_frame_time > 0) {
            //     const qint64 now = QDateTime::currentMSecsSinceEpoch();
            //     const qint64 elapsed = now - last_frame_time;
            //     const qint64 frame_duration = pts_ms - last_pts;

            //     if (frame_duration > elapsed) {
            //         // Sleep only if we're ahead of schedule
            //         const qint64 remaining = frame_duration - elapsed;
            //         QElapsedTimer timer;
            //         timer.start();
            //         while (!timer.hasExpired(remaining)) {
            //             if (QThread::currentThread()->isInterruptionRequested()) break;
            //             QThread::msleep(qMin(remaining - timer.elapsed(), 10LL));
            //         }
            //     }
            // }
            // last_pts = pts_ms;
            // last_frame_time = QDateTime::currentMSecsSinceEpoch();
        }

        // Flush the decoder for remaining frames
        avcodec_send_packet(video_codec_ctx, nullptr);
        while (avcodec_receive_frame(video_codec_ctx, frame) >= 0) {
            // Throw those frames away
            av_frame_unref(frame);
        }

    } catch (const tbb::user_abort &) {
        // Nothing to do
    } catch (const std::exception &e) {
        qCCritical(apss_camera_capture) << e.what();
    } catch (...) {
        qCCritical(apss_camera_capture) << "Uknown exception thrown at" << QThread().currentThread()->objectName() << "thread";
    }

    // Free the resources
    av_packet_free(&packet);
    av_frame_free(&frame);
    av_free(buffer);
    av_frame_free(&rgb_frame);
    sws_freeContext(sws_ctx);
    if (video_codec_ctx) {
        avcodec_free_context(&video_codec_ctx);
    }
    if (fmt_ctx) {
        avformat_close_input(&fmt_ctx);
    }
    avformat_network_deinit();

    qCInfo(apss_camera_capture) << "Aborting on thread" << QThread::currentThread()->objectName();
}

#include "moc_cameracapture.cpp"
