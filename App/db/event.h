#pragma once

// Hide includes from ODB_COMPILER
#include <cstddef>
#ifndef ODB_COMPILER
#include <QtCore/QString>
#include <QtCore/QDateTime>
#endif

#include <odb/forward.hxx>

#pragma db model version(1, 1)

#pragma db object
class Event
{
public:
    Event();
    size_t id() const;
    QString label() const;
    QString subLabel() const;
    QString camera() const;
    QDateTime startTime() const;
    QDateTime endTime() const;
    float topScore() const;
    float score() const;
    bool falsePositive() const;
    QString zones() const;
    QString thumbnail() const;
    bool hasClip() const;
    QString data() const;
    int trackerId() const;

    void setLabel(const QString &newLabel);
    void setSubLabel(const QString &newSubLabel);
    void setCamera(const QString &newCamera);
    void setStartTime(const QDateTime &newStartTime);
    void setEndTime(const QDateTime &newEndTime);
    void setTopScore(float newTopScore);
    void setScore(float newScore);
    void setFalsePositive(bool newFalsePositive);
    void setZones(const QString &newZones);
    void setThumbnail(const QString &newThumbnail);
    void setHasClip(bool newHasClip);
    void setData(const QString &newData);
    void setTrackerId(int newTrackerId);

private:
    friend odb::access;

    #pragma db id auto
    size_t m_id;
    QString m_label;
    QString m_subLabel;
    QString m_camera;
    QDateTime m_startTime;
    QDateTime m_endTime;
    int m_trackerId = -1;
    float m_topScore = 0.0f;
    float m_score = 0.0f;
    bool m_falsePositive = false;
    QString m_zones;
    QString m_thumbnail;
    bool m_hasClip = false;
    QString m_data;
};

inline Event::Event() {}

inline size_t Event::id() const
{
    return m_id;
}

inline QString Event::label() const
{
    return m_label;
}

inline QString Event::subLabel() const
{
    return m_subLabel;
}

inline QString Event::camera() const
{
    return m_camera;
}

inline QDateTime Event::startTime() const
{
    return m_startTime;
}

inline QDateTime Event::endTime() const
{
    return m_endTime;
}

inline float Event::topScore() const
{
    return m_topScore;
}

inline float Event::score() const
{
    return m_score;
}

inline bool Event::falsePositive() const
{
    return m_falsePositive;
}

inline QString Event::zones() const
{
    return m_zones;
}

inline QString Event::thumbnail() const
{
    return m_thumbnail;
}

inline bool Event::hasClip() const
{
    return m_hasClip;
}

inline QString Event::data() const
{
    return m_data;
}

inline int Event::trackerId() const
{
    return m_trackerId;
}

inline void Event::setLabel(const QString &newLabel)
{
    if (m_label == newLabel)
        return;
    m_label = newLabel;
    // emit labelChanged(m_label);
}

inline void Event::setSubLabel(const QString &newSubLabel)
{
    if (m_subLabel == newSubLabel)
        return;
    m_subLabel = newSubLabel;
    // emit subLabelChanged(m_subLabel);
}

inline void Event::setCamera(const QString &newCamera)
{
    if (m_camera == newCamera)
        return;
    m_camera = newCamera;
    // emit cameraChanged(m_camera);
}

inline void Event::setStartTime(const QDateTime &newStartTime)
{
    if (m_startTime == newStartTime)
        return;
    m_startTime = newStartTime;
    // emit startTimeChanged(m_startTime);
}

inline void Event::setEndTime(const QDateTime &newEndTime)
{
    if (m_endTime == newEndTime)
        return;
    m_endTime = newEndTime;
    // emit endTimeChanged(m_endTime);
}

inline void Event::setTopScore(float newTopScore)
{
    if (m_topScore == newTopScore)
        return;
    m_topScore = newTopScore;
    // emit topScoreChanged(m_topScore);
}

inline void Event::setScore(float newScore)
{
    if (m_score == newScore)
        return;
    m_score = newScore;
    // emit scoreChanged(m_score);
}

inline void Event::setFalsePositive(bool newFalsePositive)
{
    if (m_falsePositive == newFalsePositive)
        return;
    m_falsePositive = newFalsePositive;
    // emit falsePositiveChanged(m_falsePositive);
}

inline void Event::setZones(const QString &newZones)
{
    if (m_zones == newZones)
        return;
    m_zones = newZones;
    // emit zonesChanged(m_zones);
}

inline void Event::setThumbnail(const QString &newThumbnail)
{
    if (m_thumbnail == newThumbnail)
        return;
    m_thumbnail = newThumbnail;
    // emit thumbnailChanged(m_thumbnail);
}

inline void Event::setHasClip(bool newHasClip)
{
    if (m_hasClip == newHasClip)
        return;
    m_hasClip = newHasClip;
    // emit hasClipChanged(m_hasClip);
}

inline void Event::setData(const QString &newData)
{
    if (m_data == newData)
        return;
    m_data = newData;
    // emit dataChanged(m_data);
}

inline void Event::setTrackerId(int newTrackerId)
{
    m_trackerId = newTrackerId;
}
