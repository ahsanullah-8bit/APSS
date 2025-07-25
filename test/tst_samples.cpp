#include <QtGui/QImage>
#include <QtMultimedia/QVideoFrame>

#include <gtest/gtest.h>
#include <opencv2/core/mat.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc/imgproc.hpp>

// just to test some code, that bugs about optimization
class TestSamples : public ::testing::Test {};

/**
 * Qt claims a QVideoFrame(QImage) will not create-a-copy/re-construct the whole QImage, if its color format matches.
 * where QVideoFormat has 4 channeled formats and I don't know if the A/X missing channel in the QImage still means
 * create-a-copy. Plus, OpenCV uses BGR and QImage has both. So, we'll try and see which one does it faster and also
 * if there is a copy when QVideoFrame(QImage).
*/
TEST_F(TestSamples, MatColorConversion) {
    cv::Mat rgb = cv::imread("test/assets/vehicles.jpg");

    cv::cvtColor(rgb, rgb, cv::COLOR_BGR2RGB);
    QImage img(rgb.data, rgb.cols, rgb.rows, static_cast<int>(rgb.step), QImage::Format_RGB888);

    QVideoFrame videoframe(img);
}

TEST_F(TestSamples, MatWithoutConversion) {
    cv::Mat rgb = cv::imread("test/assets/vehicles.jpg");

    QImage img(rgb.data, rgb.cols, rgb.rows, static_cast<int>(rgb.step), QImage::Format_BGR888);
    QVideoFrame videoframe(img);
}
