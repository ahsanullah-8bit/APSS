#pragma once

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
