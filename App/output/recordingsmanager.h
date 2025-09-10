#pragma once

#include <QObject>
#include <QMediaCaptureSession>
#include <QMediaRecorder>
#include <QVideoFrameInput>

#include <opencv2/core/mat.hpp>
#include <opencv2/videoio.hpp>

#include <odb/sqlite/database.hxx>

#include <tbb_patched.h>
#include <camera/camerametrics.h>
#include <config/apssconfig.h>
#include <output/packetringbuffer.h>
#include <output/perobjectremuxer.h>
#include <utils/frame.h>

// manager
class RecordingsManager : public QObject
{
    Q_OBJECT
public:
    explicit RecordingsManager(const APSSConfig &config,
                               std::shared_ptr<odb::database> db,
                               const QHash<QString, SharedCameraMetrics> &cameraMetrics);

public slots:
    void init();
    void stop();
    void onRecordFrame(SharedFrame frame, const QList<int> &activeEvents);

private:
    struct Remuxer {
        PerObjectRemuxer *remuxer = nullptr;
        QThread *thread = nullptr;
        QDateTime startTime;
        bool isFree = true;
        int assignedTo = -1;
    };

    const APSSConfig &m_apssConfig;
    std::shared_ptr<odb::database> m_db;

    // recorder/remuxers
    const QHash<QString, SharedCameraMetrics> &m_cameraMetrics;
    QList<Remuxer> m_remuxerPool;
};

