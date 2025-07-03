#pragma once

#include <QString>
#include <QHash>
#include <QMutex>
#include <QWaitCondition>

#include <opencv2/core/mat.hpp>
#include <chrono>
#include <optional>
#include <qdatetime.h>
#include <memory>
// #include <QSharedPointer>

#include <tbb_patched.h>
#include "apss.h"

// struct FrameId {
//     // Camera Id + Frame Id + Timestamp
//     constexpr static const char* FRAME_ID_PATTERN = "";

//     QString cameraId;
//     QString frameIndex;
//     QTime timeStamp;

//     FrameId() = default;
//     FrameId(const QString &wholeId) {
//         fromString(wholeId);
//     }
//     FrameId(const QString& cameraId, const QString& frameIndex, const QTime& timeStamp = QTime::currentTime()) {
//         this->cameraId = cameraId;
//         this->frameIndex = frameIndex;
//         this->timeStamp = timeStamp;
//     }
//     FrameId(const FrameId &) = default;
//     FrameId(FrameId &&) = default;
//     FrameId &operator=(const FrameId &) = default;
//     FrameId &operator=(FrameId &&) = default;

//     QString toString() const {
//         return QString::fromStdString(std::format("{}_{}_{}",
//                                                   cameraId.toStdString(),
//                                                   frameIndex.toStdString(),
//                                                   timeStamp.toString("hh:mm:ss").toStdString()
//                                                   ));
//     }

//     void fromString(const QString& wholeId) {
//         Q_ASSERT(!wholeId.isEmpty());
//         const QList splits = wholeId.split('_', Qt::SkipEmptyParts);

//         Q_ASSERT(splits.size() >= 3);
//         cameraId = splits[0];
//         frameIndex = splits[1];
//         timeStamp.fromString(splits[2], "hh:mm:ss");
//     }
// };


/**
 * @brief A rich Frame class designed for a video processing pipeline.
 *
 * Holds the raw frame data, along with associated metadata relevant
 * to surveillance applications.
 */
class Frame {
public:
    QWaitCondition frameProcessedCond;

public:
    using TimePoint = std::chrono::time_point<std::chrono::system_clock>;
    using TypePredictionsHash = QHash<Prediction::Type, PredictionList>;

    Frame() = default;
    Frame(const QString &cameraId, const QString &frameId, cv::Mat data, TimePoint timestamp = std::chrono::system_clock::now());
    Frame(const QString &cameraId, const QString &frameId, cv::Mat data, const TypePredictionsHash &predictions, TimePoint timestamp = std::chrono::system_clock::now());
    Frame(const Frame &other);
    Frame(Frame &&other) noexcept;
    Frame& operator=(const Frame &other);
    Frame& operator=(Frame &&other) noexcept;
    Frame clone() const;

    // Accessors and Mutators

    QString id();
    QString cameraId() const;
    QString frameId() const;
    cv::Mat data() const;
    TimePoint timestamp() const;
    const TypePredictionsHash &predictions() const;
    PredictionList predictions(const Prediction::Type target) const;
    std::optional<cv::Rect> roi() const;
    bool hasExpired() const;

    void setCameraId(const QString &newCameraId);
    void setFrameId(const QString &newFrameId);
    void setData(cv::Mat newData);
    void setTimestamp(const TimePoint &newTimestamp);
    void setPredictions(const TypePredictionsHash &newPredictions);
    void setPredictions(TypePredictionsHash &&newPredictions);
    void setPredictions(const Prediction::Type target, const PredictionList &newPredictions);
    void setPredictions(const Prediction::Type target, PredictionList &&newPredictions);
    void setRoi(std::optional<cv::Rect> newRoi);
    void setHasExpired(bool newHasExpired);

private:
    std::atomic_bool m_hasExpired = false;
    QString m_cameraId;
    QString m_frameId;
    cv::Mat m_data;
    TimePoint m_timestamp;
    TypePredictionsHash m_predictions; // target_name, predictions for that target.
    std::optional<cv::Rect> m_roi;
};

using SharedFrame = std::shared_ptr<Frame>;
using SharedFrameBoundedQueue = tbb::concurrent_bounded_queue<SharedFrame>;
using SharedFrameList = std::vector<SharedFrame>;

inline Frame::Frame(const QString &cameraId, const QString &frameId, cv::Mat data, TimePoint timestamp)
    : m_cameraId(cameraId)
    , m_frameId(frameId)
    , m_data(data)
    , m_timestamp(timestamp)
{}

inline Frame::Frame(const QString &cameraId, const QString &frameId, cv::Mat data, const TypePredictionsHash &predictions, TimePoint timestamp)
    : m_cameraId(cameraId)
    , m_frameId(frameId)
    , m_data(data)
    , m_predictions(predictions)
    , m_timestamp(timestamp)
{}

inline Frame::Frame(const Frame &other)
    : m_cameraId(other.m_cameraId)
    , m_frameId(other.m_frameId)
    , m_data(other.m_data)
    , m_timestamp(other.m_timestamp)
    , m_predictions(other.m_predictions)
    , m_roi(other.m_roi)
{}

inline Frame::Frame(Frame &&other) noexcept
    : m_cameraId(std::move(other.m_cameraId))
    , m_frameId(std::move(other.m_frameId))
    , m_data(std::move(other.m_data))
    , m_timestamp(std::move(other.m_timestamp))
    , m_predictions(std::move(other.m_predictions))
    , m_roi(std::move(other.m_roi))
{}

inline Frame &Frame::operator=(const Frame &other) {
    if (this == &other) {
        return *this;
    }

    m_cameraId = other.m_cameraId;
    m_frameId = other.m_frameId;
    m_data = other.m_data;
    m_timestamp = other.m_timestamp;
    m_predictions = other.m_predictions;
    m_roi = other.m_roi;
    return *this;
}

inline Frame &Frame::operator=(Frame &&other) noexcept {
    if (this == &other) {
        return *this;
    }

    m_cameraId = std::move(other.m_cameraId);
    m_frameId = std::move(other.m_frameId);
    m_data = std::move(other.m_data);
    m_timestamp = std::move(other.m_timestamp);
    m_predictions = std::move(other.m_predictions);
    m_roi = std::move(other.m_roi);
    return *this;
}

inline Frame Frame::clone() const {
    return Frame(m_cameraId, m_frameId, m_data.clone(), m_predictions, m_timestamp);
}

inline QString Frame::id()
{
    return QString("%1_%2").arg(m_cameraId).arg(m_frameId);
}

inline QString Frame::cameraId() const
{
    return m_cameraId;
}

inline QString Frame::frameId() const
{
    return m_frameId;
}

inline cv::Mat Frame::data() const {
    return m_data;
}

inline PredictionList Frame::predictions(const Prediction::Type target) const {
    return m_predictions[target];
}

inline const Frame::TypePredictionsHash &Frame::predictions() const {
    return m_predictions;
}

inline std::optional<cv::Rect> Frame::roi() const {
    return m_roi;
}

inline Frame::TimePoint Frame::timestamp() const {
    return m_timestamp;
}

inline bool Frame::hasExpired() const
{
    return m_hasExpired.load(std::memory_order_acquire);
}

inline void Frame::setFrameId(const QString &newFrameId) {
    m_frameId = newFrameId;
}

inline void Frame::setData(cv::Mat newData) {
    m_data = std::move(newData);
}

inline void Frame::setTimestamp(const TimePoint &newTimestamp) {
    m_timestamp = newTimestamp;
}

inline void Frame::setPredictions(const TypePredictionsHash &newPredictions) {
    m_predictions = newPredictions;
}

inline void Frame::setPredictions(TypePredictionsHash &&newPredictions) {
    m_predictions = std::move(newPredictions);
}

inline void Frame::setPredictions(const Prediction::Type target, const PredictionList &newPredictions) {
    m_predictions[target] = newPredictions;
}

inline void Frame::setPredictions(const Prediction::Type target, PredictionList &&newPredictions) {
    m_predictions[target] = std::move(newPredictions);
}

inline void Frame::setRoi(std::optional<cv::Rect> newRoi) {
    m_roi = newRoi;
}

inline void Frame::setHasExpired(bool newHasExpired)
{
    m_hasExpired.store(newHasExpired, std::memory_order_release);
}

inline void Frame::setCameraId(const QString &newCameraId)
{
    m_cameraId = newCameraId;
}
