#include "ffmpegcamera.h"

// FFmpeg headers
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/log.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
}

#include <QDebug>

#include <opencv2/core/mat.hpp>

#include <tbb_patched.h>

#include "frame.h"

FFmpegCamera::FFmpegCamera(tbb::concurrent_bounded_queue<Frame> &frameQueue, const QUrl &url, QObject *parent)
    : QThread{parent}
    , m_frameQueue(frameQueue)
    , m_url(url)
{}

QUrl FFmpegCamera::url() const
{
    return m_url;
}

void FFmpegCamera::setUrl(const QUrl &newUrl)
{
    if (m_url == newUrl)
        return;
    m_url = newUrl;
    emit urlChanged();
}

QString FFmpegCamera::id() const
{
    return m_id;
}

void FFmpegCamera::run()
{
    std::string url = m_url.path().toStdString();

    const char *rtsp_url = url.c_str();
    AVFormatContext *fmt_ctx = nullptr;
    AVCodecContext *video_codec_ctx = nullptr;
    AVStream *video_stream = nullptr;
    int video_stream_index = -1;
    AVFrame *frame = nullptr;
    AVFrame *rgb_frame = nullptr;
    AVPacket *packet = nullptr;
    SwsContext *sws_ctx = nullptr;

    try {
        // 1. Initialize FFmpeg
        av_log_set_level(AV_LOG_ERROR);
        avformat_network_init();

        // Open input stream
        if (avformat_open_input(&fmt_ctx, rtsp_url, nullptr, nullptr) < 0) {
            throw std::exception("Failed to open input stream");
        }

        // Retrieve stream information
        if (avformat_find_stream_info(fmt_ctx, nullptr) < 0) {
            throw std::exception("Failed to find stream info");
        }

        // 2. Find video stream
        for (unsigned int i = 0; i < fmt_ctx->nb_streams; ++i) {
            if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                video_stream_index = i;
                video_stream = fmt_ctx->streams[i];
                break;
            }
        }

        if (video_stream_index == -1) {
            throw std::exception("Could not find video stream");
        }

        // 3. Get stream and codec information
        const AVCodecParameters *codec_params = video_stream->codecpar;
        const AVCodec *video_codec = avcodec_find_decoder(codec_params->codec_id);
        if (!video_codec) {
            throw std::exception("Unsupported codec");
        }

        video_codec_ctx = avcodec_alloc_context3(video_codec);
        if (!video_codec_ctx) {
            throw std::exception("Failed to allocate codec context");
        }

        if (avcodec_parameters_to_context(video_codec_ctx, codec_params) < 0) {
            throw std::exception("Failed to copy codec parameters to codec context");
        }

        if (avcodec_open2(video_codec_ctx, video_codec, nullptr) < 0) {
            throw std::exception("Failed to open video codec");
        }

        // 4. Read Metadata
        qInfo() << "--- Format metadata ---";
        AVDictionaryEntry *tag = nullptr;
        while ((tag = av_dict_get(fmt_ctx->metadata, "", tag, AV_DICT_IGNORE_SUFFIX))) {
            qInfo() << tag->key << "=" << tag->value;
        }

        qInfo() << "--- Stream metadata ---";
        tag = nullptr;
        while ((tag = av_dict_get(video_stream->metadata, "", tag, AV_DICT_IGNORE_SUFFIX))) {
            qInfo() << tag->key << "=" << tag->value;
        }

        packet = av_packet_alloc();
        if (!packet) {
            throw std::exception("Failed to allocate packet");
        }

        // Getting the frames
        // Allocate frame buffer
        frame = av_frame_alloc();
        if (!frame) {
            throw std::exception("Failed to allocate frame");
        }

        rgb_frame = av_frame_alloc();
        if (!rgb_frame) {
            throw std::exception("Failed to allocate RGB frame");
        }

        // Initialize SwsContext for conversion to RGB
        sws_ctx = sws_getContext(
            video_codec_ctx->width, video_codec_ctx->height, video_codec_ctx->pix_fmt,
            video_codec_ctx->width, video_codec_ctx->height, AV_PIX_FMT_BGR24,
            SWS_BILINEAR, nullptr, nullptr, nullptr);
        if (!sws_ctx) {
            throw std::exception("Failed to initialize SwsContext");
        }

        int buffer_size = av_image_get_buffer_size(AV_PIX_FMT_BGR24, video_codec_ctx->width, video_codec_ctx->height, 1);
        uint8_t *buffer = static_cast<uint8_t *>(av_malloc(buffer_size));
        if (!buffer) {
            throw std::exception("Failed to allocate buffer for RGB frame");
        }
        av_image_fill_arrays(rgb_frame->data, rgb_frame->linesize, buffer, AV_PIX_FMT_BGR24, video_codec_ctx->width, video_codec_ctx->height, 1);

        // Read and Decode Frames
        size_t frame_index = 0;
        while (av_read_frame(fmt_ctx, packet) >= 0) {
            if (QThread::currentThread()->isInterruptionRequested())
                throw tbb::user_abort();

            if (packet->stream_index == video_stream_index) {
                // Send packet to the decoder
                if (avcodec_send_packet(video_codec_ctx, packet) < 0) {
                    qWarning() << "Error sending packet to decoder";
                    continue;
                }

                // Receive frame from decoder
                while (avcodec_receive_frame(video_codec_ctx, frame) >= 0) {
                    // 7. Convert Frame to OpenCV Format
                    sws_scale(sws_ctx, frame->data, frame->linesize, 0, video_codec_ctx->height,
                              rgb_frame->data, rgb_frame->linesize);

                    cv::Mat cv_frame(video_codec_ctx->height, video_codec_ctx->width, CV_8UC3, rgb_frame->data[0], rgb_frame->linesize[0]);
                    FrameId frame_id(m_id, QString::number(frame_index));
                    Frame final_frame(frame_id, cv_frame);

                    m_frameQueue.emplace(final_frame);

                    av_frame_unref(frame);
                }
            }
            av_packet_unref(packet);
        }
    } catch (const tbb::user_abort &) {
        // Nothing to do
    } catch (const std::exception &e) {
        qCritical() << e.what();
    }

    // Free resources
    av_packet_free(&packet);
    av_frame_free(&frame);
    av_frame_free(&rgb_frame);
    sws_freeContext(sws_ctx);
    if (video_codec_ctx) {
        avcodec_free_context(&video_codec_ctx);
    }
    if (fmt_ctx) {
        avformat_close_input(&fmt_ctx);
    }
    avformat_network_deinit();

    qInfo() << "Aborting on thread" << QThread::currentThread()->objectName();
}










