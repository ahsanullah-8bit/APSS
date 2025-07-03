#include <QObject>
#include <QDebug>
#include <QTest>

#include <opencv2/opencv.hpp>

#include "detectors/objectdetector.h"
#include "detectors/poseestimator.h"
#include "detectors/objectdetectorsession.h"
#include "detectors/yolodetection.h"
#include "utils/frame.h"

#include "config/modelconfig.h"


class TestDetectors : public QObject {
    Q_OBJECT
public:
    TestDetectors() = default;

private slots:
    void objectDetector();
    void objectDetectorSession();
    void utils();
    void poseEstimater();
};

void TestDetectors::objectDetector()
{
    // Images
    cv::Mat img1 = cv::imread("test/assets/vehicles2.jpg");

    // Config
    PredictorConfig config { "cpu",
                           ModelConfig {
                               "models/yolo11n.onnx",
                               ""
                           },
                           "models/yolo11n.onnx" };

    // Session
    ObjectDetector detector(config);
    // YOLODetection y_detector("models/yolo11n.onnx");
    std::vector<PredictionList> predictions = detector.predict({ img1 });
    // std::vector<PredictionList> predictions = y_detector.predict({ img1 });

    QVERIFY(!predictions.empty());
    QVERIFY(!predictions[0].empty());
}

void TestDetectors::objectDetectorSession()
{

}

void TestDetectors::utils()
{

}

void TestDetectors::poseEstimater()
{
    // Images
    cv::Mat img1 = cv::imread("test/assets/vehicles2.jpg");

    // Config
    PredictorConfig config { "cpu",
                           ModelConfig {
                               "models/yolo11n-pose-1700_320.onnx",
                               ""
                           },
                           "models/yolo11n-pose-1700_320.onnx" };

    // Session
    PoseEstimator estimator(config);
    std::vector<PredictionList> predictions = estimator.predict({ img1 });

    QVERIFY(!predictions.empty());
    QVERIFY(!predictions[0].empty());
}


QTEST_MAIN(TestDetectors)
#include "tst_predictors.moc"
