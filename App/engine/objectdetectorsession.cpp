#include "objectdetectorsession.h"

#include <QDebug>

#include <Eigen/Dense>
#include <BYTETracker.h>

#include <apss.h>

ObjectDetectorSession::ObjectDetectorSession(tbb::concurrent_bounded_queue<Frame> &frameQueue,
                                             tbb::concurrent_bounded_queue<Frame> &processedFrameQueue,
                                             const QString &modelPath,
                                             int maxBatchSize,
                                             float confThreshold,
                                             float iouThreshold,
                                             bool drawPredictions,
                                             const std::string &labelsPath,
                                             QObject *parent)
    : QThread(parent)
    , m_frameQueue(frameQueue)
    , m_processedFrameQueue(processedFrameQueue)
    , m_detector(modelPath.toStdString(), labelsPath)
    , m_maxBatchSize(maxBatchSize)
    , m_confThreshold(confThreshold)
    , m_iouThreshold(iouThreshold)
    , m_drawPredictions(drawPredictions)
{}

YOLODetection &ObjectDetectorSession::detector() {
    return m_detector;
}

void ObjectDetectorSession::run() {
    try {
        int tracker_buffer_size = 30;
        BYTETracker tracker(0.20f, tracker_buffer_size, 0.4f);

        // TODO: Find a proper way to hold seen ids.
        // Don't mistaken this for the tracker remembering objects. This one is for us to avoid going to another
        // stage for the same object in multiple frames. It focuses on whether it's a new
        // object or an old with increased quality/size/area. This usually helps in cases
        // of camera being mounted on the ground.
        QList<std::pair<size_t, int>> delta_objects(TRACKER_DELTA_OBJECT_LIMIT); // <id, area>

        // Two conditions can cause a finish interuption request or user_abort on the queue.
        // user_abort occurs if this thread is waiting and interruption occurs when the thread is still reading.
        while (!QThread::currentThread()->isInterruptionRequested()) {
            int batch_count = 0;
            MatList batch;
            std::vector<Frame> frames;
            do {
                Frame frame;
                m_frameQueue.pop(frame);
                frames.emplace_back(frame);
                batch.emplace_back(frames.back().data());
            } while (++batch_count < m_maxBatchSize && !m_frameQueue.empty());

            if (batch.empty())
                continue;

            std::vector<PredictionList> results_list = m_detector.predict(batch, MODEL_OBJECTS_CONFIDENDCE_THRESHOLD);

            // Push the results back to the processed queue, based on tracking results.
            // NOTE: This is a single tracker expecting frames from a single source.
            // TODO: Introduce some frameId system to separate/use multiple trackers based on the input frame stream.
            for (size_t l = 0; l < results_list.size(); ++l) {
                Frame frame = std::move(frames[l]);
                // PredictionList final_predictions;
                PredictionList &results = results_list.at(l);

                // Tracking code
                // Store results in a Nx5 matrix, row[xywh + conf]
                Eigen::MatrixXf e_predictions(results.size(), 5);
                for (size_t i = 0; i < results.size(); ++i) {
                    const cv::Rect &box = results.at(i).box;
                    e_predictions.row(i) <<
                        static_cast<float>(box.x),
                        static_cast<float>(box.y),
                        static_cast<float>(box.width),
                        static_cast<float>(box.height), results.at(i).conf;
                }

                // Boxes in eigen matrix, in top-left bottom-right format
                Eigen::MatrixXf e_tlbr_boxes(results.size(), 4);
                e_tlbr_boxes << e_predictions.col(0), e_predictions.col(1),
                    e_predictions.col(0) + e_predictions.col(2),
                    e_predictions.col(1) + e_predictions.col(3);

                // Some tracking results
                std::vector<KalmanBBoxTrack> tracks = tracker.process_frame_detections(e_predictions);
                std::vector<int> track_ids = match_detections_with_tracks(e_tlbr_boxes.cast<double>(), tracks);

                // Filter predictions based on track ids
                for (int i = 0; i < track_ids.size(); ++i) {
                    int id = track_ids.at(i);
                    if (id == -1)
                        continue;

                    results[i].trackerId = id;

                    int delta_indx = id % (TRACKER_DELTA_OBJECT_LIMIT);  // Resets when limit is reached
                    int box_area = results.at(i).box.area();

                    // We proceed if the id is not seen before or old area is less than the current (box is much bigger). Then
                    // we reconsider the other stages to re-process these predictions if they want to. i.e. LP was not visible, now is.
                    int area_increase = (delta_objects.at(delta_indx).second * DET_RECONSIDER_AREA_INCREASE);
                    if (delta_objects.at(delta_indx).first != id
                        || delta_objects.at(delta_indx).second + area_increase < box_area) {

                        delta_objects[delta_indx] = std::pair(id, box_area);
                        // Crop and save this one or whatever
                        results[i].hasDeltas = true;
                        // qDebug() << track_ids.at(i);
                    }
                }

                frame.setPredictions(Prediction::Type::Objects, std::move(results));
                m_processedFrameQueue.push(std::move(frame));
            }
        }
    }
    catch(const tbb::user_abort &e) {}
    catch(const std::exception &e) {
        qCritical() << e.what();
    }
    catch(...) {
        qCritical() << "Uknown/Uncaught exception occurred.";
    }

    qInfo() << "Aborting on thread" << QThread::currentThread()->objectName();
}
