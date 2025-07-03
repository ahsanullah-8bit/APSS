#pragma once

#include <optional>

#include "databaseconfig.h"
#include "licenseplateconfig.h"
#include "modelconfig.h"
#include "detectconfig.h"
#include "objectconfig.h"
#include "recordconfig.h"
#include "predictorconfig.h"
#include "cameraconfig.h"

inline std::string DEFAULT_APSS_CONFIG = R"(
version: 0.1
cameras:
  local_file:
    enabled: true
    ffmpeg:
      inputs:
        - path: C:/Users/MadGuy/Videos/ny_street.mp4
          roles:
            - Detect
    detect:
      enabled: false
      width: 1280
      height: 720
predictors:
  yolo11_det:
    model:
      path: models/yolo11n.onnx
    ep: CPUExecutionProvider
database:
  path: db/apss.sqlite3
)";


struct APSSConfig
{
    std::optional<std::string> version;
    std::map<std::string, CameraConfig> cameras;
    std::map<std::string, PredictorConfig> predictors;
    std::optional<DatabaseConfig> database;
    std::optional<ModelConfig> model = std::make_optional<ModelConfig>();
    std::optional<LicensePlateConfig> lpr = std::make_optional<LicensePlateConfig>();
};


/*
APSSConfig {
    version
    cameras
    detectors
    database
    model
    ffmpeg
}

Camera {
    name
    enabled
    detect
    objects
    motion
    record
    snapshots
    live
}
*/

/*
     * version
     * enviroment_vars
     * logger
     * auth
     * database
     * go2rtc
     * mqtt
     * notifications
     * proxy
     * telemetry
     * tls
     * classification
     * semantic_search
     * face_recognition
     * lpr
     * ui
     * detectors
     * model
     * cameras
     * audio
     * birdseye
     * detect
     * ffmpeg
     * genai
     * live
     * motion
     * objects
     * record
     * review
     * snapshots
     * timestamp_style
     * camera_groups
     * _plus_api
    */
