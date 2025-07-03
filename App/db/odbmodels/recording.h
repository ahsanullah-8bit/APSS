#pragma once

// Hide includes from ODB_COMPILER
#ifndef ODB_COMPILER
#include <QtCore/QString>
#include <QtCore/QDateTime>
#endif

#pragma db object
class Recording
{
public:
    Recording();
    QString id() const;
    QString camera() const;
    QString path() const;
    QDateTime startTime() const;
    QDateTime endTime() const;
    long motion() const;
    float duration() const;
    long objects() const;
    long dBFS() const;
    float segmentSize() const;
    long regions() const;

    void setId(const QString &newId);
    void setCamera(const QString &newCamera);
    void setPath(const QString &newPath);
    void setStartTime(const QDateTime &newStartTime);
    void setEndTime(const QDateTime &newEndTime);
    void setDuration(float newDuration);
    void setMotion(long newMotion);
    void setObjects(long newObjects);
    void setDBFS(long newDBFS);
    void setSegmentSize(float newSegmentSize);
    void setRegions(long newRegions);

private:
    #pragma db id
    QString m_id;
    QString m_camera;
    QString m_path;
    QDateTime m_startTime;
    QDateTime m_endTime;
    float m_duration;
    long m_motion;
    long m_objects;
    long m_dBFS;
    float m_segmentSize;
    long m_regions;
};

inline Recording::Recording() {}

inline QString Recording::id() const
{
    return m_id;
}

inline QString Recording::camera() const
{
    return m_camera;
}

inline QString Recording::path() const
{
    return m_path;
}

inline QDateTime Recording::startTime() const
{
    return m_startTime;
}

inline QDateTime Recording::endTime() const
{
    return m_endTime;
}

inline long Recording::motion() const
{
    return m_motion;
}

inline float Recording::duration() const
{
    return m_duration;
}

inline long Recording::objects() const
{
    return m_objects;
}

inline long Recording::dBFS() const
{
    return m_dBFS;
}

inline float Recording::segmentSize() const
{
    return m_segmentSize;
}

inline long Recording::regions() const
{
    return m_regions;
}

inline void Recording::setId(const QString &newId)
{
    if (m_id == newId)
        return;
    m_id = newId;
    // emit idChanged(m_id);
}

inline void Recording::setCamera(const QString &newCamera)
{
    if (m_camera == newCamera)
        return;
    m_camera = newCamera;
    // emit cameraChanged(m_camera);
}

inline void Recording::setPath(const QString &newPath)
{
    if (m_path == newPath)
        return;
    m_path = newPath;
    // emit pathChanged(m_path);
}

inline void Recording::setStartTime(const QDateTime &newStartTime)
{
    if (m_startTime == newStartTime)
        return;
    m_startTime = newStartTime;
    // emit startTimeChanged(m_startTime);
}

inline void Recording::setEndTime(const QDateTime &newEndTime)
{
    if (m_endTime == newEndTime)
        return;
    m_endTime = newEndTime;
    // emit endTimeChanged(m_endTime);
}

inline void Recording::setDuration(float newDuration)
{
    if (m_duration == newDuration)
        return;
    m_duration = newDuration;
    // emit durationChanged(m_duration);
}

inline void Recording::setMotion(long newMotion)
{
    if (m_motion == newMotion)
        return;
    m_motion = newMotion;
    // emit motionChanged(m_motion);
}

inline void Recording::setObjects(long newObjects)
{
    if (m_objects == newObjects)
        return;
    m_objects = newObjects;
    // emit objectsChanged(m_objects);
}

inline void Recording::setDBFS(long newDBFS)
{
    if (m_dBFS == newDBFS)
        return;
    m_dBFS = newDBFS;
    // emit dBFSChanged(m_dBFS);
}

inline void Recording::setSegmentSize(float newSegmentSize)
{
    if (m_segmentSize == newSegmentSize)
        return;
    m_segmentSize = newSegmentSize;
    // emit segmentSizeChanged(m_segmentSize);
}

inline void Recording::setRegions(long newRegions)
{
    if (m_regions == newRegions)
        return;
    m_regions = newRegions;
    // emit regionsChanged(m_regions);
}

// Q_OBJECT
// Q_PROPERTY(std::string id READ id WRITE setId NOTIFY idChanged FINAL)
// Q_PROPERTY(std::string camera READ camera WRITE setCamera NOTIFY cameraChanged FINAL)
// Q_PROPERTY(std::string path READ path WRITE setPath NOTIFY pathChanged FINAL)
// Q_PROPERTY(std::string startTime READ startTime WRITE setStartTime NOTIFY startTimeChanged FINAL)
// Q_PROPERTY(std::string endTime READ endTime WRITE setEndTime NOTIFY endTimeChanged FINAL)
// Q_PROPERTY(float duration READ duration WRITE setDuration NOTIFY durationChanged FINAL)
// Q_PROPERTY(long motion READ motion WRITE setMotion NOTIFY motionChanged FINAL)
// Q_PROPERTY(long objects READ objects WRITE setObjects NOTIFY objectsChanged FINAL)
// Q_PROPERTY(long dBFS READ dBFS WRITE setDBFS NOTIFY dBFSChanged FINAL)
// Q_PROPERTY(float segmentSize READ segmentSize WRITE setSegmentSize NOTIFY segmentSizeChanged FINAL)
// Q_PROPERTY(long regions READ regions WRITE setRegions NOTIFY regionsChanged FINAL)

// signals:
//     void idChanged(std::string id);
//     void cameraChanged(std::string camera);
//     void pathChanged(std::string path);
//     void startTimeChanged(std::string startTime);
//     void endTimeChanged(std::string endTime);
//     void durationChanged(float duration);
//     void motionChanged(long motion);
//     void objectsChanged(long objects);
//     void dBFSChanged(long dBFS);
//     void segmentSizeChanged(float segmentSize);
//     void regionsChanged(long regions);
