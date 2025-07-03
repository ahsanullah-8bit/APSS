#pragma once

#include "modelconfig.h"
#include <rfl/Flatten.hpp>

struct PredictorConfig {
    std::optional<ModelConfig> model = ModelConfig{};
    std::optional<std::string> model_path;
    std::optional<std::string> ep = "CPUExecutionProvider"; // Execution provider
};

// struct ONNXInferenceConfig {
//     rfl::Flatten<PredictorConfig> predictor;
//     std::string ep = "CPUExecutionProvider"; // Execution provider
// };
