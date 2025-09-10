#pragma once

#include <chrono>
#include <optional>
#include <memory>
#include <shared_mutex>

#include <QString>
#include <QHash>
#include <QMutex>
#include <QWaitCondition>
#include <QSharedPointer>
#include <qdatetime.h>

extern "C" {
#include <libavcodec/packet.h>
}

#include <opencv2/core/mat.hpp>

#include <tbb_patched.h>
#include <apss.h>
#include <detectors/licensed/utility.h>
#include <config/platerecognizerconfig.h>
#include <utils/prediction.h>


using SharedPacket = QSharedPointer<AVPacket>;

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
    // using TypePredictionsHash = QHash<Prediction::Type, PredictionList>;

    Frame() = default;
    Frame(const QString &camera, size_t frameIndx, cv::Mat data,
          const PredictionList &predictions = {},
          QDateTime timestamp = QDateTime::currentDateTimeUtc());

    Frame(const QString &camera, size_t frameIndx, const cv::Mat &data,
          const PredictionList &predictions,
          QDateTime timestamp,
          const std::vector<PaddleOCR::OCRPredictResultList> &ocrResults,
          std::optional<ANPRSnapshot> anprSnapshot);

    Frame(const Frame &other)            = delete;
    Frame(Frame &&other)                 = delete;
    Frame& operator=(const Frame &other) = delete;
    Frame& operator=(Frame &&other)      = delete;
    ~Frame();
    Frame clone() const;

    // Accessors and Mutators

    QString id();
    QString camera() const;
    size_t frameIndx() const;
    cv::Mat data() const;
    QDateTime timestamp() const;
    PredictionList predictions() const;
    // PredictionList predictions(const Prediction::Type target) const;
    bool hasExpired() const;
    bool hasBeenProcessed() const;
    std::vector<PaddleOCR::OCRPredictResultList> ocrResults() const;
    std::optional<ANPRSnapshot> anprSnapshot() const;

    void setData(cv::Mat newData);
    void setTimestamp(const QDateTime &newTimestamp);
    void setPredictions(const PredictionList &newPredictions);
    void setPredictions(PredictionList &&newPredictions);
    void addPredictions(const PredictionList &newPredictions);
    void addPredictions(PredictionList &&newPredictions);
    void setHasExpired(bool newHasExpired);
    void setHasBeenProcessed(bool newHasBeenProcessed);
    void setOcrResults(const std::vector<PaddleOCR::OCRPredictResultList> &newOcrResults);
    void setOcrResults(std::vector<PaddleOCR::OCRPredictResultList> &&newOcrResults);
    void setAnprSnapshot(std::optional<ANPRSnapshot> newAnprSnapshot);

    // static helpers
    static QString makeFrameId(const QString &camera, size_t frameIndx);
    static std::optional<std::tuple<QString, size_t>> splitFrameId(const QString &frameIndx);


private:
    QString m_camera;
    size_t m_frameIndx;
    cv::Mat m_data;
    QDateTime m_timestamp;
    std::atomic_bool m_hasExpired = false;
    std::atomic_bool m_hasBeenProcessed = false;
    // QHash<Prediction::Type, PredictionList> m_predictions;
    PredictionList m_predictions;
    std::vector<PaddleOCR::OCRPredictResultList> m_ocrResults;
    std::optional<ANPRSnapshot> m_anprSnapshot; // Comprehensive ANPR data

    mutable std::shared_mutex m_mtx;
};

using SharedFrame = std::shared_ptr<Frame>;
using SharedFrameBoundedQueue = tbb::concurrent_bounded_queue<SharedFrame>;
using SharedFrameList = std::vector<SharedFrame>;
