#include <QList>
#include <QLoggingCategory>

#include "framemanager.h"

#include "frame.h"

Q_STATIC_LOGGING_CATEGORY(apss_frame_manager, "apss.utils.frame_mngr")

FrameManager &FrameManager::instance()
{
    static FrameManager manager;
    return manager;
}

void FrameManager::setMaxFramesPerCamera(const QString &camera, int maxFrames)
{
    m_maxFramesPerCamera.insert_or_assign(camera, maxFrames);
    Q_ASSERT(m_maxFramesPerCamera.value(camera));

    InternalFrameStore_t::accessor accessor;
    if (m_frameStore.insert(accessor, std::make_pair(camera.toStdString(), std::vector<cv::Mat>()))) {
        accessor->second.assign(m_maxFramesPerCamera.value(camera, maxFrames), cv::Mat());
    }
}

void FrameManager::write(const QString &frameId, cv::Mat img)
{
    const auto parts = Frame::splitFrameId(frameId);
    if (!parts)
        return;

    const auto &[camera, frame_indx] = parts.value();
    const int max_frames = m_maxFramesPerCamera.value(camera, 0);
    if (max_frames < 1)
        return;
\
    InternalFrameStore_t::accessor accessor;
    if (m_frameStore.find(accessor, camera.toStdString())) {
        accessor->second.at(frame_indx % max_frames) = img;
    } else {

    }
}

std::optional<cv::Mat> FrameManager::get(const QString &frameId)
{
    const auto parts = Frame::splitFrameId(frameId);
    if (!parts)
        return std::nullopt;

    const auto &[camera, frame_indx] = parts.value();
    const int max_frames = m_maxFramesPerCamera.value(camera, 0);
    if (max_frames < 1)
        return std::nullopt;

    InternalFrameStore_t::const_accessor accessor;
    if (m_frameStore.find(accessor, camera.toStdString())) {
        return accessor->second.at(frame_indx % max_frames);
    }

    return std::nullopt;
}

bool FrameManager::retire(const QString &frameId)
{
    const auto parts = Frame::splitFrameId(frameId);
    if (!parts)
        return false;

    const auto &[camera, frame_indx] = parts.value();
    const int max_frames = m_maxFramesPerCamera.value(camera, 0);
    if (max_frames < 1)
        return false;

    InternalFrameStore_t::accessor accessor;
    if (m_frameStore.find(accessor, camera.toStdString())) {
        accessor->second.at(frame_indx % max_frames) = cv::Mat();
        return true;
    }

    return false;
}

FrameManager::FrameManager()
{}
