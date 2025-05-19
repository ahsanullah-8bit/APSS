#include "lprsession.h"

#include <QDebug>

#include <filesystem>

#include "utils.h"

LPRSession::LPRSession(tbb::concurrent_bounded_queue<Frame> &objDetectedFrameQueue,
                       tbb::concurrent_bounded_queue<Frame> &lpDetectedFrameQueueOut,
                       const std::set<std::string> &filterClasses,
                       const std::string &modelPath,
                       int maxBatchSize,
                       float confThreshold,
                       float iouThreshold,
                       bool drawPredictions,
                       const std::string &labelsPath,
                       QObject *parent)
    : QThread(parent)
    , m_objDetectedFrameQueue(objDetectedFrameQueue)
    , m_lpDetectedFrameQueue(lpDetectedFrameQueueOut)
    , m_keyPointDetector(modelPath, labelsPath)
    , m_filterClasses(std::move(filterClasses))
    , m_maxBatchSize(maxBatchSize)
    , m_confThreshold(confThreshold)
    , m_iouThreshold(iouThreshold)
    , m_drawPredictions(drawPredictions)
{}

YOLOPose& LPRSession::detector() {
    return m_keyPointDetector;
}

void LPRSession::run() {
    try {
        while (!QThread::currentThread()->isInterruptionRequested()) {
            Frame frame;
            m_objDetectedFrameQueue.pop(frame);

            // Detect LPs for a single frame.
            // We only need no-copy Mats from each frame that that are one of the filtered classes
            const PredictionList &object_predictions = frame.predictions()[Prediction::Type::Objects];
            PredictionList filtered_vehicle_predictions;

            for (const auto& prediction : object_predictions) {
                if (m_filterClasses.contains(prediction.className) && prediction.hasDeltas)
                    filtered_vehicle_predictions.emplace_back(prediction);
            }

            MatList batch;
            PredictionList lp_predictions;
            for (size_t p = 0; p < filtered_vehicle_predictions.size(); ++p) {

                cv::Mat vehicle;
                const Prediction &vehicle_prediction = filtered_vehicle_predictions[p];
                Utils::crop(frame.data(), vehicle, vehicle_prediction.box);
                batch.emplace_back(vehicle);

                // Model doesn't support dynamic batch || Max batch size reached || No more predictions to complete the batch.
                if (!m_keyPointDetector.hasDynamicBatch() || batch.size() >= m_maxBatchSize || p + 1 >= filtered_vehicle_predictions.size()) {
                    // Detect LP
                    std::vector<PredictionList> results_list = m_keyPointDetector.predict(batch, true, MODEL_LP_CONFIDENDCE_THRESHOLD);

                    int vehicle_x = vehicle_prediction.box.x;
                    int vehicle_y = vehicle_prediction.box.y;
                    for (auto& results : results_list) {
                        // Go through each result, displace coordinates to the vehicle's location.
                        for (size_t pred_indx = 0; pred_indx < results.size(); ++pred_indx) {
                            Prediction& prediction = results[pred_indx];
                            // Displace LP box coordinates
                            prediction.box.x += vehicle_x;
                            prediction.box.y += vehicle_y;

                            // Displace LP keypoint coordinates
                            for (auto& point : prediction.points) {
                                point.x += vehicle_x;
                                point.y += vehicle_y;
                            }
                        }

                        // Save some License Plates
                        for (size_t m = 0; m < results.size(); ++m) {
                            cv::Mat lp_mat;
                            Utils::perspectiveCrop(vehicle, lp_mat, results[m].points);

                            // SCRIPTS_DIR/lps/<frame_id> - <prediction_indx>
                            const std::string img_dir = std::format("{}/{}", std::string(SCRIPTS_DIR), ".lps");
                            if (!std::filesystem::exists(img_dir))
                                std::filesystem::create_directories(img_dir);

                            std::string img_path = std::format("{}/{} - {}.jpg", img_dir,
                                                               std::string(frame.frameIdString().toStdString() + " - " + std::to_string(m + p)),
                                                               results[m].conf);
                            // cv::Mat frame_clone = frame.data().clone();
                            // m_keyPointDetector.drawPredictionsMask(frame_clone, results); // License Plate predictions aren't coming very good, see why is that happening.
                            // std::string vehicle_path = std::format("{}/{} - v.jpg", img_dir,
                            //                                        std::string(frame.frameId().toStdString() + " - " + std::to_string(m + p)));
                            cv::imwrite(img_path, lp_mat);
                            // cv::imwrite(vehicle_path, frame_clone);

                            frame.lp_paths.insert(0, QString::fromStdString(img_path));
                        }


                        lp_predictions.insert(lp_predictions.end(), results.begin(), results.end());
                    }

                    batch.clear();
                }
            }


            frame.setPredictions(Prediction::Type::LicensePlates, std::move(lp_predictions));
            m_lpDetectedFrameQueue.emplace(frame);

            // Recognize an LP
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

bool LPRSession::emitProcessedFrames() const
{
    return m_emitProcessedFrames.load();
}

void LPRSession::setEmitProcessedFrames(bool yes)
{
    m_emitProcessedFrames.store(yes);
}
