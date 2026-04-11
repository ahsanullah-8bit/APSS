#include <algorithm>
#include <exception>

#include <QLoggingCategory>
#include <qcontainerfwd.h>

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <odb/transaction.hxx>
#include <rfl/json/write.hpp>
#include <rfl/json.hpp>

#include <apss.h>
#include <detectors/image.h>
#include <db/event-odb.hxx>
#include <db/prediction-odb.hxx>
#include "trackedobjectprocessor.h"

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
            processFrame(frame, events_history);
            cleanupLostTracks(events_history);

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

        finalizeAllEvents(cameras_history);
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

void TrackedObjectProcessor::processFrame(SharedFrame frame, QHash<int, TrackedEvent> &eventsHistory)
{
    for (auto &event_history : eventsHistory)
        event_history.lostCount++;

    // Process current predictions
    try {
        for (const Prediction& object : frame->predictions()) {
            if (object.trackerId < 0) 
                continue;

            auto &event_history = eventsHistory[object.trackerId];
            event_history.lostCount = 0;

            updateThumbnails(event_history, object, frame);
            processLicensePlates(event_history, object, frame);

            if (!event_history.isPersisted) {
                odb::transaction t(m_db->begin());

                // Event
                auto &event = event_history.event;
                event.label = QString::fromStdString(object.className);
                event.camera = frame->camera();
                event.startTime = frame->timestamp();
                event.topScore = object.conf;
                event.trackerId = object.trackerId;
                event_history.id = m_db->persist(event);

                // NOTE: Only the first prediction will have timestamp. To acheive replayable prediction footage,
                // we would need to track predictions separately. But that's not my goal and nothing is using the
                // predictions being submitted to the database, at the moment. So, I won't bother updating them. 
                // But only submitted at the end.
                
                APSS::ODB::Prediction p;
                p.eventId = event_history.id;
                p.frameId = frame->id();
                p.streamTimestamp = frame->timestamp();
                event_history.predictions.emplace_back(p, object);

                t.commit();

                event_history.isPersisted = true;
                emit eventPersisted(event_history.id);
            } else {
                // Update existing event
                APSS::ODB::Prediction p;
                p.eventId = event_history.id;
                p.frameId = frame->id();
                p.streamTimestamp = frame->timestamp();
                event_history.predictions.emplace_back(p, object);
                event_history.event.endTime = frame->timestamp();

                if (object.conf > event_history.event.topScore)
                    event_history.event.topScore = object.conf;

                // TODO: Do a proper average of all the scores
                event_history.event.score = object.conf;

                // emit eventUpdated(event_history.id);
            }
        }
    } catch (const std::exception &e) {
        qCCritical(logger) << "Error updating db event," << e.what();
    }
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

void TrackedObjectProcessor::updateThumbnails(TrackedEvent &eventHistory, const Prediction& object, SharedFrame frame)
{
    if (eventHistory.bestThumbnail.counter > 2)
        return;

    cv::Mat frame_data = frame->data();
    auto &best_thumbnail = eventHistory.bestThumbnail;
    const auto [crop_rect, is_smart_croppable] = getSmartCropRect(object.box, frame_data.size());

    bool is_first_ever = best_thumbnail.img.empty();
    bool improved_visibility = is_smart_croppable && !best_thumbnail.wasSmartCropped;
    bool significantly_larger = (object.box.area() > eventHistory.lastObjectBoxArea * 1.2f);

    if (is_first_ever || improved_visibility || (is_smart_croppable && significantly_larger)) {
        best_thumbnail.img = frame_data(crop_rect).clone();
        best_thumbnail.counter++;
        best_thumbnail.wasSmartCropped = is_smart_croppable;
        eventHistory.lastObjectBoxArea = object.box.area();
        
        if (is_first_ever)
            eventHistory.event.thumbnail = THUMB_DIR.filePath(QString("%1_%2.jpg").arg(frame->camera()).arg(object.trackerId));  // Dragons, I KNOW!
        // TODO: This should be moved to a separate thread.
        cv::imwrite(eventHistory.event.thumbnail.toStdString(), eventHistory.bestThumbnail.img);

        if (eventHistory.isPersisted)
            emit eventUpdated(eventHistory.id, EventThumbnail);
    }
}

void TrackedObjectProcessor::processLicensePlates(TrackedEvent& eventHistory, const Prediction& object, SharedFrame frame)
{
    if (!object.subPredictions)
        return;

    cv::Mat frame_data = frame->data();
    auto &best_plate = eventHistory.bestPlate;
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
                cv::imwrite(THUMB_DIR.filePath(QString("%1_%2_lp.jpg").arg(frame->camera()).arg(object.trackerId)).toStdString(), eventHistory.bestPlate);

            if (eventHistory.isPersisted)
                emit eventUpdated(eventHistory.id, EventPlate);
        }
    }
}
void TrackedObjectProcessor::cleanupLostTracks(QHash<int, TrackedEvent>& eventsHistory)
{
    // Remove lost
    try {
        for (auto it = eventsHistory.begin(); it != eventsHistory.end();) {
            if (it->lostCount > TRACK_MAX_EVENTS) {
                const auto &history = eventsHistory[it.key()];

                // TODO: Persist events if they're not.                
                APSS::ODB::Event event;
                odb::transaction t(m_db->begin());

                m_db->load(it->id, event);
                event.endTime = history.event.endTime;
                event.topScore = history.event.topScore;
                event.score = history.event.score;
                m_db->update(event);

                for (auto [p, object] : history.predictions) {
                    p.data = QString::fromStdString(rfl::json::write(object));
                    m_db->persist(p);
                }

                t.commit();

                emit eventUpdated(it->id, EventEndTime | EventTopScore | EventScore | EventData);
                
                it = eventsHistory.erase(it);
            } else {
                ++it;
            }
        }
    } catch (const std::exception &e) {
        qCCritical(logger) << "Error updating db event," << e.what();
    }
}
void TrackedObjectProcessor::finalizeAllEvents(QHash<QString, QHash<int, TrackedEvent>> &camerasHistory)
{
    // Submit the remaining events
    try {
        for (auto cam = camerasHistory.begin(); cam != camerasHistory.end(); ++cam) {
            for (auto e = cam->begin(); e != cam->end(); ++e) {
                APSS::ODB::Event event;
                odb::transaction t(m_db->begin());
                
                m_db->load(e->id, event);
                event.endTime = e->event.endTime;
                event.topScore = e->event.topScore;
                event.score = e->event.score;
                m_db->update(event);

                for (auto [p, object] : e->predictions) {
                    p.data = QString::fromStdString(rfl::json::write(object));
                    m_db->persist(p);
                }

                t.commit();
            }
        }
    } catch (const std::exception &e) {
        qCCritical(logger) << "Error updating db event," << e.what();
    }
}