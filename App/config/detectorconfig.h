#pragma once

#include "modelconfig.h"

struct DetectorConfig {
    std::string type = "cpu";
    std::optional<ModelConfig> model;
    std::optional<std::string> model_path;
};
