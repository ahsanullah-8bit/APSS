#pragma once

#include <rfl/Flatten.hpp>

struct FFmpegConfig {
    std::string path = "default";
    float retry_interval = 10.0f;
};

enum class CameraRoleEnum { Audio, Record, Detect };

struct CameraInput {
    std::string path;
    std::vector<CameraRoleEnum> roles;
};

struct CameraFfmpegConfig {
    // rfl::Flatten<FFmpegConfig> ffmpeg{};
    std::vector<CameraInput> inputs;

    bool validate_roles() const {
        std::set<CameraRoleEnum> seen_roles;
        for (const auto& input : inputs) {
            for (const auto& role : input.roles) {
                if (!seen_roles.insert(role).second)
                    return false; // duplicate role
            }
        }
        return seen_roles.count(CameraRoleEnum::Detect);
    }
};
