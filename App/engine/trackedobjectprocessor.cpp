#include <QLoggingCategory>

#include <rfl/json.hpp>

#include "db/sqlite/event-odb.hxx"
#include "db/sqlite/frameprediction-odb.hxx"
#include "events/detectionsubpub.h"
#include "trackedobjectprocessor.h"
#include "utils/framemanager.h"

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

    QHash<int, Event> active_events;            // trackerId -> Event
    QHash<int, PredictionList> object_history;  // trackerId -> Prediction history
    QHash<int, int> lost_counts;                // trackerId -> Consecutive lost frames
    QHash<int, QDateTime> last_seen_timestamps; // trackerId -> Last seen timestamp

    try {
        while(!isInterruptionRequested()) {
            SharedFrame frame;
            m_frameQueue.pop(frame);
            if (!frame)
                continue;

            const PredictionList& predictions = frame->predictions();
            const QDateTime frame_time = frame->timestamp();

            // process active objects first
            const QList<int> active_ids = object_history.keys();
            for (int tracker_id : active_ids) {
                // check if object appears in current predictions
                bool found = std::any_of(predictions.cbegin(), predictions.cend(),
                                         [tracker_id](const Prediction& p) { return p.trackerId == tracker_id; });

                if (found) {
                    // reset lost counter on reappearance
                    lost_counts[tracker_id] = 0;
                } else {
                    // handle lost object
                    if (!lost_counts.contains(tracker_id)) {
                        lost_counts[tracker_id] = 1;
                    } else {
                        lost_counts[tracker_id]++;
                    }

                    // remove if exceeded loss limit
                    if (lost_counts[tracker_id] > TRACKER_OBJECT_LOSS_LIMIT) {
                        if (!active_events.contains(tracker_id)) {
                            qCWarning(logger) << "Missing event during cleanup for" << tracker_id;
                        } else {
                            Event& event = active_events[tracker_id];

                            const auto &prediction_history = object_history[tracker_id];
                            const std::string label = prediction_history.empty() ? "Uknown"     // I hope this first condition is not true
                                                                             : prediction_history.at(0).className;
                            event.setLabel(QString::fromStdString(label));
                            event.setCamera(frame->camera());
                            event.setEndTime(last_seen_timestamps[tracker_id]); // last seen time
                            event.setTrackerId(tracker_id);

                            const std::string predictions = rfl::json::write(prediction_history);
                            event.setData(QString::fromStdString(predictions));

                            // persist event with prediction history
                            // event.setPredictionHistory(objectHistory[tracker_id]);

                            odb::transaction t(m_db->begin());
                            try {
                                m_db->persist(event);
                                t.commit();
                            } catch (const odb::exception& e) {
                                t.rollback();
                                qCCritical(logger) << "DB Error:" << e.what();
                            }
                        }

                        // qCDebug(logger) << "Removed:" << tracker_id;

                        // cleanup tracking data
                        object_history.remove(tracker_id);
                        active_events.remove(tracker_id);
                        lost_counts.remove(tracker_id);
                        last_seen_timestamps.remove(tracker_id);
                    }
                }
            }

            // process current predictions
            for (const Prediction& pred : predictions) {
                if (pred.trackerId == -1) continue;
                const int tracker_id = pred.trackerId;

                if (!object_history.contains(tracker_id)) {
                    // initialize new event
                    Event newEvent;
                    newEvent.setId(QString("%1-%2")
                                       .arg(frame_time.toString(Qt::ISODateWithMs),
                                            QUuid::createUuid().toString(QUuid::WithoutBraces)));
                    newEvent.setStartTime(frame_time);
                    newEvent.setTopScore(pred.conf);

                    // store first prediction
                    object_history[tracker_id] = {pred};
                    active_events[tracker_id] = newEvent;
                    last_seen_timestamps[tracker_id] = frame_time;
                } else {
                    // update existing event
                    object_history[tracker_id].push_back(pred);
                    last_seen_timestamps[tracker_id] = frame_time;

                    auto &event = active_events[tracker_id];
                    if (event.topScore() < pred.conf)
                        event.setTopScore(pred.conf);

                    event.setScore(pred.conf);
                }
            }

            emit frameChanged(frame);
        }
    }
    catch(const tbb::user_abort &) {}
    catch(const std::exception &e) {
        qCCritical(logger) << e.what();
    }
    catch(...) {
        qCCritical(logger) << "Uknown/Uncaught exception occurred.";
    }

    qCInfo(logger) << "Stopping" << objectName() << "thread";
}
