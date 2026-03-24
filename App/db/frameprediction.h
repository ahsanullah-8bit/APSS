#pragma once

// Hide includes from ODB_COMPILER
#ifndef ODB_COMPILER
#include <QtCore/QString>
#include <QtCore/QDateTime>
#endif

#pragma db model version(1, 1)

#pragma db object
class FramePrediction {
public:
    #pragma db id auto
    size_t id;
    QString frame_id;
    QDateTime video_timestamp;
    QDateTime stream_timestamp;
    QString data;
};
