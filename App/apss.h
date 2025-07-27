#pragma once

#include <QDir.h>
#include <QUrl.h>
#include <vector>
#include <QString>
#include <QStandardPaths>

namespace cv {
class Mat;  // to avoid linking against the whole opencv, just for using apss.h
}

// Some common things APSS depends on

// Typedefs
using MatList = std::vector<cv::Mat>;

template <typename T>
using Vector3d = std::vector<std::vector<std::vector<T>>>;

// ZMQ
constexpr char SOCKET_SUB[] = "inproc://events/proxy_sub";
constexpr char SOCKET_PUB[] = "inproc://events/proxy_pub";

// Detection related
constexpr float DET_MIN_CONF = 0.4f;
constexpr float DET_MIN_IOU_THRESH = 0.4f;
constexpr float POSE_MIN_CONF = 0.4f;
constexpr float POSE_MIN_IOU_THRESH = 0.4f;
constexpr float OCR_MIN_CONF = 0.4f;
constexpr float OCR_MIN_IOU_THRESH = 0.4f;
constexpr float DET_RECONSIDER_AREA_INCREASE = 0.30f;   // Percentage, Reconsider sending a seen object to go through the pipeline again, if area is increase by the %.
constexpr int   TRACKER_DELTA_OBJECT_LIMIT = 40 * 24;   // 40 secs * 24 FPS, 960 ids at the moment
constexpr int   TRACKER_OBJECT_LOSS_LIMIT = 15;         // upto five frames

const QDir APSS_DIR(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
const QDir CONFIG_DIR =         APSS_DIR.cleanPath("config");
const QDir DEFAULT_DB_PATH =    CONFIG_DIR.cleanPath("apss.db");
const QDir MODEL_CACHE_DIR =    CONFIG_DIR.cleanPath("model_cache");
// const QDir BASE_DIR = (APSS_DIR.cleanPath("media/apss"));
const QDir EXPORT_DIR =         APSS_DIR.cleanPath("exports");
const QDir RECORD_DIR =         APSS_DIR.cleanPath("recordings");
const QDir CLIPS_DIR =          APSS_DIR.cleanPath("clips");
const QDir CLIPS_CACHE_DIR =    CLIPS_DIR.cleanPath("cache");
const QDir FACE_DIR =           CLIPS_DIR.cleanPath("faces");
const QDir THUMB_DIR =          CLIPS_DIR.cleanPath("thumbs");
const QDir CACHE_DIR =          APSS_DIR.cleanPath("tmp/cache");

// Attribute & Object constants

constexpr float LABEL_CONSOLIDATION_DEFAULT = 0.9f;
constexpr float LABEL_NMS_DEFAULT = 0.4f;
const std::map<std::string, float> LABEL_CONSOLIDATION_MAP = {
    {"car", 0.8},
    {"face", 0.5}
};

const std::map<std::string, float> LABEL_NMS_MAP = {
    {"car", 0.6}
};

// Audio constants

// AUDIO_DURATION = 0.975
// AUDIO_FORMAT = "s16le"
// AUDIO_MAX_BIT_RANGE = 32768.0
// AUDIO_SAMPLE_RATE = 16000
// AUDIO_MIN_CONFIDENCE = 0.5

// # DB constants

constexpr int MAX_WAL_SIZE = 10;  // MB

// # Regex constants

const QString REGEX_CAMERA_NAME = R"(^[a-zA-Z0-9_-]+$)";
const QString REGEX_RTSP_CAMERA_USER_PASS = R"(:\/\/[a-zA-Z0-9_-]+:[\S]+@)";
const QString REGEX_HTTP_CAMERA_USER_PASS = R"(user=[a-zA-Z0-9_-]+&password=[\S]+)";
// const QString REGEX_JSON = re.compile(r"^\s*\{");

// # Record Values

const QString CACHE_SEGMENT_FORMAT = "%Y%m%d%H%M%S%z";
constexpr int MAX_PRE_CAPTURE = 60;
constexpr int MAX_SEGMENT_DURATION = 600;
constexpr int MAX_SEGMENTS_IN_CACHE = 6;
constexpr int MAX_PLAYLIST_SECONDS = 7200;  // support 2 hour segments for a single playlist to account for cameras with inconsistent segment times
