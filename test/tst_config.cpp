#include <QObject>
#include <QFile>
#include <QTextStream>

#include <fstream>

#include <rfl.hpp>
#include <rfl/yaml.hpp>

#include <gtest/gtest.h>

#include "apssconfig.h"
#include "cameraconfig.h"
#include "databaseconfig.h"
#include "detectconfig.h"
#include "modelconfig.h"
#include "objectconfig.h"
#include "predictorconfig.h"
#include "recordconfig.h"

class TestConfig : public ::testing::Test {
protected:
    std::string m_pathPrefix = "test/config";

    static void SetUpTestSuite() {
        std::filesystem::create_directories("test/config");
    }

    static void TearDownTestSuite() {
        // optional cleanup
    }

    void SetUp() override {
        ASSERT_TRUE(std::filesystem::exists(m_pathPrefix));
    }

    void TearDown() override {
        // optional
    }
};

TEST_F(TestConfig, apssConfig) {
    // LOAD the default
    // auto maybe_config = rfl::yaml::read<APSSConfig>(DEFAULT_APSS_CONFIG);
    const std::string filepath = m_pathPrefix + "/tst_apss_config.yaml";

    CameraConfig cam_config;
    cam_config.enabled = true;
    cam_config.ffmpeg.inputs.emplace_back(CameraInput( "C:/Users/MadGuy/Videos/ny_street.mp4", {CameraRoleEnum::Detect}));

    PredictorConfig pred_config;
    pred_config.model = ModelConfig();
    pred_config.model->path = "models/yolo11n.onxx";

    APSSConfig config;
    config.version = "0.1";
    config.cameras["local_file"] = cam_config;
    config.predictors["yolo11_det"] = pred_config;

    std::string yaml_out = rfl::yaml::write(config);

    std::ofstream out_file(filepath);
    ASSERT_TRUE(out_file.is_open());
    out_file << yaml_out;
    out_file.close();

    ASSERT_TRUE(std::filesystem::exists(filepath));
    // const auto& def_config = maybe_config.value();
    // EXPECT_EQ(def_config.version, "0.1");
    // EXPECT_FALSE(def_config.cameras.empty());
}

TEST_F(TestConfig, cameraConfig)
{
    const std::string filepath = m_pathPrefix + "/tst_camera_config.yaml";

    CameraConfig camera_config;
    camera_config.name = "Front Door";
    camera_config.enabled = true;
    camera_config.best_image_timeout = 90;
    camera_config.enabled_in_config = std::nullopt;

    camera_config.detect->enabled = true;
    camera_config.detect->fps = 30;
    camera_config.detect->height = 1080;
    camera_config.detect->width = 1920;

    camera_config.objects->track = {"person", "car"};
    camera_config.objects->mask = std::vector<std::string>{"region_1", "region_2"};

    camera_config.record->enabled = true;
    camera_config.record->expire_interval = 120;
    camera_config.record->retain->days = 5;
    camera_config.record->retain->mode = RetainModeEnum::Motion;

    std::string yaml_out = rfl::yaml::write(camera_config);

    std::ofstream out_file(filepath);
    ASSERT_TRUE(out_file.is_open());
    out_file << yaml_out;
    out_file.close();

    ASSERT_TRUE(std::filesystem::exists(filepath));

    std::ifstream in_file(filepath);
    ASSERT_TRUE(in_file.is_open());

    std::string yaml_in((std::istreambuf_iterator<char>(in_file)),
                        std::istreambuf_iterator<char>());
    in_file.close();

    // For raw string comparison
    EXPECT_EQ(yaml_out, yaml_in);
}

TEST_F(TestConfig, databaseConfig)
{
    const std::string filepath = m_pathPrefix + "/tst_db_config.yaml";

    // Create and write config
    const auto db_config = DatabaseConfig{ .path = "path/to/the/database.db" };
    std::string yaml_conf_out = rfl::yaml::write(db_config);

    std::ofstream offstream(filepath);
    ASSERT_TRUE(offstream.is_open());
    offstream << yaml_conf_out;
    offstream.close();

    ASSERT_TRUE(std::filesystem::exists(filepath));

    // Read file contents
    std::ifstream instream(filepath);
    ASSERT_TRUE(instream.is_open());

    std::string yaml_conf_in;
    std::string buff;
    while (std::getline(instream, buff)) {
        yaml_conf_in += buff;
    }
    instream.close();

    EXPECT_EQ(yaml_conf_out, yaml_conf_in);

    // Deserialize and compare
    DatabaseConfig db_config_in = rfl::yaml::read<DatabaseConfig>(yaml_conf_in).value();
    EXPECT_EQ(db_config.path, db_config_in.path);
}

TEST_F(TestConfig, detectConfig)
{
    const std::string filepath = m_pathPrefix + "/tst_detect_config.yaml";

    // Construct test DetectConfig
    DetectConfig detect_config;
    detect_config.enabled = true;
    detect_config.height = 720;
    detect_config.width = 1280;
    detect_config.fps = 30;
    detect_config.min_initialized = 5;
    detect_config.max_disappeared = 50;
    detect_config.stationary->interval = 10;
    detect_config.stationary->threshold = 20;
    detect_config.stationary->max_frames->default_max_frames = 100;
    detect_config.stationary->max_frames->objects = {{"person", 200}, {"car", 300}};
    detect_config.annotation_offset = 5;

    // Serialize to YAML
    std::string yaml_out = rfl::yaml::write(detect_config);

    // Write to file
    std::ofstream out_file(filepath);
    ASSERT_TRUE(out_file.is_open());
    out_file << yaml_out;
    out_file.close();

    // Verify file existence
    ASSERT_TRUE(std::filesystem::exists(filepath));

    // Read YAML back
    std::ifstream in_file(filepath);
    ASSERT_TRUE(in_file.is_open());
    std::string yaml_in((std::istreambuf_iterator<char>(in_file)),
                        std::istreambuf_iterator<char>());
    in_file.close();

    // Compare serialized and read-back YAML strings
    EXPECT_EQ(yaml_out, yaml_in);
}

TEST_F(TestConfig, detectorConfig)
{
    const std::string filepath = m_pathPrefix + "/tst_detector_config.yaml";

    PredictorConfig predictor_config;
    // predictor_config.type = "gpu";
    predictor_config.model = ModelConfig();
    predictor_config.model->path = "models/detector.onnx";

    std::string yaml_out = rfl::yaml::write(predictor_config);

    std::ofstream out_file(filepath);
    ASSERT_TRUE(out_file.is_open());
    out_file << yaml_out;
    out_file.close();

    ASSERT_TRUE(std::filesystem::exists(filepath));

    std::ifstream in_file(filepath);
    ASSERT_TRUE(in_file.is_open());
    std::string yaml_in((std::istreambuf_iterator<char>(in_file)),
                        std::istreambuf_iterator<char>());
    in_file.close();

    EXPECT_EQ(yaml_out, yaml_in);
}

TEST_F(TestConfig, modelConfig)
{
    const std::string filepath = m_pathPrefix + "/tst_model_config.yaml";

    ModelConfig model_config;
    model_config.path = "models/ssd.onnx";
    model_config.labelmap_path = "models/labelmap.txt";
    model_config.width = 416;
    model_config.height = 416;
    model_config.labelmap = {{0,{"person"}}, {1,"car"}};
    model_config.attributes_map = std::map<std::string, std::vector<std::string>>{{"person", {"moving", "standing"}}, {"car", {"parked", "driving"}}};
    // model_config.input_tensor = model_config.inputTensorToString(InputTensorEnum::NCHW);
    // model_config.input_pixel_format = model_config.pixelFormatToString(PixelFormatEnum::BGR);
    // model_config.input_dtype = model_config.inputDTypeToString(InputDTypeEnum::FLOAT);
    // model_config.model_type = model_config.modelTypeToString(ModelTypeEnum::YOLOX);

    std::string yaml_out = rfl::yaml::write(model_config);

    std::ofstream out_file(filepath);
    ASSERT_TRUE(out_file.is_open());
    out_file << yaml_out;
    out_file.close();

    ASSERT_TRUE(std::filesystem::exists(filepath));

    std::ifstream in_file(filepath);
    ASSERT_TRUE(in_file.is_open());
    std::string yaml_in((std::istreambuf_iterator<char>(in_file)),
                        std::istreambuf_iterator<char>());
    in_file.close();

    EXPECT_EQ(yaml_out, yaml_in);
}

TEST_F(TestConfig, objectConfig)
{
    const std::string filepath = m_pathPrefix + "/tst_object_config.yaml";

    // Create test ObjectConfig
    ObjectConfig object_config;
    object_config.track = std::set<std::string>{"person", "car", "bike"};
    object_config.mask = std::vector<std::string>{"masked_region_1", "masked_region_2"};

    FilterConfig person_filter {
        .min_area = 100,
        .max_area = 50000,
        .threshold = 0.8,
        .raw_mask = "dynamic_mask_person"
    };
    FilterConfig car_filter {
        .min_area = 500,
        .max_area = 50000,
        .threshold = 0.8,
        .raw_mask = "dynamic_mask_car"
    };

    object_config.filters = {{"person", person_filter}, {"car", car_filter}};

    // Serialize to YAML
    std::string yaml_out = rfl::yaml::write(object_config);

    // Write to file
    std::ofstream out_file(filepath);
    ASSERT_TRUE(out_file.is_open());
    out_file << yaml_out;
    out_file.close();

    // Confirm file existence
    ASSERT_TRUE(std::filesystem::exists(filepath));

    // Read YAML back
    std::ifstream in_file(filepath);
    ASSERT_TRUE(in_file.is_open());
    std::string yaml_in((std::istreambuf_iterator<char>(in_file)),
                        std::istreambuf_iterator<char>());
    in_file.close();

    // Compare serialized YAML
    EXPECT_EQ(yaml_out, yaml_in);
}

TEST_F(TestConfig, recordConfig)
{
    const std::string filepath = m_pathPrefix + "/tst_record_config.yaml";

    // Create test RecordConfig
    RecordConfig record_config;
    record_config.enabled = true;
    record_config.sync_recordings = true;
    record_config.expire_interval = 120;
    record_config.retain->days = 7;
    record_config.retain->mode = RetainModeEnum::Motion;
    record_config.detections->pre_capture = 10;
    record_config.detections->post_capture = 15;
    record_config.alerts->pre_capture = 5;
    record_config.alerts->post_capture = 10;
    record_config.exports->timelapse_args = "-vf setpts=0.1*PTS -r 24";
    record_config.preview->quality = RecordQualityEnum::High;
    record_config.enabled_in_config = std::nullopt;

    // Serialize to YAML
    std::string yaml_out = rfl::yaml::write(record_config);

    // Write to file
    std::ofstream out_file(filepath);
    ASSERT_TRUE(out_file.is_open());
    out_file << yaml_out;
    out_file.close();

    // Confirm file existence
    ASSERT_TRUE(std::filesystem::exists(filepath));

    // Read YAML back
    std::ifstream in_file(filepath);
    ASSERT_TRUE(in_file.is_open());
    std::string yaml_in((std::istreambuf_iterator<char>(in_file)),
                        std::istreambuf_iterator<char>());
    in_file.close();

    // Compare serialized and deserialized YAML
    EXPECT_EQ(yaml_out, yaml_in);
}
