#pragma once

// Hide includes from ODB_COMPILER
// #ifndef ODB_COMPILER
// #include <QtCore/QString>
// #include <QtCore/QDateTime>
// #endif

#pragma db object
class Event
{
public:
    Event();
    QString id() const;
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
    // bool hasSnapshot() const;
    // QString region() const;
    // QString box() const;
    // long area() const;
    // bool retainIndefinitely() const;
    QString data() const;
    int trackerId() const;

    void setId(const QString &newId);
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
    // void setHasSnapshot(bool newHasSnapshot);
    // void setRegion(const QString &newRegion);
    // void setBox(const QString &newBox);
    // void setArea(long newArea);
    // void setRetainIndefinitely(bool newRetainIndefinitely);
    void setData(const QString &newData);
    void setTrackerId(int newTrackerId);

private:
    #pragma db id
    QString m_id;
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
    // bool m_hasSnapshot = false;
    // QString m_region;
    // QString m_box;
    // long m_area = 0l;
    // bool m_retainIndefinitely = false;
    QString m_data;
};

inline Event::Event() {}

inline QString Event::id() const
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

// inline bool Event::hasSnapshot() const
// {
//     return m_hasSnapshot;
// }

// inline QString Event::region() const
// {
//     return m_region;
// }

// inline QString Event::box() const
// {
//     return m_box;
// }

// inline bool Event::retainIndefinitely() const
// {
//     return m_retainIndefinitely;
// }

inline QString Event::data() const
{
    return m_data;
}

inline int Event::trackerId() const
{
    return m_trackerId;
}

inline void Event::setId(const QString &newId)
{
    if (m_id == newId)
        return;
    m_id = newId;
    // emit idChanged(m_id);
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

// inline void Event::setHasSnapshot(bool newHasSnapshot)
// {
//     if (m_hasSnapshot == newHasSnapshot)
//         return;
//     m_hasSnapshot = newHasSnapshot;
//     // emit hasSnapshotChanged(m_hasSnapshot);
// }

// inline void Event::setRegion(const QString &newRegion)
// {
//     if (m_region == newRegion)
//         return;
//     m_region = newRegion;
//     // emit regionChanged(m_region);
// }

// inline void Event::setRetainIndefinitely(bool newRetainIndefinitely)
// {
//     if (m_retainIndefinitely == newRetainIndefinitely)
//         return;
//     m_retainIndefinitely = newRetainIndefinitely;
//     // emit retainIndefinitelyChanged(m_retainIndefinitely);
// }

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
