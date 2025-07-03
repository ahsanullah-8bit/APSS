#include "camerawatchdog.h"

CameraWatchdog::CameraWatchdog(const QString &cameraName, const CameraConfig &config, QSharedPointer<SharedFrameBoundedQueue> inFrameQueue, QSharedPointer<SharedFrameBoundedQueue> outFrameQueue, int cameraFPS, int skippedFPS, QObject *parent)
    : QThread{parent}
{}

int CameraWatchdog::cameraFPS() const
{
    return m_cameraFPS.load(std::memory_order_relaxed);
}

int CameraWatchdog::skippedFPS() const
{
    return m_skippedFPS.load(std::memory_order_relaxed);
}

QString CameraWatchdog::cameraName() const
{
    return m_cameraName;
}

QSharedPointer<QThread> CameraWatchdog::captureThread() const
{
    return m_captureThread;
}

void CameraWatchdog::setCameraFPS(int newCameraFPS)
{
    int expected = m_cameraFPS.load(std::memory_order_acquire);
    if (expected != newCameraFPS)
    {
        m_cameraFPS.compare_exchange_strong(expected, newCameraFPS, std::memory_order_acq_rel);
        Q_EMIT cameraFPSChanged(newCameraFPS);
    }
}

void CameraWatchdog::setSkippedFPS(int newSkippedFPS)
{
    int expected = m_skippedFPS.load(std::memory_order_acquire);
    if (expected != newSkippedFPS) {
        m_skippedFPS.compare_exchange_strong(expected, newSkippedFPS, std::memory_order_acq_rel);
        Q_EMIT skippedFPSChanged(newSkippedFPS);
    }
}

void CameraWatchdog::setCameraName(const QString &newCameraName)
{
    if (m_cameraName == newCameraName)
        return;
    m_cameraName = newCameraName;
    Q_EMIT cameraNameChanged(m_cameraName);
}

void CameraWatchdog::setCaptureThread(QSharedPointer<QThread> newCaptureThread)
{
    if (m_captureThread == newCaptureThread)
        return;
    m_captureThread = newCaptureThread;
    Q_EMIT captureThreadChanged(m_captureThread);
}

void CameraWatchdog::run()
{

}

QSharedPointer<SharedFrameBoundedQueue> CameraWatchdog::inFrameQueue() const
{
    return m_inFrameQueue;
}

void CameraWatchdog::setInFrameQueue(QSharedPointer<SharedFrameBoundedQueue> newInFrameQueue)
{
    if (m_inFrameQueue == newInFrameQueue)
        return;
    m_inFrameQueue = newInFrameQueue;
    Q_EMIT inFrameQueueChanged(m_inFrameQueue);
}

QSharedPointer<SharedFrameBoundedQueue> CameraWatchdog::outFrameQueue() const
{
    return m_outFrameQueue;
}

void CameraWatchdog::setOutFrameQueue(QSharedPointer<SharedFrameBoundedQueue> newOutFrameQueue)
{
    if (m_outFrameQueue == newOutFrameQueue)
        return;
    m_outFrameQueue = newOutFrameQueue;
    Q_EMIT outFrameQueueChanged(m_outFrameQueue);
}
