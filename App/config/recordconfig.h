#pragma once

const std::string DEFAULT_TIME_LAPSE_FFMPEG_ARGS = "-vf setpts=0.04*PTS -r 30";

enum class RetainModeEnum { All, Motion, ActiveObjects };
enum class RecordQualityEnum { VeryLow, Low, Medium, High, VeryHigh };

struct RecordRetainConfig {
    std::optional<float> days = 0;
    std::optional<RetainModeEnum> mode = RetainModeEnum::All;
};

struct ReviewRetainConfig {
    std::optional<float> days = 10;
    std::optional<RetainModeEnum> mode = RetainModeEnum::Motion;
};

struct EventsConfig {
    std::optional<int> pre_capture = 5;
    std::optional<int> post_capture = 5;
    std::optional<ReviewRetainConfig> retain = {};
};

struct RecordPreviewConfig {
    std::optional<RecordQualityEnum> quality = RecordQualityEnum::Medium;
};

struct RecordExportConfig {
    std::optional<std::string> timelapse_args = DEFAULT_TIME_LAPSE_FFMPEG_ARGS;
};

struct RecordConfig {
    std::optional<bool> enabled = false;
    std::optional<bool> sync_recordings = false;
    std::optional<int> expire_interval = 60;
    std::optional<RecordRetainConfig> retain = RecordRetainConfig{};
    std::optional<EventsConfig> detections = EventsConfig{};
    std::optional<EventsConfig> alerts = EventsConfig{};
    std::optional<RecordExportConfig> exports = RecordExportConfig{};
    std::optional<RecordPreviewConfig> preview = RecordPreviewConfig{};
    std::optional<bool> enabled_in_config;
};
