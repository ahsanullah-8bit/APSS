#pragma once

#include "modelconfig.h"
#include <rfl/Flatten.hpp>

enum SupportedEP {
    CPU,
    OpenVINO,
    CUDA
};

struct PredictorConfig {
    std::optional<ModelConfig> model = ModelConfig{};
    std::optional<int> batch_size = 1;
    std::optional<std::vector<int>> kpt_shape = std::vector<int>{4, 3}; // for pose model
};

// struct ONNXInferenceConfig {
//     rfl::Flatten<PredictorConfig> predictor;
//     std::string ep = "CPUExecutionProvider"; // Execution provider
// };
