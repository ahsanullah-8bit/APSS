#include "framemanager.h"

#include <QList>

#include "frame.h"

FrameManager &FrameManager::instance(int maxFramesPerCamera)
{
    static FrameManager manager(maxFramesPerCamera);
    return manager;
}

void FrameManager::write(const QString &frameId, cv::Mat img)
{
    const auto parts = Frame::splitFrameId(frameId);
    if (!parts)
        return;

    const auto &[camera, frame_indx] = parts.value();
\
    InternalFrameStore_t::accessor accessor;
    if (m_frameStore.insert(accessor, std::make_pair(camera.toStdString(), tbb::concurrent_vector<cv::Mat>()))) {
        accessor->second.assign(m_maxFramesPerCamera, cv::Mat());
    }

    Q_ASSERT(accessor->second.size() == m_maxFramesPerCamera);
    accessor->second.at(frame_indx % m_maxFramesPerCamera) = img;
}

std::optional<cv::Mat> FrameManager::get(const QString &frameId)
{
    const auto parts = Frame::splitFrameId(frameId);
    if (!parts)
        return std::nullopt;

    const auto &[camera, frame_indx] = parts.value();

    InternalFrameStore_t::const_accessor accessor;
    if (m_frameStore.find(accessor, camera.toStdString())) {
        return accessor->second.at(frame_indx % m_maxFramesPerCamera);
    }

    return std::nullopt;
}

bool FrameManager::retire(const QString &frameId)
{
    const auto parts = Frame::splitFrameId(frameId);
    if (!parts)
        return false;

    const auto &[camera, frame_indx] = parts.value();
    InternalFrameStore_t::accessor accessor;
    if (m_frameStore.find(accessor, camera.toStdString())) {
        accessor->second.at(frame_indx % m_maxFramesPerCamera) = cv::Mat();
        return true;
    }

    return false;
}

FrameManager::FrameManager(int maxFramesPerCamera)
    : m_maxFramesPerCamera(maxFramesPerCamera)
{}
