#pragma once

#include "detectconfig.h"
#include "objectconfig.h"
#include "recordconfig.h"
#include "cameraffmpeg.h"

struct CameraConfig {
    std::optional<std::string> name;
    bool enabled = true;
    // audio
    // birdseye
    std::optional<DetectConfig> detect = DetectConfig();
    CameraFfmpegConfig ffmpeg;
    // genai
    // live
    // motion
    std::optional<ObjectConfig> objects = ObjectConfig();
    std::optional<RecordConfig> record = RecordConfig();
    // review
    // snapshots
    // timestamp_style

    std::optional<int> best_image_timeout = 60;
    std::optional<int> image_detect_timeout = 50;
    // mqtt
    // noticiations
    // onvif
    // ui
    // webui_url
    // zones
    std::optional<bool> enabled_in_config;

    // advanced
    std::optional<bool> pull_based_order = false;
};
