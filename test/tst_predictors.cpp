#include <QObject>
#include <QDebug>

#include <gtest/gtest.h>

#include <opencv2/opencv.hpp>

#include "detectors/objectdetector.h"
#include "detectors/poseestimator.h"
#include "detectors/objectdetectorsession.h"
#include "config/modelconfig.h"

class TestPredictors : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        std::filesystem::create_directories("test/results");
    }

    static void TearDownTestSuite() {
        // optional cleanup
    }

    void SetUp() override {}

    void TearDown() override {
        // optional
    }
};

TEST_F(TestPredictors,  ObjectDetector) {

    cv::Mat img1 = cv::imread("test/assets/vehicles.jpg");
    cv::Mat img2 = cv::imread("test/assets/vehicles2.jpg");

    PredictorConfig config;
    config.model = ModelConfig();
    config.model->path = "models/yolo11n.onnx";
    config.batch_size = 1;

    auto start_time = std::chrono::high_resolution_clock::now();
    ObjectDetector detector(config);
    auto loading_time = std::chrono::high_resolution_clock::now() - start_time;

    start_time = std::chrono::high_resolution_clock::now();
    std::vector<PredictionList> predictions = detector.predict({ img1 });
    auto inference_time = std::chrono::high_resolution_clock::now() - start_time;

    detector.draw(img1, predictions[0], 0.2);

    ASSERT_TRUE(predictions.size() > 0);
    ASSERT_TRUE(predictions[0].size() > 0);

    std::string output_path = "test/results/obj_detector_tst.jpg";
    cv::imwrite(output_path, img1);
    qDebug() << "Results written to" << output_path;

    qDebug() << "Object Detector Loading:" << std::chrono::duration<double, std::milli>(loading_time);
    qDebug() << "Object Detector Inference:" << std::chrono::duration<double, std::milli>(inference_time);
}

TEST_F(TestPredictors, PoseEstimator) {
    cv::Mat img1 = cv::imread("test/assets/vehicles.jpg");
    cv::Mat img2 = cv::imread("test/assets/vehicles2.jpg");


    PredictorConfig config;
    config.model = ModelConfig();
    config.model->path = "models/yolo11n-pose-1700_320.onnx";

    auto start_time = std::chrono::high_resolution_clock::now();
    // Skeleton: top-left -> top-right -> bottom-right -> bottom-left -> top-left
    const std::vector<std::pair<int, int>> skeleton = {{0, 1}, {1, 2}, {2, 3}, {3, 0}};
    PoseEstimator estimator(config);
    estimator.setPoseSkeleton(skeleton);
    auto loading_time = std::chrono::high_resolution_clock::now() - start_time;

    start_time = std::chrono::high_resolution_clock::now();
    std::vector<PredictionList> predictions = estimator.predict({ img1 });
    auto inference_time = std::chrono::high_resolution_clock::now() - start_time;

    estimator.draw(img1, predictions[0], 0.2);

    ASSERT_TRUE(predictions.size() > 0);
    ASSERT_TRUE(predictions[0].size() > 0);

    std::string output_path = "test/resutls/pose_estimator_tst.jpg";
    cv::imwrite(output_path, img1);
    qDebug() << "Results written to" << output_path;

    qDebug() << "Pose Estimator Loading:" << std::chrono::duration<double, std::milli>(loading_time);
    qDebug() << "Pose Estimator Inference:" << std::chrono::duration<double, std::milli>(inference_time);
}
