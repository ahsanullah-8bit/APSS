#pragma once

#include <deque>
#include <shared_mutex>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
}

#include <tbb_patched.h>

// A small ring buffer of compressed packets (cloned) for immediate GOP rewind when a track starts.
// We store packets for the video stream only.
class PacketRingBuffer {
public:
    // duration_limit: maximum duration of packets to keep in seconds (approx).
    PacketRingBuffer(double durationLimitSec = 2.0);
    ~PacketRingBuffer();
    void push(AVPacket *pkt, AVRational timeBase);
    // Extract packet clones to give to new muxer.
    // Returns vector of AVPacket* (owned by caller).
    std::vector<AVPacket*> extractAll();

private:
    std::deque<AVPacket*> m_buffer;
    double m_durationLimit = 2.0;
    double m_totalSeconds = 0.0;
    int64_t m_lastPts = AV_NOPTS_VALUE;

    std::shared_mutex m_mtx;
};
