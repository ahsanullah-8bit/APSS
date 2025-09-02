#include <filesystem>

#include <QtGui/QImage>
#include <QtMultimedia/QVideoFrame>

#include <gtest/gtest.h>
#include <opencv2/core/mat.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include "detectors/image.h"

// just to test some code, that bugs about optimization
class TestSamples : public ::testing::Test {
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

/**
 * Qt claims a QVideoFrame(QImage) will not create-a-copy/re-construct the whole QImage, if its color format matches.
 * where QVideoFormat has 4 channeled formats and I don't know if the A/X missing channel in the QImage still means
 * create-a-copy. Plus, OpenCV uses BGR and QImage has both. So, we'll try and see which one does it faster and also
 * if there is a copy when QVideoFrame(QImage).
*/
TEST_F(TestSamples, MatColorConversion)
{
    cv::Mat rgb = cv::imread("test/assets/vehicles.jpg");

    cv::cvtColor(rgb, rgb, cv::COLOR_BGR2RGB);
    QImage img(rgb.data, rgb.cols, rgb.rows, static_cast<int>(rgb.step), QImage::Format_RGB888);

    QVideoFrame videoframe(img);
}

TEST_F(TestSamples, MatWithoutConversion)
{
    cv::Mat rgb = cv::imread("test/assets/vehicles.jpg");

    QImage img(rgb.data, rgb.cols, rgb.rows, static_cast<int>(rgb.step), QImage::Format_BGR888);
    QVideoFrame videoframe(img);
}

TEST_F(TestSamples, QImageToVideoFrame)
{
    cv::Mat rgb = cv::imread("test/assets/vehicles.jpg");

    QImage img(rgb.data, rgb.cols, rgb.rows, static_cast<int>(rgb.step), QImage::Format_BGR888);

    // QImage to QVideoFrame
    auto start_time = std::chrono::high_resolution_clock::now();
    QVideoFrame videoframe(img);
    auto img2vf_time = std::chrono::high_resolution_clock::now() - start_time;

    start_time = std::chrono::high_resolution_clock::now();
    QImage qimg_again = videoframe.toImage();
    auto vf2qimg_time = std::chrono::high_resolution_clock::now() - start_time;

    qDebug() << "QImage to QVideoFrame: " << std::chrono::duration<double, std::milli>(img2vf_time);
    qDebug() << "QVideoFrame to QImage: " << std::chrono::duration<double, std::milli>(vf2qimg_time);
}

TEST_F(TestSamples, PerspectiveCropCentered)
{
    /*
     * 0.114484 0.675227 0.090712 0.059896
     * 0.06912799924612045 0.6452789902687073, 0.13488300144672394 0.6649739742279053, 0.15984000265598297 0.70517498254776, 0.09538699686527252 0.6859700083732605
    */

    cv::Mat img = cv::imread("test/assets/skewed_vehicle.jpg");
    const float sizeGain = 0.4f;
    std::vector<cv::Point2f> srcPoints = { {0.069127, 0.645278},
                                          {0.134883, 0.664973},
                                          {0.159840, 0.705174},
                                          {0.095386, 0.685970} };

    for (auto &point : srcPoints) {
        point.x = point.x * img.cols;
        point.y = point.y * img.rows;
    }

    cv::Mat res;
    Utils::perspectiveCrop(img, res, srcPoints);

    cv::imwrite("test/results/perspective_centered_crop.jpg", res);

    for (size_t i = 0; i < 4; i++) {
        int x = std::round(srcPoints[i].x);
        int y = std::round(srcPoints[i].y);
        cv::circle(img, cv::Point(x, y), 10, cv::Scalar(0,0,255), -1, cv::LINE_AA);
    }
    cv::imwrite("test/results/perspective_centered_src_drawn.jpg", img);
}
