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
    std::optional<std::string> model_path;
    std::optional<int> batch_size = 1;
};

// struct ONNXInferenceConfig {
//     rfl::Flatten<PredictorConfig> predictor;
//     std::string ep = "CPUExecutionProvider"; // Execution provider
// };
