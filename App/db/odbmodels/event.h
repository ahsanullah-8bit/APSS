#pragma once

// Hide includes from ODB_COMPILER
#ifndef ODB_COMPILER
#include <QtCore/QString>
#include <QtCore/QDateTime>
#endif

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
    bool hasSnapshot() const;
    QString region() const;
    QString box() const;
    long area() const;
    bool retainIndefinitely() const;
    float ratio() const;
    QString plusId() const;
    QString modelHash() const;
    QString detectorType() const;
    QString modelType() const;
    QString data() const;

    void setId(const QString &newId);
    void setLabel(const QString &newLabel);
    void setSubLabel(const QString &newSubLabel);
    void setcamera(const QString &newCamera);
    void setStartTime(const QDateTime &newStartTime);
    void setEndTime(const QDateTime &newEndTime);
    void setTopScore(float newTopScore);
    void setScore(float newScore);
    void setFalsePositive(bool newFalsePositive);
    void setZones(const QString &newZones);
    void setThumbnail(const QString &newThumbnail);
    void setHasClip(bool newHasClip);
    void setHasSnapshot(bool newHasSnapshot);
    void setRegion(const QString &newRegion);
    void setBox(const QString &newBox);
    void setArea(long newArea);
    void setRetainIndefinitely(bool newRetainIndefinitely);
    void setRatio(float newRatio);
    void setPlusId(const QString &newPlusId);
    void setModelHash(const QString &newModelHash);
    void setDetectorType(const QString &newDetectorType);
    void setModelType(const QString &newModelType);
    void setData(const QString &newData);

private:
    #pragma db id
    QString m_id;
    QString m_label;
    QString m_subLabel;
    QString m_camera;
    QDateTime m_startTime;
    QDateTime m_endTime;
    float m_topScore;
    float m_score;
    bool m_falsePositive;
    QString m_zones;
    QString m_thumbnail;
    bool m_hasClip;
    bool m_hasSnapshot;
    QString m_region;
    QString m_box;
    long m_area;
    bool m_retainIndefinitely;
    float m_ratio;
    QString m_plusId;
    QString m_modelHash;
    QString m_detectorType;
    QString m_modelType;
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

inline bool Event::hasSnapshot() const
{
    return m_hasSnapshot;
}

inline QString Event::region() const
{
    return m_region;
}

inline QString Event::box() const
{
    return m_box;
}

inline long Event::area() const
{
    return m_area;
}

inline bool Event::retainIndefinitely() const
{
    return m_retainIndefinitely;
}

inline float Event::ratio() const
{
    return m_ratio;
}

inline QString Event::plusId() const
{
    return m_plusId;
}

inline QString Event::modelHash() const
{
    return m_modelHash;
}

inline QString Event::detectorType() const
{
    return m_detectorType;
}

inline QString Event::modelType() const
{
    return m_modelType;
}

inline QString Event::data() const
{
    return m_data;
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

inline void Event::setcamera(const QString &newCamera)
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

inline void Event::setHasSnapshot(bool newHasSnapshot)
{
    if (m_hasSnapshot == newHasSnapshot)
        return;
    m_hasSnapshot = newHasSnapshot;
    // emit hasSnapshotChanged(m_hasSnapshot);
}

inline void Event::setRegion(const QString &newRegion)
{
    if (m_region == newRegion)
        return;
    m_region = newRegion;
    // emit regionChanged(m_region);
}

inline void Event::setBox(const QString &newBox)
{
    if (m_box == newBox)
        return;
    m_box = newBox;
    // emit boxChanged(m_box);
}

inline void Event::setArea(long newArea)
{
    if (m_area == newArea)
        return;
    m_area = newArea;
    // emit areaChanged(m_area);
}

inline void Event::setRetainIndefinitely(bool newRetainIndefinitely)
{
    if (m_retainIndefinitely == newRetainIndefinitely)
        return;
    m_retainIndefinitely = newRetainIndefinitely;
    // emit retainIndefinitelyChanged(m_retainIndefinitely);
}

inline void Event::setRatio(float newRatio)
{
    if (m_ratio == newRatio)
        return;
    m_ratio = newRatio;
    // emit ratioChanged(m_ratio);
}

inline void Event::setPlusId(const QString &newPlusId)
{
    if (m_plusId == newPlusId)
        return;
    m_plusId = newPlusId;
    // emit plusIdChanged(m_plusId);
}

inline void Event::setModelHash(const QString &newModelHash)
{
    if (m_modelHash == newModelHash)
        return;
    m_modelHash = newModelHash;
    // emit modelHashChanged(m_modelHash);
}

inline void Event::setDetectorType(const QString &newDetectorType)
{
    if (m_detectorType == newDetectorType)
        return;
    m_detectorType = newDetectorType;
    // emit detectorTypeChanged(m_detectorType);
}

inline void Event::setModelType(const QString &newModelType)
{
    if (m_modelType == newModelType)
        return;
    m_modelType = newModelType;
    // emit modelTypeChanged(m_modelType);
}

inline void Event::setData(const QString &newData)
{
    if (m_data == newData)
        return;
    m_data = newData;
    // emit dataChanged(m_data);
}

// Q_OBJECT
// Q_PROPERTY(std::string id READ id WRITE setId NOTIFY idChanged FINAL)
// Q_PROPERTY(std::string label READ label WRITE setLabel NOTIFY labelChanged FINAL)
// Q_PROPERTY(std::string subLabel READ subLabel WRITE setSubLabel NOTIFY subLabelChanged FINAL)
// Q_PROPERTY(std::string camera READ camera WRITE setcamera NOTIFY cameraChanged FINAL)
// Q_PROPERTY(std::string startTime READ startTime WRITE setStartTime NOTIFY startTimeChanged FINAL)
// Q_PROPERTY(std::string endTime READ endTime WRITE setEndTime NOTIFY endTimeChanged FINAL)
// Q_PROPERTY(float topScore READ topScore WRITE setTopScore NOTIFY topScoreChanged FINAL)
// Q_PROPERTY(float score READ score WRITE setScore NOTIFY scoreChanged FINAL)
// Q_PROPERTY(bool falsePositive READ falsePositive WRITE setFalsePositive NOTIFY falsePositiveChanged FINAL)
// Q_PROPERTY(std::string zones READ zones WRITE setZones NOTIFY zonesChanged FINAL)
// Q_PROPERTY(std::string thumbnail READ thumbnail WRITE setThumbnail NOTIFY thumbnailChanged FINAL)
// Q_PROPERTY(bool hasClip READ hasClip WRITE setHasClip NOTIFY hasClipChanged FINAL)
// Q_PROPERTY(bool hasSnapshot READ hasSnapshot WRITE setHasSnapshot NOTIFY hasSnapshotChanged FINAL)
// Q_PROPERTY(std::string region READ region WRITE setRegion NOTIFY regionChanged FINAL)
// Q_PROPERTY(std::string box READ box WRITE setBox NOTIFY boxChanged FINAL)
// Q_PROPERTY(long area READ area WRITE setArea NOTIFY areaChanged FINAL)
// Q_PROPERTY(bool retainIndefinitely READ retainIndefinitely WRITE setRetainIndefinitely NOTIFY retainIndefinitelyChanged FINAL)
// Q_PROPERTY(float ratio READ ratio WRITE setRatio NOTIFY ratioChanged FINAL)
// Q_PROPERTY(std::string plusId READ plusId WRITE setPlusId NOTIFY plusIdChanged FINAL)
// Q_PROPERTY(std::string modelHash READ modelHash WRITE setModelHash NOTIFY modelHashChanged FINAL)
// Q_PROPERTY(std::string detectorType READ detectorType WRITE setDetectorType NOTIFY detectorTypeChanged FINAL)
// Q_PROPERTY(std::string modelType READ modelType WRITE setModelType NOTIFY modelTypeChanged FINAL)
// Q_PROPERTY(std::string data READ data WRITE setData NOTIFY dataChanged FINAL)


// signals:
//     void idChanged(std::string id);
//     void labelChanged(std::string label);
//     void subLabelChanged(std::string subLabel);
//     void cameraChanged(std::string camera);
//     void startTimeChanged(std::string startTime);
//     void endTimeChanged(std::string endTime);
//     void topScoreChanged(float topScore);
//     void scoreChanged(float score);
//     void falsePositiveChanged(bool falsePositive);
//     void zonesChanged(std::string zones);
//     void thumbnailChanged(std::string thumbnail);
//     void hasClipChanged(bool hasClip);
//     void hasSnapshotChanged(bool hasSnapshot);
//     void regionChanged(std::string region);
//     void boxChanged(std::string box);
//     void areaChanged(long area);
//     void retainIndefinitelyChanged(bool retainIndefinitely);
//     void ratioChanged(float ratio);
//     void plusIdChanged(std::string plusId);
//     void modelHashChanged(std::string modelHash);
//     void detectorTypeChanged(std::string detectorType);
//     void modelTypeChanged(std::string modelType);
//     void dataChanged(std::string data);
