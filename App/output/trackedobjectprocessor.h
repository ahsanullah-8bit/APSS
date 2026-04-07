#pragma once

#include <cstddef>

#include <QThread>
#include <qcontainerfwd.h>

#include <odb/sqlite/database.hxx>
#include <db/event-odb.hxx>
#include <opencv2/core/mat.hpp>
#include <opencv2/core/types.hpp>

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
        Event event;
        bool isPersisted = false;
        int lostCount = 0;
        int lastObjectBoxArea;
        cv::Mat bestPlate;
        PredictionList predictions;

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
    void frameChanged(SharedFrame frame);
    void frameChangedWithEvents(SharedFrame frame, const QList<int> &activeEvents);

    // QThread interface
protected:
    void run() override;

private:
    QString cropAndSaveThumbnail(const QString &name, const SharedFrame frame, const Prediction &pred);
    std::pair<cv::Rect, bool> getSmartCropRect(cv::Rect object, cv::Size frameSize, float aspectRatio = 1.5f);

private:
    SharedFrameBoundedQueue &m_frameQueue;
    std::shared_ptr<odb::database> m_db;
};
