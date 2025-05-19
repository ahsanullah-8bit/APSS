#pragma once

#include <QString>
#include <QHash>

#include <opencv2/core/mat.hpp>
#include <chrono>
#include <optional>
#include <qdatetime.h>

#include "apss.h"


struct FrameId {
    // Camera Id + Frame Id + Timestamp
    constexpr static const char* FRAME_ID_PATTERN = "";

    QString cameraId;
    QString frameIndex;
    QTime timeStamp;

    FrameId() = default;
    FrameId(const QString &wholeId) {
        fromString(wholeId);
    }
    FrameId(const QString& cameraId, const QString& frameIndex, const QTime& timeStamp = QTime::currentTime()) {
        this->cameraId = cameraId;
        this->frameIndex = frameIndex;
        this->timeStamp = timeStamp;
    }
    FrameId(const FrameId &) = default;
    FrameId(FrameId &&) = default;
    FrameId &operator=(const FrameId &) = default;
    FrameId &operator=(FrameId &&) = default;

    QString toString() const {
        return QString::fromStdString(std::format("{}_{}_{}",
                                                  cameraId.toStdString(),
                                                  frameIndex.toStdString(),
                                                  timeStamp.toString("hh:mm:ss").toStdString()
                                                  ));
    }

    void fromString(const QString& wholeId) {
        Q_ASSERT(!wholeId.isEmpty());
        const QList splits = wholeId.split('_', Qt::SkipEmptyParts);

        Q_ASSERT(splits.size() >= 3);
        cameraId = splits[0];
        frameIndex = splits[1];
        timeStamp.fromString(splits[2], "hh:mm:ss");
    }
};


/**
 * @brief A rich Frame class designed for a video processing pipeline.
 *
 * Holds the raw frame data, along with associated metadata relevant
 * to surveillance applications.
 */
class Frame {
public:
    using TimePoint = std::chrono::time_point<std::chrono::system_clock>;
    using TypePredictionsHash = QHash<Prediction::Type, PredictionList>;

    Frame() = default;

    Frame(const FrameId &frameId,
          cv::Mat data,
          TimePoint timestamp = std::chrono::system_clock::now())
        : m_frameId(frameId)
        , m_data(data)
        , m_timestamp(timestamp)
    {}

    Frame(const FrameId &frameId,
          cv::Mat data,
          const TypePredictionsHash &predictions,
          TimePoint timestamp = std::chrono::system_clock::now())
        : m_frameId(frameId)
        , m_data(data)
        , m_predictions(predictions)
        , m_timestamp(timestamp)
    {}

    // Copy constructor (deep copy of cv::Mat)
    Frame(const Frame& other)
        : m_frameId(other.m_frameId)
        , m_data(other.m_data)
        , m_timestamp(other.m_timestamp)
        , m_predictions(other.m_predictions)
        , m_cameraSource(other.m_cameraSource)
        , m_processingStage(other.m_processingStage)
        , m_roi(other.m_roi)
    {}

    // Move constructor
    Frame(Frame&& other) noexcept
        : m_frameId(std::move(other.m_frameId))
        , m_data(std::move(other.m_data))
        , m_timestamp(std::move(other.m_timestamp))
        , m_predictions(std::move(other.m_predictions))
        , m_cameraSource(std::move(other.m_cameraSource))
        , m_processingStage(std::move(other.m_processingStage))
        , m_roi(std::move(other.m_roi))
    {}

    // Copy assignment operator (deep copy of cv::Mat)
    Frame& operator=(const Frame& other) {
        if (this == &other) {
            return *this;
        }
        m_frameId = other.m_frameId;
        m_data = other.m_data;
        m_timestamp = other.m_timestamp;
        m_predictions = other.m_predictions;
        m_cameraSource = other.m_cameraSource;
        m_processingStage = other.m_processingStage;
        m_roi = other.m_roi;
        return *this;
    }

    // Move assignment operator
    Frame& operator=(Frame&& other) noexcept {
        if (this == &other) {
            return *this;
        }
        m_frameId = std::move(other.m_frameId);
        m_data = std::move(other.m_data);
        m_timestamp = std::move(other.m_timestamp);
        m_predictions = std::move(other.m_predictions);
        m_cameraSource = std::move(other.m_cameraSource);
        m_processingStage = std::move(other.m_processingStage);
        m_roi = std::move(other.m_roi);
        return *this;
    }

    Frame clone() const {
        return Frame(m_frameId, m_data.clone(), m_predictions, m_timestamp);
    }

    // Accessors and Mutators

    FrameId frameId() const {
        return m_frameId;
    }

    QString frameIdString() const {
        return m_frameId.toString();
    }

    void setFrameId(const FrameId &newFrameId) {
        m_frameId = newFrameId;
    }

    void setFrameId(const QString &newFrameId) {
        m_frameId.fromString(newFrameId);
    }

    cv::Mat data() const {
        return m_data;
    }

    void setData(cv::Mat newData) {
        m_data = std::move(newData);
    }

    TimePoint timestamp() const {
        return m_timestamp;
    }

    void setTimestamp(const TimePoint &newTimestamp) {
        m_timestamp = newTimestamp;
    }

    const TypePredictionsHash &predictions() const {
        return m_predictions;
    }

    PredictionList predictions(const Prediction::Type target) const {
        return m_predictions[target];
    }

    void setPredictions(const TypePredictionsHash &newPredictions) {
        m_predictions = newPredictions;
    }

    void setPredictions(TypePredictionsHash &&newPredictions) {
        m_predictions = std::move(newPredictions);
    }

    void setPredictions(const Prediction::Type target, const PredictionList &newPredictions) {
        m_predictions[target] = newPredictions;
    }

    void setPredictions(const Prediction::Type target, PredictionList &&newPredictions) {
        m_predictions[target] = std::move(newPredictions);
    }

    std::optional<QString> cameraSource() const {
        return m_cameraSource;
    }

    void setCameraSource(std::optional<QString> newCameraSource) {
        m_cameraSource = newCameraSource;
    }

    std::optional<QString> processingStage() const {
        return m_processingStage;
    }

    void setProcessingStage(std::optional<QString> newProcessingStage) {
        m_processingStage = newProcessingStage;
    }

    std::optional<cv::Rect> roi() const {
        return m_roi;
    }

    void setRoi(std::optional<cv::Rect> newRoi) {
        m_roi = newRoi;
    }


    QStringList lp_paths;
private:
    FrameId m_frameId;
    cv::Mat m_data;
    TimePoint m_timestamp;
    TypePredictionsHash m_predictions; // target_name, predictions for that target.
    std::optional<QString> m_cameraSource;
    std::optional<QString> m_processingStage;
    std::optional<cv::Rect> m_roi;

    // You might consider adding more metadata relevant to your pipeline, such as:
    // - Frame number
    // - Sequence ID
    // - Metadata from sensors
    // - Flags indicating processing status
};
