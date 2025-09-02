#pragma once

#include <string>
#include <map>
#include <vector>
#include <optional>

enum class PixelFormatEnum { RGB, BGR, YUV };
enum class InputTensorEnum { NCHW, NHWC };
enum class InputDTypeEnum { FLOAT, INT };
enum class ModelTypeEnum { SSD, YOLOX, YOLOV9, YOLO11, YOLONAS, DFINE };

inline const std::map<std::string,std::vector<std::string>> DEFAULT_ATTRIBUTES_MAP = {
    {"person", {"amazon", "face"}},
    { "car", {
            "amazon",
            "an_post",
            "dhl",
            "dpd",
            "fedex",
            "gls",
            "license_plate",
            "nzpost",
            "postnl",
            "postnord",
            "purolator",
            "ups",
            "usps"}
    }
};

struct ModelConfig {
    std::optional<std::string> path = "models/yolo11n.onnx";
    std::optional<std::string> labelmap_path;
    std::optional<int> width = 320;
    std::optional<int> height = 320;
    std::optional<std::map<int,std::string>> labelmap = {};
    std::optional<std::map<std::string,std::vector<std::string>>> attributes_map = DEFAULT_ATTRIBUTES_MAP;
    std::optional<InputTensorEnum> input_tensor = InputTensorEnum::NHWC;
    std::optional<PixelFormatEnum> input_pixel_format = PixelFormatEnum::RGB;
    std::optional<InputDTypeEnum> input_dtype = InputDTypeEnum::FLOAT;
    std::optional<ModelTypeEnum> model_type = ModelTypeEnum::YOLO11;
};
