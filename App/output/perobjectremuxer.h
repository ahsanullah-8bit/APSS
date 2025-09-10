#pragma once

#include <QSharedPointer>

extern "C" {
#include <libavformat/avformat.h>
}

#include <output/packetringbuffer.h>

using SharedPacket = QSharedPointer<AVPacket>;

// Per-object remuxer worker (no re-encoding).
class PerObjectRemuxer : public QObject {
    Q_OBJECT
public:
    PerObjectRemuxer();
    ~PerObjectRemuxer();
    AVStream *outStream() const;
    QString path();

public slots:
    // Create an output file and copy the codec parameters from in_stream->codecpar
    bool openOutput(const QString &filename, const AVStream *inStream);
    bool writeHeader();
    // Write a packet that came from input stream; rescale timestamps to output timebase
    bool writePacket(QSharedPointer<AVPacket> pkt, AVRational inTb, AVRational outTb, int outStreamIndex);
    bool writePacket(QSharedPointer<AVPacket> pkt, AVRational inTb);
    bool writeCachedPackets(QSharedPointer<PacketRingBuffer> ringBuffer, AVRational inTimebase);
    void close();

private:
    AVFormatContext *m_oc = nullptr;
    AVStream *m_outStream = nullptr;
    bool m_headerWritten = false;
    QString m_path;
};
