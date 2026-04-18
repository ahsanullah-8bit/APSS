#pragma once

#include <utility>
#include <vector>
#include <cstddef>

#include <QThread>

#include <odb/sqlite/database.hxx>
#include <opencv2/core/mat.hpp>
#include <opencv2/core/types.hpp>

#include <db/event-odb.hxx>
#include <db/prediction-odb.hxx>
#include <utils/frame.h>
#include <utils/prediction.h>

class TrackedObjectProcessor : public QThread
{
    Q_OBJECT
public:
    enum EventUpdate {
        EventSubLabel    = 1 << 0,
        EventEndTime     = 1 << 1,
        EventTopScore    = 1 << 2,
        EventScore       = 1 << 3,
        EventThumbnail   = 1 << 4,
        EventPlate       = 1 << 5,
        EventData        = 1 << 6
    };

    struct TrackedEvent {
        size_t id;
        APSS::ODB::Event event;
        bool isPersisted = false;
        int lostCount = 0;
        int lastObjectBoxArea;
        cv::Mat bestPlate;
        std::vector<std::pair<APSS::ODB::Prediction, Prediction>> predictions;

        struct {
            cv::Mat img;
            bool wasSmartCropped = false;
            int counter = 0;
        } bestThumbnail;
    };

    explicit TrackedObjectProcessor(SharedFrameBoundedQueue &frameQueue,
                                    std::shared_ptr<odb::database> db,
                                    QObject *parent = nullptr);
    void stop();

signals:
    void eventPersisted(size_t id);
    void eventUpdated(size_t id, int updateType);
    void eventCompleted(size_t id);
    void frameChanged(SharedFrame frame);
    void frameChangedWithEvents(SharedFrame frame, const QList<int> &activeEvents);

    // QThread interface
protected:
    void run() override;

private:
    void processFrame(SharedFrame frame, QHash<int, TrackedEvent> &eventsHistory);
    std::pair<cv::Rect, bool> getSmartCropRect(cv::Rect object, cv::Size frameSize, float aspectRatio = 1.5f);
    void updateThumbnails(TrackedEvent &event, const Prediction& object, SharedFrame frame);
    void processLicensePlates(TrackedEvent& event, const Prediction& object, SharedFrame frame);
    void cleanupLostTracks(QHash<int, TrackedEvent>& eventsHistory);
    void finalizeAllEvents(QHash<QString, QHash<int, TrackedEvent>> &camerasHistory);

private:
    SharedFrameBoundedQueue &m_frameQueue;
    std::shared_ptr<odb::database> m_db;
};
