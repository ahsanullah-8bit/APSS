#include <QString>

#include <gtest/gtest.h>
#include <opencv2/core.hpp>
#include "utils/framemanager.h"
#include "utils/frame.h"

using cv::Mat;

class TestFrameManager : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

// helper to create a simple Mat with distinct value
template <typename T>
inline Mat makeMat(int rows, int cols, T val) {
    Mat m(rows, cols, cv::DataType<T>::type);
    m.setTo(val);
    return m;
}

TEST_F(TestFrameManager, SingletonKeepsInitialCapacity) {
    auto &mgr1 = FrameManager::instance();
    auto &mgr2 = FrameManager::instance();

    QString cam = "camA";

    // set capacity
    mgr1.setMaxFramesPerCamera(cam, 3);

    // write at index 4 should wrap to index 4 % 3 = 1
    QString id_wrapped = Frame::makeFrameId(cam, 4);
    Mat img = makeMat<int>(2, 2, 42);
    mgr1.write(id_wrapped, img);

    auto result = mgr2.get(id_wrapped);
    ASSERT_TRUE(result.has_value());

    // check that the stored mat equals the original
    ASSERT_FALSE(result->empty());
    EXPECT_EQ(cv::countNonZero(*result != img), 0);
}

TEST_F(TestFrameManager, WriteAndGetReturnsSameMat) {
    auto &mgr = FrameManager::instance();

    QString cam = "camB";

    // set capacity
    mgr.setMaxFramesPerCamera(cam, 3);

    QString id = Frame::makeFrameId(cam, 2);
    Mat img = makeMat<float>(3, 1, 3.14f);

    mgr.write(id, img);
    auto got = mgr.get(id);
    ASSERT_TRUE(got.has_value());
    EXPECT_EQ(got->rows, img.rows);
    EXPECT_EQ(got->cols, img.cols);
    EXPECT_EQ(cv::countNonZero(*got != img), 0);
}

TEST_F(TestFrameManager, GetNonExistentAndInvalidIDs) {
    auto &mgr = FrameManager::instance();

    // non-existent camera
    auto no_cam = mgr.get(Frame::makeFrameId("unknownCam", 0));
    EXPECT_FALSE(no_cam.has_value());

    // invalid frame ID format (no underscore separator)
    auto invalid = mgr.get("badformatstring");
    EXPECT_FALSE(invalid.has_value());
}

TEST_F(TestFrameManager, RetireClearsFrame) {
    auto &mgr = FrameManager::instance();

    QString cam = "camC";
    // set capacity
    mgr.setMaxFramesPerCamera(cam, 3);

    QString id = Frame::makeFrameId(cam, 1);
    Mat img = makeMat<uchar>(4, 4, 255);

    mgr.write(id, img);
    auto before = mgr.get(id);
    ASSERT_TRUE(before.has_value());
    ASSERT_FALSE(before->empty());

    bool retired = mgr.retire(id);
    EXPECT_TRUE(retired);

    auto after = mgr.get(id);
    ASSERT_TRUE(after.has_value());
    EXPECT_TRUE(after->empty());
}

TEST_F(TestFrameManager, RetireNonExistentAndInvalidIDs) {
    auto &mgr = FrameManager::instance();

    // non-existent camera
    EXPECT_FALSE(mgr.retire(Frame::makeFrameId("ghostCam", 0)));

    // invalid ID format (no underscore separator)
    EXPECT_FALSE(mgr.retire("noUnderscore"));
}
