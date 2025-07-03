#pragma once

struct StationaryMaxFramesConfig {
    std::optional<int> default_max_frames;
    std::optional<std::map<std::string, int>> objects = {};
};

struct StationaryConfig {
    std::optional<int> interval;
    std::optional<int> threshold;
    std::optional<StationaryMaxFramesConfig> max_frames = StationaryMaxFramesConfig{};
};

struct DetectConfig {
    std::optional<bool> enabled = false;
    std::optional<int> height;
    std::optional<int> width;
    std::optional<int> fps = 5;
    std::optional<int> min_initialized;
    std::optional<int> max_disappeared;
    std::optional<StationaryConfig> stationary = StationaryConfig{};
    std::optional<int> annotation_offset = 0;
};
