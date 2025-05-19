#pragma once

#include <QThread>
#include <QVideoSink>
#include <QVideoFrame>
#include <QHash>
#include <QQmlEngine>
#include <QFile>
#include <QCamera>
#include <QMediaDevices>

#include "filestream.h"
#include "ffmpegfile.h"
#include "lprsession.h"
#include "objectdetectorsession.h"
#include "boundedinformer.h"
#include "frame.h"

#include <tbb_patched.h>

// This class will handle most of the stuff
class APSSEngine : public QObject
{
    Q_OBJECT
    QML_SINGLETON
public:
    explicit APSSEngine(QObject *parent = nullptr);
    ~APSSEngine();
    Q_INVOKABLE void openAFootage(const QString& path, QVideoSink *sink = nullptr);
    Q_INVOKABLE void addRemoteCamera(const QString& path);
    Q_INVOKABLE void setCameraFeedSink(const QString& camera_id, QVideoSink *sink);
    Q_INVOKABLE QVideoSink* getCameraFeed(const QString& camera_id, QVideoSink *defaultValue = nullptr);
    Q_INVOKABLE QVideoSink* getCameraFeedByPath(const QString& path, QVideoSink *defaultValue = nullptr);
    Q_INVOKABLE QVideoSink* getCameraFeed(const QCamera &camera, QVideoSink *defaultValue = nullptr);
    Q_INVOKABLE void start();
    Q_INVOKABLE void stop();

public slots:
    void onFrameChanged(const Frame& frame);

private:
    FileStream m_fileStream;
    FFmpegFile m_ffmpegFileStream;
    ObjectDetectorSession m_objectDetector;
    LPRSession m_lprDetectorSession;
    BoundedInformer m_frameInformer;

    QHash<QString, QVideoSink*> m_cameraOutputFeeds;

    tbb::concurrent_bounded_queue<Frame> m_unProcessedFrameQueue;
    tbb::concurrent_bounded_queue<Frame> m_objDetectedFrameQueue;
    tbb::concurrent_bounded_queue<Frame> m_lpDetectedFrameQueue;
};
