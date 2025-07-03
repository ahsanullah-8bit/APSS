#pragma once

#include <QThread>
#include <QDebug>

#include <tbb_patched.h>

#include "utils/frame.h"
#include "detectors/yolodetection.h"
#include "detectors/yolopose.h"

/*
 * @brief A Class that informs main thread of a frame update while listening to detector.
 *
*/
class BoundedInformer : public QThread {
    Q_OBJECT
public:
    BoundedInformer(SharedFrameBoundedQueue &boundedFrameQueue,
                    QObject *parent = nullptr)
        : QThread(parent)
        , m_boundedFrameQueue(boundedFrameQueue)
    {}

Q_SIGNALS:
    void frameChanged(SharedFrame frame);

    // QThread interface
protected:
    void run() override {
        try {
            while (!QThread::currentThread()->isInterruptionRequested()) {
                SharedFrame frame;
                m_boundedFrameQueue.pop(frame);
                cv::Mat mat = frame->data();

                // m_detector.drawPredictionsMask(mat, frame->predictions()[Prediction::Type::Objects]);
                // m_poseEstimator.drawPredictionsMask(mat, frame->predictions()[Prediction::Type::LicensePlates]);
                // qDebug() << "Size" << frame.predictions()[Prediction::Type::LicensePlates].size();

                Q_EMIT frameChanged(frame);
            }
        }
        catch(const tbb::user_abort &) {}
        catch(...) {
            qCritical() << "Uknown/Uncaught exception occurred.";
        }

        qInfo() << "Aborting on thread" << QThread::currentThread()->objectName();
    }

private:
    SharedFrameBoundedQueue &m_boundedFrameQueue;
    // YOLODetection &m_detector;
    // YOLOPose &m_poseEstimator;
};
