#pragma once

inline const std::vector<std::string> DEFAULT_TRACKED_OBJECTS = {"person"};

struct FilterConfig {
    std::optional<std::variant<int, float>> min_area = 0;
    std::optional<std::variant<int, float>> max_area = 24000000;
    std::optional<float> min_ratio = 0.0f;
    std::optional<float> max_ratio = 24000000.0f;
    std::optional<float> threshold = 0.7f;
    std::optional<float> min_score = 0.5f;
    std::optional<std::variant<std::string, std::vector<std::string>>> mask;
    std::optional<std::variant<std::string, std::vector<std::string>>> raw_mask = "";
};

struct ObjectConfig {
    std::optional<std::vector<std::string>> track = DEFAULT_TRACKED_OBJECTS;
    std::optional<std::map<std::string, FilterConfig>> filters = {};
    std::optional<std::variant<std::string, std::vector<std::string>>> mask = "";
};
