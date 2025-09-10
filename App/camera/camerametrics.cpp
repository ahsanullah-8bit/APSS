#include "camerametrics.h"

CameraMetrics::CameraMetrics(const QString &name, bool isPullBased, QObject *parent)
    : QObject(parent)
    , m_frameQueue(new SharedFrameBoundedQueue())
    , m_name(name)
    , m_isPullBased(isPullBased)
{
    m_frameQueue->set_capacity(2);
}

QString CameraMetrics::name() const
{
    return m_name;
}

double CameraMetrics::cameraFPS() const
{
    return m_cameraFPS.load(std::memory_order_acquire);
}

double CameraMetrics::detectionFPS() const
{
    return m_detectionFPS.load(std::memory_order_acquire);
}

double CameraMetrics::processFPS() const
{
    return m_processFPS.load(std::memory_order_acquire);
}

double CameraMetrics::skippedFPS() const
{
    return m_skippedFPS.load(std::memory_order_acquire);
}

int CameraMetrics::detectionFrame() const
{
    return m_detectionFrame.load(std::memory_order_acquire);
}

int CameraMetrics::readStart() const
{
    return m_readStart.load(std::memory_order_acquire);
}

QVideoSink *CameraMetrics::videoSink() const
{
    return m_videoSink.load(std::memory_order_acquire);
}

QSharedPointer<PacketRingBuffer> CameraMetrics::packetRingBuffer() const
{
    return m_packetRingBuffer;
}

QSharedPointer<SharedFrameBoundedQueue> CameraMetrics::frameQueue() const
{
    return m_frameQueue;
}

QSharedPointer<QThread> CameraMetrics::thread() const
{
    return m_thread;
}

QSharedPointer<QThread> CameraMetrics::captureThread() const
{
    return m_captureThread;
}

bool CameraMetrics::isPullBased() const
{
    return m_isPullBased;
}

void CameraMetrics::setCameraFPS(double newCameraFPS)
{
    // Quick check to avoid CAS when unnecessary
    double current = m_cameraFPS.load(std::memory_order_relaxed);
    if (current == newCameraFPS) return;

    // CAS loop for atomic update
    while (!m_cameraFPS.compare_exchange_weak(
        current, newCameraFPS,
        std::memory_order_release,  // Success: publish to other threads
        std::memory_order_relaxed   // Failure: just reload current value
        )) {
        // Check if another thread already set our target value
        if (current == newCameraFPS) return;
    }

    // Only emit if WE changed the value
    Q_EMIT cameraFPSChanged(newCameraFPS);

    // double expected = m_cameraFPS.load(std::memory_order_acquire);
    // if (expected != newCameraFPS) {
    //     m_cameraFPS.compare_exchange_strong(expected, newCameraFPS, std::memory_order_acq_rel);
    //     Q_EMIT cameraFPSChanged(newCameraFPS);
    // }
}

void CameraMetrics::setDetectionFPS(double newDetectionFPS)
{
    double current = m_detectionFPS.load(std::memory_order_relaxed);
    if (current == newDetectionFPS)
        return;

    while (!m_detectionFPS.compare_exchange_weak(
        current, newDetectionFPS,
        std::memory_order_release,
        std::memory_order_relaxed
        )) {

        if (current == newDetectionFPS)
            return;
    }

    Q_EMIT detectionFPSChanged(newDetectionFPS);
}

void CameraMetrics::setProcessFPS(double newProcessFPS)
{
    double current = m_processFPS.load(std::memory_order_relaxed);
    if (current == newProcessFPS)
        return;

    while (!m_processFPS.compare_exchange_weak(
        current, newProcessFPS,
        std::memory_order_release,
        std::memory_order_relaxed
        )) {

        if (current == newProcessFPS)
            return;
    }

    Q_EMIT processFPSChanged(newProcessFPS);
}

void CameraMetrics::setSkippedFPS(double newSkippedFPS)
{
    double current = m_skippedFPS.load(std::memory_order_relaxed);
    if (current == newSkippedFPS)
        return;

    while (!m_skippedFPS.compare_exchange_weak(
        current, newSkippedFPS,
        std::memory_order_release,
        std::memory_order_relaxed
        )) {

        if (current == newSkippedFPS)
            return;
    }

    Q_EMIT skippedFPSChanged(newSkippedFPS);
}

void CameraMetrics::setDetectionFrame(int newDetectionFrame)
{
    int current = m_detectionFrame.load(std::memory_order_relaxed);
    if (current == newDetectionFrame)
        return;

    while (!m_detectionFrame.compare_exchange_weak(
        current, newDetectionFrame,
        std::memory_order_release,
        std::memory_order_relaxed
        )) {

        if (current == newDetectionFrame)
            return;
    }

    Q_EMIT detectionFrameChanged(newDetectionFrame);
}

void CameraMetrics::setReadStart(int newReadStart)
{
    int current = m_readStart.load(std::memory_order_relaxed);
    if (current == newReadStart)
        return;

    while (!m_readStart.compare_exchange_weak(
        current, newReadStart,
        std::memory_order_release,
        std::memory_order_relaxed
        )) {

        if (current == newReadStart)
            return;
    }

    Q_EMIT readStartChanged(newReadStart);
}

void CameraMetrics::setVideoSink(QVideoSink *newVideoSink)
{
    QVideoSink *current = m_videoSink.load(std::memory_order_relaxed);
    if (current == newVideoSink)
        return;

    while (!m_videoSink.compare_exchange_weak(
        current, newVideoSink,
        std::memory_order_release,
        std::memory_order_relaxed
        )) {

        if (current == newVideoSink)
            return;
    }

    emit videoSinkChanged(newVideoSink);
}

void CameraMetrics::setPacketRingBuffer(QSharedPointer<PacketRingBuffer> newPacketRingBuffer)
{
    m_packetRingBuffer = newPacketRingBuffer;
}

void CameraMetrics::setFrameQueue(QSharedPointer<SharedFrameBoundedQueue> newFrameQueue)
{
    // TODO: Make these thread-safe.
    // I don't think they need it, as they are set only once, by the main thread.
    if (m_frameQueue == newFrameQueue)
        return;
    m_frameQueue = newFrameQueue;
    Q_EMIT frameQueueChanged(m_frameQueue);
}

void CameraMetrics::setThread(QSharedPointer<QThread> newThread)
{
    if (m_thread == newThread)
        return;
    m_thread = newThread;
    Q_EMIT threadChanged(m_thread);
}

void CameraMetrics::setCaptureThread(QSharedPointer<QThread> newCaptureThread)
{
    if (m_captureThread == newCaptureThread)
        return;
    m_captureThread = newCaptureThread;
    Q_EMIT captureThreadChanged(m_captureThread);
}

#include "moc_camerametrics.cpp"
