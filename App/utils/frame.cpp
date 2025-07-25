#include "frame.h"

Frame::Frame(const QString &camera, size_t frameIndx, cv::Mat data, const QHash<Prediction::Type, PredictionList> &predictions, TimePoint timestamp)
    : m_camera(camera)
    , m_frameIndx(frameIndx)
    , m_data(data)
    , m_predictions(predictions)
    , m_timestamp(timestamp)
{}

Frame::Frame(const QString &camera, size_t frameIndx, const cv::Mat &data,
             const QHash<Prediction::Type, PredictionList> &predictions,
             Frame::TimePoint timestamp,
             const std::vector<PaddleOCR::OCRPredictResultList> &ocrResults,
             std::optional<ANPRSnapshot> anprSnapshot)
    : m_camera(camera),
    m_frameIndx(std::move(frameIndx)),
    m_data(data),
    m_timestamp(std::move(timestamp)),
    m_predictions(predictions),
    m_ocrResults(ocrResults),
    m_anprSnapshot(std::move(anprSnapshot))
{}

Frame Frame::clone() const
{
    std::shared_lock<std::shared_mutex> lock(m_mtx);

    return Frame(m_camera, m_frameIndx, m_data.clone(),
                 m_predictions, m_timestamp, m_ocrResults,
                 m_anprSnapshot);
}

QString Frame::id()
{
    return makeFrameId(m_camera, m_frameIndx);
}

QString Frame::camera() const
{
    return m_camera;
}

size_t Frame::frameIndx() const
{
    return m_frameIndx;
}

cv::Mat Frame::data() const {
    std::shared_lock<std::shared_mutex> lock(m_mtx);
    return m_data;
}

Frame::TimePoint Frame::timestamp() const {
    std::shared_lock<std::shared_mutex> lock(m_mtx);
    return m_timestamp;
}

const QHash<Prediction::Type, PredictionList> &Frame::predictions() const
{
    std::shared_lock<std::shared_mutex> lock(m_mtx);
    return m_predictions;
}

PredictionList Frame::predictions(const Prediction::Type target) const
{
    std::shared_lock<std::shared_mutex> lock(m_mtx);
    return m_predictions.value(target, {});
}

bool Frame::hasExpired() const
{
    return m_hasExpired.load(std::memory_order_acquire);
}

bool Frame::hasBeenProcessed() const
{
    return m_hasBeenProcessed.load(std::memory_order_acquire);
}

std::vector<PaddleOCR::OCRPredictResultList> Frame::ocrResults() const
{
    std::shared_lock<std::shared_mutex> lock(m_mtx);
    return m_ocrResults;
}

std::optional<ANPRSnapshot> Frame::anprSnapshot() const
{
    std::shared_lock<std::shared_mutex> lock(m_mtx);
    return m_anprSnapshot;
}

void Frame::setData(cv::Mat newData)
{
    std::unique_lock<std::shared_mutex> lock(m_mtx);
    m_data = newData;
}

void Frame::setTimestamp(const TimePoint &newTimestamp)
{
    std::unique_lock<std::shared_mutex> lock(m_mtx);
    m_timestamp = newTimestamp;
}

void Frame::setPredictions(const QHash<Prediction::Type, PredictionList> &newPredictions)
{
    std::unique_lock<std::shared_mutex> lock(m_mtx);
    m_predictions = newPredictions;
}

void Frame::setPredictions(QHash<Prediction::Type, PredictionList> &&newPredictions)
{
    std::unique_lock<std::shared_mutex> lock(m_mtx);
    m_predictions = std::move(newPredictions);
}

void Frame::setPredictions(const Prediction::Type target, const PredictionList &newPredictions)
{
    std::unique_lock<std::shared_mutex> lock(m_mtx);
    m_predictions.insertOrAssign(target, newPredictions);
}

void Frame::setPredictions(const Prediction::Type target, PredictionList &&newPredictions)
{
    std::unique_lock<std::shared_mutex> lock(m_mtx);
    m_predictions.insertOrAssign(target, std::move(newPredictions));
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

void Frame::setOcrResults(const std::vector<PaddleOCR::OCRPredictResultList> &newOcrResults)
{
    std::unique_lock<std::shared_mutex> lock(m_mtx);
    m_ocrResults = newOcrResults;
}

void Frame::setOcrResults(std::vector<PaddleOCR::OCRPredictResultList> &&newOcrResults)
{
    std::unique_lock<std::shared_mutex> lock(m_mtx);
    m_ocrResults = std::move(newOcrResults);
}

void Frame::setAnprSnapshot(std::optional<ANPRSnapshot> newAnprSnapshot)
{
    std::unique_lock<std::shared_mutex> lock(m_mtx);
    m_anprSnapshot = newAnprSnapshot;
}

QString Frame::makeFrameId(const QString &camera, size_t frameIndx)
{
    return QString("%1:%2").arg(camera).arg(frameIndx);
}

std::optional<std::tuple<QString, size_t> > Frame::splitFrameId(const QString &frameIndx)
{
    const auto parts = frameIndx.split(':', Qt::SkipEmptyParts);
    if (parts.size() != 2)
        return std::nullopt;

    bool ok;
    const size_t frame_indx = parts.at(1).toLongLong(&ok);
    if (!ok)
        return std::nullopt;
    const QString camera = parts.at(0);

    return std::tuple<QString, size_t>{ camera, frame_indx };
}
