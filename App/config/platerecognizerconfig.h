#pragma once

#include <string>
#include <vector>
#include <optional>

struct Box {
    int xmin, ymin, xmax, ymax;
};

struct PlateProp {
    std::string value;
    double score;
};

struct RegionProp {
    std::string value;
    double score;
};

struct PlateInfo {
    std::string type;
    double score;
    Box box;
    struct Props {
        std::vector<PlateProp> plate;
        std::vector<RegionProp> region;
    } props;
};

struct VehicleProp {
    std::string value;
    double score;
};

struct MakeModel {
    std::string make;
    std::string model;
    double score;
};

struct VehicleInfo {
    std::string type;
    double score;
    Box box;
    struct Props {
        std::vector<MakeModel> make_model;
        std::vector<VehicleProp> orientation;
        std::vector<VehicleProp> color;
    };
    std::optional<Props> props;
};

struct Candidate {
    std::string plate;
    double score;
};

struct Region {
    std::string code;
    double score;
};

// combined
struct ANPRResult {
    std::variant<std::string, PlateInfo> plate;
    VehicleInfo vehicle;
    std::optional<int> direction;
    double direction_score;

    std::optional<Box> box;
    std::optional<Region> region;
    std::optional<double> score;
    std::optional<double> dscore;
    std::optional<std::vector<Candidate>> candidates;
    std::optional<std::vector<MakeModel>> model_make;
    std::optional<std::vector<VehicleProp>> color;
    std::optional<std::vector<VehicleProp>> orientation;
};

// combined
struct ANPRSnapshot {
    std::string filename;
    std::string timestamp;
    std::optional<std::string> camera_id;
    std::vector<ANPRResult> results;

    std::optional<double> processing_time;
    std::optional<int> version;
};
