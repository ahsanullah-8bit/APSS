#pragma once

extern "C" {
#include <libavformat/avformat.h>
}

// Per-object remuxer worker (no re-encoding).
class PerObjectRemuxer : public QObject {
    Q_OBJECT
public:
    PerObjectRemuxer(int id_);
    ~PerObjectRemuxer();

public slots:
    // Create an output file and copy the codec parameters from in_stream->codecpar
    bool openOutput(const std::string &filename, AVStream *inStream);
    bool writeHeader();
    // Write a packet that came from input stream; rescale timestamps to output timebase
    bool writePacket(AVPacket *pkt, AVRational inTb, AVRational outTb, int outStreamIndex);
    void close();

private:
    int id = -1;
    AVFormatContext *m_oc = nullptr;
    AVStream *m_outStream = nullptr;
    bool m_headerWritten = false;
};
