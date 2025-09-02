#include "perobjectremuxer.h"

#include <QLoggingCategory>

Q_STATIC_LOGGING_CATEGORY(logger, "apss.output.por");

PerObjectRemuxer::PerObjectRemuxer(int id_) : id(id_) {}

PerObjectRemuxer::~PerObjectRemuxer() {
    close();
}

bool PerObjectRemuxer::openOutput(const std::string &filename, AVStream *inStream) {
    int ret;
    avformat_alloc_output_context2(&m_oc, nullptr, nullptr, filename.c_str());
    if (!m_oc) {
        qCCritical(logger) << "Failed to alloc output context for " << filename;
        return false;
    }
    m_outStream = avformat_new_stream(m_oc, nullptr);
    if (!m_outStream) {
        qCCritical(logger) << "Failed to create out stream\n";
        return false;
    }
    // copy codec parameters
    ret = avcodec_parameters_copy(m_outStream->codecpar, inStream->codecpar);
    if (ret < 0) {
        qCCritical(logger) << "Failed to copy codecpar\n";
        return false;
    }
    m_outStream->time_base = inStream->time_base;

    if (!(m_oc->oformat->flags & AVFMT_NOFILE)) {
        ret = avio_open(&m_oc->pb, filename.c_str(), AVIO_FLAG_WRITE);
        if (ret < 0) {
            char buf[256]; av_strerror(ret, buf, sizeof(buf));
            qCCritical(logger) << "Could not open output file: " << buf << "\n";
            return false;
        }
    }
    // We'll write header on-demand right before the first packet (so we can set options if needed)
    return true;
}

bool PerObjectRemuxer::writeHeader() {
    if (m_headerWritten)
        return true;

    int ret = avformat_write_header(m_oc, nullptr);
    if (ret < 0) {
        char buf[256];
        av_strerror(ret, buf, sizeof(buf));
        qCCritical(logger) << "write_header failed: " << buf << "\n";
        return false;
    }

    m_headerWritten = true;
    return true;
}

bool PerObjectRemuxer::writePacket(AVPacket *pkt, AVRational inTb, AVRational outTb, int outStreamIndex) {
    // clone packet to be safe (av_interleaved_write_frame expects ownership)
    AVPacket *p = av_packet_clone(pkt);
    if (!p)
        return false;

    av_packet_rescale_ts(p, inTb, outTb);
    p->stream_index = outStreamIndex;

    int ret = av_interleaved_write_frame(m_oc, p);
    if (ret < 0) {
        char buf[256];
        av_strerror(ret, buf, sizeof(buf));
        qCCritical(logger) << "Failed to write packet for obj " << id << ": " << buf << "\n";
        av_packet_free(&p);
        return false;
    }
    av_packet_free(&p);
    return true;
}

void PerObjectRemuxer::close() {
    if (!m_oc)
        return;
    // flush and trailer
    av_write_trailer(m_oc);
    if (!(m_oc->oformat->flags & AVFMT_NOFILE))
        avio_closep(&m_oc->pb);

    avformat_free_context(m_oc);
    m_oc = nullptr;
    m_outStream = nullptr;
    m_headerWritten = false;
}
