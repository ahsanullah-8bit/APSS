#pragma once

#include <string>
#include <set>
#include <vector>
#include <map>
#include <optional>

struct LicensePlateConfig {
    bool enabled = false;
    float detection_threshold = 0.7;
    int min_area = 1000;
    float recognition_threshold = 0.9;
    int min_plate_length = 4;
    int match_distance = 1;
    std::optional<std::string> format;
    std::optional<std::map<std::string, std::vector<std::string>>> known_plates;
    // vehicles-of-interest
    std::optional<std::set<std::string>> voi = std::set<std::string>({ "bicycle", "car", "motorcycle", "bus", "truck" });
};
