#include "lpdetectorsession.h"

#include <QDebug>

#include "detectors/image.h"

LPDetectorSession::LPDetectorSession(SharedFrameBoundedQueue &inFrameQueue,
                                     QHash<QString, QSharedPointer<QWaitCondition>> &cameraWaitConditions,
                                     const PredictorConfig &config,
                                     const LicensePlateConfig &lpConfig,
                                     QObject *parent)
    : QThread(parent)
    , m_inFrameQueue(inFrameQueue)
    , m_cameraWaitConditions(cameraWaitConditions)
    , m_keyPointDetector(config)
    , m_config(config)
    , m_lpConfig(lpConfig)
{
    setObjectName("lp_detector");
}

PoseEstimator& LPDetectorSession::detector() {
    return m_keyPointDetector;
}

void LPDetectorSession::run() {
    // Skeleton: top-left -> top-right -> bottom-right -> bottom-left -> top-left
    static const std::vector<std::pair<int, int>> skeleton = {{0, 1}, {1, 2}, {2, 3}, {3, 0}};
    m_keyPointDetector.setPoseSkeleton(skeleton);

    qInfo() << "Starting" << objectName() << "thread";

    try {
        // vehicles-of-interest
        std::set<std::string> voi = { "car", "motorcycle",
                                      "bus", "truck" };
        if (m_lpConfig.voi)
            voi = m_lpConfig.voi.value_or(std::set<std::string>());

        int max_batch_size = 1;
        if (m_config.batch_size)
            max_batch_size = m_config.batch_size.value_or(1);

        m_eps.start();

        while (!QThread::currentThread()->isInterruptionRequested()) {
            SharedFrame frame;
            m_inFrameQueue.pop(frame);
            if (!frame || frame->hasExpired())
                continue;

            // Detect LPs for a single frame.
            // We only need no-copy Mats from each frame that that are one of the filtered classes
            const PredictionList &object_predictions = frame->predictions();
            PredictionList filtered_vehicle_predictions;

            for (const auto& prediction : object_predictions) {
                if (voi.contains(prediction.className) && prediction.hasDeltas)
                    filtered_vehicle_predictions.emplace_back(prediction);
            }

            MatList batch;
            PredictionList lp_predictions;
            for (size_t p = 0; p < filtered_vehicle_predictions.size(); ++p) {

                cv::Mat vehicle;
                const Prediction &vehicle_prediction = filtered_vehicle_predictions[p];
                Utils::crop(frame->data(), vehicle, vehicle_prediction.box);
                batch.emplace_back(vehicle);

                // Model doesn't support dynamic batch || Max batch size reached || No more predictions to complete the batch.
                if (!m_keyPointDetector.hasDynamicBatch() || batch.size() >= max_batch_size || p + 1 >= filtered_vehicle_predictions.size()) {
                    // Detect LP
                    std::vector<PredictionList> results_list = m_keyPointDetector.predict(batch);

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

                        results = filterLicensePlates(results);

                        lp_predictions.insert(lp_predictions.end(), results.begin(), results.end());
                    }

                    batch.clear();
                }
            }

            if (!frame || frame->hasExpired())
                continue;

            frame->addPredictions(std::move(lp_predictions));
            frame->setHasBeenProcessed(true);   // NOTE: This is very necessary to prevent CameraProcessor's prediction blocking, if finished very early.
            QString camera_name = frame->camera();
            Q_ASSERT(m_cameraWaitConditions.contains(camera_name));
            m_cameraWaitConditions.value(camera_name)->notify_all();    // Notify waiting camera processors.

            m_eps.update();
        }
    }
    catch(const tbb::user_abort &) {}
    catch(const std::exception &e) {
        qCritical() << e.what();
    }
    catch(...) {
        qCritical() << "Uknown/Uncaught exception occurred.";
    }

    qInfo() << "Stopping" << objectName() << "thread";
}

PredictionList LPDetectorSession::filterLicensePlates(const PredictionList &predictions)
{
    PredictionList results;
    for (const auto &prediction : predictions) {
        if (prediction.conf < m_lpConfig.detection_threshold)
            continue;

        results.emplace_back(std::move(prediction));
    }

    return results;
}

const EventsPerSecond &LPDetectorSession::eps() const
{
    return m_eps;
}

void LPDetectorSession::stop()
{
    try {
        if (isRunning()) {
            requestInterruption();

            qDebug() << "Waiting for" << objectName() << "to exit gracefully...";
            if (!wait(3000)) {
                qDebug() << objectName() << "didn't exit. Applying force killing...";
                terminate();
                wait();
            }
            qDebug() << objectName() << "thread has exited...";
        }
    } catch (const std::exception &e) {
        qDebug() << e.what();
    } catch (...) {
        qCritical() << "Uknown/Uncaught exception occurred!";
    }
}


// Debugging, Save some License Plates
// for (size_t m = 0; m < results.size(); ++m) {
//     cv::Mat lp_mat;
//     Utils::perspectiveCrop(vehicle, lp_mat, results[m].points);

//     // SCRIPTS_DIR/lps/<frame_id> - <prediction_indx>
//     const std::string img_dir = std::format("{}/{}", std::string(SCRIPTS_DIR), ".lps");
//     if (!std::filesystem::exists(img_dir))
//         std::filesystem::create_directories(img_dir);

//     std::string img_path = std::format("{}/{} - {}.jpg", img_dir,
//                                        std::string(frame->frameIdString().toStdString() + " - " + std::to_string(m + p)),
//                                        results[m].conf);
//     // cv::Mat frame_clone = frame.data().clone();
//     // m_keyPointDetector.drawPredictionsMask(frame_clone, results); // License Plate predictions aren't coming very good, see why is that happening.
//     // std::string vehicle_path = std::format("{}/{} - v.jpg", img_dir,
//     //                                        std::string(frame.frameId().toStdString() + " - " + std::to_string(m + p)));
//     cv::imwrite(img_path, lp_mat);
//     // cv::imwrite(vehicle_path, frame_clone);

//     frame->lp_paths.insert(0, QString::fromStdString(img_path));
// }
