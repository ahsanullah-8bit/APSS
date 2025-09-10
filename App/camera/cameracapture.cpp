#include "cameracapture.h"

#include <QThread>
#include <QDebug>
#include <QLoggingCategory>

#include <opencv2/opencv.hpp>
#include <tbb_patched.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/log.h>
#include <libswscale/swscale.h>
}

#include <utils/eventspersecond.h>
#include <utils/frame.h>

Q_STATIC_LOGGING_CATEGORY(logger, "apss.camera.capture")

// Just an RAII for AVPacket unref
struct AVPacketUnrefRAII {
    AVPacket *packet = nullptr;

    ~AVPacketUnrefRAII() {
        if (packet)
            av_packet_unref(packet);
    }
};

// Just an RAII for AVFrame unref
struct AVFrameUnrefRAII {
    AVFrame *frame = nullptr;

    ~AVFrameUnrefRAII() {
        if (frame)
            av_frame_unref(frame);
    }
};

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

AVStream *CameraCapture::inStream()
{
    return m_videoStream;
}

void CameraCapture::run()
{
    QSharedPointer<SharedFrameBoundedQueue> frame_queue = m_metrics->frameQueue();
    Q_ASSERT(frame_queue);

    // TODO: Do a proper setup of this
    std::string path = m_config.ffmpeg.inputs[0].path;
    const char *filename = path.c_str();
    AVFormatContext *fmt_ctx = nullptr;
    AVCodecContext *video_codec_ctx = nullptr;
    AVStream *video_stream = nullptr;
    int video_stream_index = -1;
    AVFrame *frame = nullptr;
    AVFrame *bgr_frame = nullptr;
    AVPacket *packet = nullptr;
    SwsContext *sws_ctx = nullptr;
    uint8_t *buffer = nullptr;

    int err_res = 0;
    char errbuf[AV_ERROR_MAX_STRING_SIZE];  // MSVC doesn't support compound literals: https://forum.lvgl.io/t/visual-studio-2019-compile-errors/1379/4

    try {
        // 1. Initialize FFmpeg
        av_log_set_level(AV_LOG_WARNING);
        avformat_network_init();

        // 2. Open input file
        if ((err_res = avformat_open_input(&fmt_ctx, filename, nullptr, nullptr)) < 0) {
            throw std::runtime_error(std::format("Failed to open input stream, {}, url {}", av_make_error_string(errbuf, sizeof(errbuf), err_res), filename));
        }

        // Allocate format context and set interrupt callback
        // fmt_ctx = avformat_alloc_context();
        fmt_ctx->interrupt_callback.callback = [] (void *) -> int { return QThread::currentThread()->isInterruptionRequested(); };

        if ((err_res = avformat_find_stream_info(fmt_ctx, nullptr)) < 0) {
            throw std::runtime_error(std::format("Failed to find stream info, {}", av_make_error_string(errbuf, sizeof(errbuf), err_res)));
        }

        // 3. Find video stream
        for (unsigned int i = 0; i < fmt_ctx->nb_streams; ++i) {
            if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                video_stream_index = i;
                video_stream = fmt_ctx->streams[i];
                m_videoStream = video_stream;
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

        bgr_frame = av_frame_alloc();
        if (!bgr_frame) {
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
        av_image_fill_arrays(bgr_frame->data, bgr_frame->linesize, buffer, AV_PIX_FMT_BGR24, video_codec_ctx->width, video_codec_ctx->height, 1);

        // FPS syncronization
        AVRational time_base = video_stream->time_base;
        int64_t start_pts = AV_NOPTS_VALUE;  // presentation time
        auto start_wall = std::chrono::steady_clock::now();

        // 6. read and decode frames
        size_t frame_index = 0;
        EventsPerSecond eps_avg;
        eps_avg.start();
        while (!isInterruptionRequested()) {
            int read_result = av_read_frame(fmt_ctx, packet);
            if (read_result == AVERROR_EXIT) {
                // interrupted by callback, break cleanly
                break;
            }

            if (read_result == AVERROR_EOF) {
                // normal end of file (EOF)
                break;
            }

            if (read_result < 0) {
                // an error
                av_strerror(read_result, errbuf, sizeof(errbuf));
                qCFatal(logger) << "Read error:" << errbuf;
                break;
            }

            if (packet->stream_index != video_stream_index) {
                // stream index isn't the same
                continue;
            }

            // send packet to the decoder
            int send_result = avcodec_send_packet(video_codec_ctx, packet);
            if (send_result < 0 && send_result != AVERROR(EAGAIN)) {
                av_strerror(send_result, errbuf, sizeof(errbuf));
                qCWarning(logger) << "Decoder error:" << errbuf;
                continue;
            }

            while (!isInterruptionRequested()) {
                // receive frames from the decoder
                int recv_result = avcodec_receive_frame(video_codec_ctx, frame);

                if (recv_result == AVERROR_EOF) {
                    break;
                }

                if (recv_result == AVERROR(EAGAIN)) {
                    // decoding buffer just emptied
                    if (send_result == AVERROR(EAGAIN)) {
                        // and a packet is still waiting, to be pushed
                        avcodec_send_packet(video_codec_ctx, packet);
                        continue;
                    } else {
                        // break to acquire another packet
                        break;
                    }
                }

                if (recv_result < 0) {
                    av_strerror(recv_result, errbuf, sizeof(errbuf));
                    qCWarning(logger) << "Frame decoding error:" << errbuf;
                    break;
                }

                // synchronization
                int64_t pts = (frame->best_effort_timestamp != AV_NOPTS_VALUE)
                                  ? frame->best_effort_timestamp
                                  : frame->pts; // fallback if no pts
                if (start_pts == AV_NOPTS_VALUE) {
                    start_pts = pts;
                    start_wall = std::chrono::steady_clock::now();
                }

                double pts_ms = (pts - start_pts) * av_q2d(time_base) * 1000.0;
                // expected wall clock time for this frame
                auto target_time = start_wall + std::chrono::milliseconds((int64_t)pts_ms);
                // wait if we’re early
                auto now = std::chrono::steady_clock::now();
                if (target_time > now) {
                    std::this_thread::sleep_until(target_time);
                }
                // -- synchronization

                // 7. convert Frame to OpenCV Format
                sws_scale(sws_ctx, frame->data, frame->linesize, 0, video_codec_ctx->height,
                          bgr_frame->data, bgr_frame->linesize);

                cv::Mat cv_frame(video_codec_ctx->height, video_codec_ctx->width, CV_8UC3, bgr_frame->data[0], bgr_frame->linesize[0]);
                SharedFrame final_frame(new Frame(m_name, frame_index, cv_frame.clone()));

                QSharedPointer<AVPacket> pkt(av_packet_clone(packet), [](AVPacket *p) { av_packet_free(&p); });
                emit packetChanged(pkt, video_stream->time_base);

                try {
                    if (!m_metrics->isPullBased()) {
                        frame_queue->emplace(final_frame);
                    } else if (!frame_queue->try_emplace(final_frame)) {
                        qCWarning(logger) << m_config.name.value_or("") << "queues overloaded, Skipping frame" << frame_index << "fps" << eps_avg.eps();
                    }
                } catch (const tbb::user_abort&) {
                    // queue was aborted, exit immediately
                    break;
                }
                frame_index++;
                eps_avg.update();
                av_frame_unref(frame);
            }
        }

        // flush the decoder for remaining frames, by throwing them away
        avcodec_send_packet(video_codec_ctx, nullptr);
        while (avcodec_receive_frame(video_codec_ctx, frame) >= 0) {
            av_frame_unref(frame);
        }

    } catch (const tbb::user_abort &) {
        // Nothing to do
    } catch (const std::exception &e) {
        qCCritical(logger) << e.what();
    } catch (...) {
        qCCritical(logger) << "Uknown exception thrown at" << objectName() << "thread";
    }

    // Free the resources
    av_packet_free(&packet);
    av_frame_free(&frame);
    av_free(buffer);
    av_frame_free(&bgr_frame);
    sws_freeContext(sws_ctx);
    if (video_codec_ctx) {
        avcodec_free_context(&video_codec_ctx);
    }
    if (fmt_ctx) {
        avformat_close_input(&fmt_ctx);
    }
    avformat_network_deinit();

    qCInfo(logger) << "Aborting on thread" << objectName();
}

#include "moc_cameracapture.cpp"
