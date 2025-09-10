#include "packetringbuffer.h"

PacketRingBuffer::PacketRingBuffer(double durationLimitSec)
    : m_durationLimit(durationLimitSec) {}

PacketRingBuffer::~PacketRingBuffer() {
    for (AVPacket *p : m_buffer)
        av_packet_free(&p);
    m_buffer.clear();
}

void PacketRingBuffer::push(AVPacket *pkt, AVRational timeBase) {
    AVPacket *pclone = av_packet_clone(pkt);
    if (!pclone)
        return;

    // compute packet duration in seconds (approx)
    double dur = 0;
    if (pclone->duration > 0) {
        dur = (double)pclone->duration * av_q2d(timeBase);
    } else if (pclone->pts != AV_NOPTS_VALUE && m_lastPts != AV_NOPTS_VALUE) {
        dur = (double)(pclone->pts - m_lastPts) * av_q2d(timeBase);
        if (dur < 0)
            dur = 0;
    }
    m_lastPts = pclone->pts;

    std::unique_lock<std::shared_mutex> lock(m_mtx);
    m_buffer.emplace_back(pclone);
    m_totalSeconds += dur;
    // shrink until total_seconds <= duration_limit
    while (m_totalSeconds > m_durationLimit && m_buffer.size()) {
        AVPacket *t = m_buffer.front();
        // estimate duration of front
        double front_dur = 0;
        if (t->duration > 0)
            front_dur = (double)t->duration * av_q2d(timeBase);

        av_packet_free(&t);
        m_buffer.pop_front();
        m_totalSeconds -= front_dur;
    }
}

std::vector<AVPacket *> PacketRingBuffer::extractAll() {
    std::vector<AVPacket*> out;
    std::shared_lock<std::shared_mutex> lock(m_mtx);
    out.reserve(m_buffer.size());
    for (AVPacket *p : m_buffer) {
        AVPacket *c = av_packet_clone(p);
        if (c)
            out.push_back(c);
    }

    return out;
}
