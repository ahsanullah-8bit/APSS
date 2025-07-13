#pragma once

#include <string>
#include <vector>
#include <map>
#include <optional>

struct PPOCRGlobalConfig {
    std::string model_name;
};

struct PPOCRDynamicShapesConfig {
    std::vector<std::vector<int>> x;
};

struct PPOCRPaddleInferConfig {
    PPOCRDynamicShapesConfig trt_dynamic_shapes;
};

struct PPOCRTensorRTConfig {
    PPOCRDynamicShapesConfig dynamic_shapes;
};

struct PPOCRBackendConfigs {
    PPOCRPaddleInferConfig paddle_infer;
    PPOCRTensorRTConfig tensorrt;
};

struct PPOCRHpiConfig {
    PPOCRBackendConfigs backend_configs;
};

struct PPOCRTransformOpParametersConfig {
    std::optional<bool> channel_first;
    std::optional<std::string> img_mode;
    std::optional<std::string> gtc_encode;
    std::optional<std::vector<int>> image_shape;
    std::optional<std::vector<std::string>> keep_keys;
    std::optional<std::vector<int>> size;
    std::optional<int> channel_num;
    std::optional<std::vector<float>> mean;
    std::optional<std::string> order;
    std::optional<double> scale;
    std::optional<std::vector<float>> std;
    std::optional<int> resize_long;
};

struct PPOCRPreProcessConfig {
    std::vector<std::map<std::string, PPOCRTransformOpParametersConfig>> transform_ops;
};

struct PPOCRTopkConfig {
    int topk;
    std::vector<std::string> label_list;
};

struct PostProcess {
    std::optional<std::string> name;
    std::optional<std::vector<std::string>> character_dict;
    std::optional<double> thresh;
    std::optional<double> box_thresh;
    std::optional<int> max_candidates;
    std::optional<double> unclip_ratio;
    std::optional<PPOCRTopkConfig> Topk;
};

struct PaddleOCRConfig {
    PPOCRGlobalConfig Global;
    PPOCRHpiConfig Hpi;
    PPOCRPreProcessConfig PreProcess;
    PostProcess PostProcess;
};
