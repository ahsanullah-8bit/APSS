#pragma once

#include <chrono>
#include <optional>
#include <memory>
#include <shared_mutex>

#include <QString>
#include <QHash>
#include <QMutex>
#include <QWaitCondition>
#include <qdatetime.h>

#include <opencv2/core/mat.hpp>

#include <tbb_patched.h>
#include "apss.h"
#include "prediction.h"
#include "detectors/licensed/utility.h"
#include "config/platerecognizerconfig.h"


/**
 * @brief A rich Frame class designed for a video processing pipeline.
 *
 * Holds the raw frame data, along with associated metadata relevant
 * to surveillance applications. It is supposed to be passed around as a shared frame.
 * @warning you aren't supposed to draw anything on its cv::Mat.
 */
class Frame {
public:
    using TimePoint = std::chrono::time_point<std::chrono::system_clock>;
    using TypePredictionsHash = QHash<Prediction::Type, PredictionList>;

    Frame() = default;
    Frame(const QString &cameraId,
          const QString &frameId,
          cv::Mat data,
          TimePoint timestamp = std::chrono::system_clock::now());
    Frame(const QString &cameraId,
          const QString &frameId,
          cv::Mat data,
          const QHash<Prediction::Type, PredictionList> &predictions,
          TimePoint timestamp = std::chrono::system_clock::now());
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
    const QHash<Prediction::Type, PredictionList> &predictions() const;
    PredictionList predictions(const Prediction::Type target) const;
    bool hasExpired() const;
    bool hasBeenProcessed() const;
    std::vector<PaddleOCR::OCRPredictResultList> ocrResults() const;
    std::optional<ANPRSnapshot> anprSnapshot() const;

    void setCameraId(const QString &newCameraId);
    void setFrameId(const QString &newFrameId);
    void setData(cv::Mat newData);
    void setTimestamp(const TimePoint &newTimestamp);
    void setPredictions(const QHash<Prediction::Type, PredictionList> &newPredictions);
    void setPredictions(QHash<Prediction::Type, PredictionList> &&newPredictions);
    void setPredictions(const Prediction::Type target, const PredictionList &newPredictions);
    void setPredictions(const Prediction::Type target, PredictionList &&newPredictions);
    void setHasExpired(bool newHasExpired);
    void setHasBeenProcessed(bool newHasBeenProcessed);
    void setOcrResults(const std::vector<PaddleOCR::OCRPredictResultList> &newOcrResults);
    void setOcrResults(std::vector<PaddleOCR::OCRPredictResultList> &&newOcrResults);
    void setAnprSnapshot(std::optional<ANPRSnapshot> newAnprSnapshot);

private:
    QString m_cameraId;
    QString m_frameId;
    cv::Mat m_data;
    TimePoint m_timestamp;
    std::atomic_bool m_hasExpired = false;
    std::atomic_bool m_hasBeenProcessed = false;
    QHash<Prediction::Type, PredictionList> m_predictions;
    std::vector<PaddleOCR::OCRPredictResultList> m_ocrResults;
    std::optional<ANPRSnapshot> m_anprSnapshot; // Comprehensive ANPR data

    mutable std::shared_mutex m_mtx;
};

using SharedFrame = std::shared_ptr<Frame>;
using SharedFrameBoundedQueue = tbb::concurrent_bounded_queue<SharedFrame>;
using SharedFrameList = std::vector<SharedFrame>;
