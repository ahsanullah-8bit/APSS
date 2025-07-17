#include "frame.h"

Frame::Frame(const QString &cameraId, const QString &frameId, cv::Mat data, TimePoint timestamp)
    : m_cameraId(cameraId)
    , m_frameId(frameId)
    , m_data(data)
    , m_timestamp(timestamp)
{}

Frame::Frame(const QString &cameraId, const QString &frameId, cv::Mat data, const QHash<Prediction::Type, PredictionList> &predictions, TimePoint timestamp)
    : m_cameraId(cameraId)
    , m_frameId(frameId)
    , m_data(data)
    , m_predictions(predictions)
    , m_timestamp(timestamp)
{}

Frame::Frame(const Frame &other)
    : m_cameraId(other.m_cameraId)
    , m_frameId(other.m_frameId)
    , m_data(other.m_data)
    , m_timestamp(other.m_timestamp)
    , m_predictions(other.m_predictions)
    , m_roi(other.m_roi)
{}

Frame::Frame(Frame &&other) noexcept
    : m_cameraId(std::move(other.m_cameraId))
    , m_frameId(std::move(other.m_frameId))
    , m_data(std::move(other.m_data))
    , m_timestamp(std::move(other.m_timestamp))
    , m_predictions(std::move(other.m_predictions))
    , m_roi(std::move(other.m_roi))
{}

Frame &Frame::operator=(const Frame &other) {
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

Frame &Frame::operator=(Frame &&other) noexcept {
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

Frame Frame::clone() const {
    return Frame(m_cameraId, m_frameId, m_data.clone(), m_predictions, m_timestamp);
}

QString Frame::id()
{
    return QString("%1_%2").arg(m_cameraId, m_frameId);
}

QString Frame::cameraId() const
{
    return m_cameraId;
}

QString Frame::frameId() const
{
    return m_frameId;
}

cv::Mat Frame::data() const {
    return m_data;
}

Frame::TimePoint Frame::timestamp() const {
    return m_timestamp;
}

const QHash<Prediction::Type, PredictionList> &Frame::predictions() const {
    return m_predictions;
}

PredictionList Frame::predictions(const Prediction::Type target) const {
    return m_predictions[target];
}

QHash<Prediction::Type, PredictionList> &Frame::predictionsByRef()
{
    return m_predictions;
}

PredictionList &Frame::predictionsByRef(const Prediction::Type target)
{
    return m_predictions[target];
}

std::optional<cv::Rect> Frame::roi() const {
    return m_roi;
}

bool Frame::hasExpired() const
{
    return m_hasExpired.load(std::memory_order_acquire);
}

std::vector<PaddleOCR::OCRPredictResultList> Frame::ocrResults() const
{
    return m_ocrResults;
}

std::optional<ANPRSnapshot> Frame::anprSnapshot() const
{
    return m_anprSnapshot;
}

bool Frame::hasBeenProcessed() const
{
    return m_hasBeenProcessed.load(std::memory_order_acquire);
}

void Frame::setCameraId(const QString &newCameraId)
{
    m_cameraId = newCameraId;
}

void Frame::setFrameId(const QString &newFrameId) {
    m_frameId = newFrameId;
}

void Frame::setData(cv::Mat newData) {
    m_data = std::move(newData);
}

void Frame::setTimestamp(const TimePoint &newTimestamp) {
    m_timestamp = newTimestamp;
}

void Frame::setPredictions(const QHash<Prediction::Type, PredictionList> &newPredictions) {
    m_predictions = newPredictions;
}

// void Frame::setPredictions(QHash<Prediction::Type, PredictionList> &&newPredictions) {
//     m_predictions = std::move(newPredictions);
// }

void Frame::setPredictions(const Prediction::Type target, const PredictionList &newPredictions) {
    m_predictions[target] = newPredictions;
}

// void Frame::setPredictions(const Prediction::Type target, PredictionList &&newPredictions) {
//     m_predictions[target] = std::move(newPredictions);
// }

void Frame::setRoi(std::optional<cv::Rect> newRoi) {
    m_roi = newRoi;
}

void Frame::setHasExpired(bool newHasExpired)
{
    bool current = m_hasExpired.load(std::memory_order_relaxed);
    if (current == newHasExpired)
        return;

    while (!m_hasExpired.compare_exchange_weak(
        current, newHasExpired,
        std::memory_order_release,
        std::memory_order_relaxed
        )) {

        if (current == newHasExpired)
            return;
    }
}

// void Frame::setOcrResults(std::vector<PaddleOCR::OCRPredictResultList> &&newOcrResults)
// {
//     m_ocrResults = std::move(newOcrResults);
// }

void Frame::setOcrResults(const std::vector<PaddleOCR::OCRPredictResultList> &newOcrResults)
{
    m_ocrResults = newOcrResults;
}

void Frame::setAnprSnapshot(std::optional<ANPRSnapshot> newAnprSnapshot)
{
    m_anprSnapshot = newAnprSnapshot;
}

void Frame::setHasBeenProcessed(bool newHasBeenProcessed)
{
    bool current = m_hasBeenProcessed.load(std::memory_order_relaxed);
    if (current == newHasBeenProcessed)
        return;

    while (!m_hasBeenProcessed.compare_exchange_weak(
        current, newHasBeenProcessed,
        std::memory_order_release,
        std::memory_order_relaxed
        )) {

        if (current == newHasBeenProcessed)
            return;
    }
}
