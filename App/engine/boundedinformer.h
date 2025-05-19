#pragma once

#include <QThread>
#include <QDebug>

#include <tbb_patched.h>

#include "frame.h"
#include "yolodetection.h"
#include "yolopose.h"

/*
 * @brief A Class that informs main thread of a frame update while listening to detector.
 *
*/
class BoundedInformer : public QThread {
    Q_OBJECT
public:
    BoundedInformer(tbb::concurrent_bounded_queue<Frame> &boundedFrameQueue,
                    YOLODetection &detector,
                    YOLOPose &poseEstimator,
                    QObject *parent = nullptr)
        : QThread(parent)
        , m_detector(detector)
        , m_poseEstimator(poseEstimator)
        , m_boundedFrameQueue(boundedFrameQueue)
    {}

signals:
    void frameChanged(const Frame &frame);

    // QThread interface
protected:
    void run() override {
        try {
            while (!QThread::currentThread()->isInterruptionRequested()) {
                Frame frame;
                m_boundedFrameQueue.pop(frame);
                cv::Mat mat = frame.data();

                m_detector.drawPredictionsMask(mat, frame.predictions()[Prediction::Type::Objects]);
                m_poseEstimator.drawPredictionsMask(mat, frame.predictions()[Prediction::Type::LicensePlates]);
                // qDebug() << "Size" << frame.predictions()[Prediction::Type::LicensePlates].size();

                emit frameChanged(std::move(frame));
            }
        }
        catch(const tbb::user_abort &e) {}
        catch(...) {
            qCritical() << "Uknown/Uncaught exception occurred.";
        }

        qInfo() << "Aborting on thread" << QThread::currentThread()->objectName();
    }

private:
    tbb::concurrent_bounded_queue<Frame> &m_boundedFrameQueue;
    YOLODetection &m_detector;
    YOLOPose &m_poseEstimator;
};
