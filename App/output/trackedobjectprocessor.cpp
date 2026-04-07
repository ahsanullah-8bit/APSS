#include <algorithm>
#include <exception>

#include <QLoggingCategory>
#include <qcontainerfwd.h>

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <odb/transaction.hxx>
#include <rfl/json/write.hpp>
#include <rfl/json.hpp>

#include <detectors/image.h>
#include "trackedobjectprocessor.h"
#include "apss.h"
#include "db/event.h"

Q_STATIC_LOGGING_CATEGORY(logger, "apss.engine.object_proc")

TrackedObjectProcessor::TrackedObjectProcessor(SharedFrameBoundedQueue &frameQueue,
                                               std::shared_ptr<odb::database> db,
                                               QObject *parent)
    : QThread{parent}
    , m_frameQueue(frameQueue)
    , m_db(db)
{
    setObjectName("tracked_object_processor");
}

void TrackedObjectProcessor::stop()
{
    try {
        if (isRunning()) {
            requestInterruption();

            qCDebug(logger) << "Waiting for" << objectName() << "to exit gracefully...";
            if (!wait(3000)) {
                qCDebug(logger) << objectName() << "didn't exit. Applying force killing...";
                terminate();
                wait();
            }
            qCDebug(logger) << objectName() << "thread has exited...";
        }
    } catch (const std::exception &e) {
        qCDebug(logger) << e.what();
    } catch (...) {
        qCCritical(logger) << "Uknown/Uncaught exception occurred!";
    }
}


void TrackedObjectProcessor::run()
{
    qCInfo(logger) << "Starting" << objectName() << "thread";

    QHash<QString, QHash<int, TrackedEvent>> cameras_history;

    try {
        while(!isInterruptionRequested()) {
            SharedFrame frame;
            m_frameQueue.pop(frame);
            if (!frame)
                continue;

            auto &events_history = cameras_history[frame->camera()];
            for (auto &event_history : events_history)
                event_history.lostCount++;

            const PredictionList& predictions = frame->predictions();
            const QDateTime frame_time = frame->timestamp();
            const cv::Mat frame_data = frame->data();

            // Process current predictions
            for (const Prediction& object : predictions) {
                if (object.trackerId < 0) 
                    continue;

                auto &event_history = events_history[object.trackerId];
                event_history.lostCount = 0;

                // Thumbnail
                if (event_history.bestThumbnail.counter < 2) {
                    auto &best_thumbnail = event_history.bestThumbnail;
                    const auto [crop_rect, is_smart_croppable] = getSmartCropRect(object.box, frame_data.size());

                    bool is_first_ever = best_thumbnail.img.empty();
                    bool improved_visibility = is_smart_croppable && !best_thumbnail.wasSmartCropped;
                    bool significantly_larger = (object.box.area() > event_history.lastObjectBoxArea * 1.2f);

                    if (is_first_ever || improved_visibility || (is_smart_croppable && significantly_larger)) {
                        best_thumbnail.img = frame_data(crop_rect).clone();
                        best_thumbnail.counter++;
                        best_thumbnail.wasSmartCropped = is_smart_croppable;
                        event_history.lastObjectBoxArea = object.box.area();
                        
                        // TODO: This should be moved to a separate thread.
                        event_history.event.setThumbnail(THUMB_DIR.filePath(QString("%1_%2.jpg").arg(frame->camera()).arg(object.trackerId)));  // Dragons, I KNOW!
                        cv::imwrite(event_history.event.thumbnail().toStdString(), event_history.bestThumbnail.img);

                        if (event_history.isPersisted)
                            emit eventUpdated(event_history.id, EventThumbnail);
                    }
                }

                // Plate
                if (object.subPredictions) {
                    auto &best_plate = event_history.bestPlate;
                    for (const auto &plate : object.subPredictions.value()) {
                        if (plate.className != "license_plate")
                            continue;

                        float intersection_area = (plate.box & object.box).area();
                        if (intersection_area / plate.box.area() < 0.95) {
                            // Plate should be at least 95% inside the vehicle's box.
                            // This step is necessary to avoid adding up plates 
                            // of other vehicles, although some plates still 
                            // may come inside the vehicle's bounding box.
                            continue;
                        }

                        if (best_plate.empty() || plate.box.area() > best_plate.total() * 1.2) {
                            // This would be a single plate anyway. But this check is required for the more than one plate case.
                            Utils::perspectiveCrop(frame_data, best_plate, plate.points);

                            // TODO: This should be moved to a separate thread.
                            if (!best_plate.empty())
                                cv::imwrite(THUMB_DIR.filePath(QString("%1_%2_lp.jpg").arg(frame->camera()).arg(object.trackerId)).toStdString(), event_history.bestPlate);

                            if (event_history.isPersisted)
                                emit eventUpdated(event_history.id, EventPlate);
                        }
                    }
                }

                if (!event_history.isPersisted) {
                    // Event
                    Event &event = event_history.event;
                    event.setLabel(QString::fromStdString(object.className));
                    event.setCamera(frame->camera());
                    event.setStartTime(frame_time);
                    event.setTopScore(object.conf);
                    event.setTrackerId(object.trackerId);
                    events_history[object.trackerId].predictions.push_back(object); // First Prediction

                    try {
                        odb::transaction t(m_db->begin());
                        event_history.id = m_db->persist(event);
                        t.commit();
    
                        event_history.isPersisted = true;

                        emit eventPersisted(event_history.id);
                    } catch (const std::exception &e) {
                        qCCritical(logger) << "Error updating db event" << QString("(%1),").arg(event.id()) << e.what();
                    }
                } else {
                    // Update existing event
                    event_history.predictions.push_back(object);
                    
                    auto &event = events_history[object.trackerId].event;
                    event.setEndTime(frame_time);
                    if (object.conf > event.topScore())
                        event.setTopScore(object.conf);

                    // TODO: Do a proper average of all the scores
                    event.setScore(object.conf);
                    
                    // emit eventUpdated(event_history.id);
                }
            }

            // Remove lost
            for (auto it = events_history.begin(); it != events_history.end();) {
                if (it->lostCount > TRACK_MAX_EVENTS) {
                    const auto &history = events_history[it.key()];

                    // TODO: Persist events if they're not.
                    Event event;

                    try {
                        odb::transaction t(m_db->begin());
                        m_db->load(it->id, event);
    
                        event.setEndTime(history.event.endTime());
                        event.setTopScore(history.event.topScore());
                        event.setScore(history.event.score());
                        event.setData(QString::fromStdString(rfl::json::write(history.predictions)));
    
                        m_db->update(event);
                        t.commit();

                        emit eventUpdated(it->id, EventEndTime | EventTopScore | EventScore | EventData);
                    } catch (const std::exception &e) {
                        qCCritical(logger) << "Error updating db event" << QString("(%1),").arg(event.id()) << e.what();
                    }

                    it = events_history.erase(it);
                } else {
                    ++it;
                }
            }

            // draw results
            PredictionList predictions_ = frame->predictions();
            cv::Mat frame_mat = frame->data();

            Utils::drawDetections(frame_mat, predictions_, {}, {}, 0.0f);
            for (const auto &prediction: predictions_) {
                if (prediction.subPredictions) {
                    Utils::drawDetections(frame_mat, prediction.subPredictions.value(), {}, {}, 0.0f);
                }
            }

            frame->setData(frame_mat);

            emit frameChanged(frame);
            emit frameChangedWithEvents(frame, events_history.keys());
        }

        // Submit the remaining events
        for (auto cam = cameras_history.begin(); cam != cameras_history.end(); ++cam) {
            for (auto e = cam->begin(); e != cam->end(); ++e) {
                Event event;

                try {
                    odb::transaction t(m_db->begin());
                    m_db->load(e->id, event);

                    event.setEndTime(e->event.endTime());
                    event.setTopScore(e->event.topScore());
                    event.setScore(e->event.score());
                    event.setData(QString::fromStdString(rfl::json::write(e->predictions)));

                    m_db->update(event);
                    t.commit();
                } catch (const std::exception &e) {
                    qCCritical(logger) << "Error updating db event" << QString("(%1),").arg(event.id()) << e.what();
                }
            }
        }
    }
    catch(const tbb::user_abort &) {}
    catch(const std::exception &e) {
        qCCritical(logger) << e.what();
    }
    catch(...) {
        qCCritical(logger) << "Uknown/Uncaught exception occurred in TrackedObjectProcessor.";
    }

    qCInfo(logger) << "Stopping" << objectName() << "thread";
}

std::pair<cv::Rect, bool> TrackedObjectProcessor::getSmartCropRect(cv::Rect objectBox, cv::Size frameSize, float aspectRatio)
{
    const int center_x = objectBox.x + objectBox.width / 2;
    const int center_y = objectBox.y + objectBox.height / 2;

    // Ratio 3:2
    const int crop_h = objectBox.height * 2;
    const int crop_w = crop_h * aspectRatio;

    const int ideal_x = center_x - crop_w / 2;
    const int ideal_y = center_y - crop_h / 2;

    const bool was_clamped = (ideal_x < 0 
                            || ideal_y < 0 
                            || (ideal_x + crop_w) > frameSize.width 
                            || (ideal_y + crop_h) > frameSize.height);
                            
    const int x = std::max(0, ideal_x);
    const int y = std::max(0, ideal_y);
    
    const int w = std::min(frameSize.width - x, crop_w);
    const int h = std::min(frameSize.height - y, crop_h);
    
    return {cv::Rect(x, y, w, h), was_clamped};
}
