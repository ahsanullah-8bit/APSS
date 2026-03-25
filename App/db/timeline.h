#pragma once

#pragma db object
class Timeline
{
public:
    Timeline();
    long id() const;
    QDateTime timestamp() const;
    QString camera() const;
    QString source() const;
    QString sourceId() const;
    QString classType() const;
    QString data() const;

    void setId(long newId);
    void setTimestamp(const QDateTime &newTimestamp);
    void setCamera(const QString &newCamera);
    void setSource(const QString &newSource);
    void setSourceId(const QString &newSourceId);
    void setClassType(const QString &newClassType);
    void setData(const QString &newData);

private:
    #pragma db id
    long m_id;
    QDateTime m_timestamp;
    QString m_camera;
    QString m_source;
    QString m_sourceId;
    QString m_classType;
    QString m_data;
};

inline Timeline::Timeline() {}

inline long Timeline::id() const
{
    return m_id;
}

inline QDateTime Timeline::timestamp() const
{
    return m_timestamp;
}

inline QString Timeline::camera() const
{
    return m_camera;
}

inline QString Timeline::source() const
{
    return m_source;
}

inline QString Timeline::sourceId() const
{
    return m_sourceId;
}

inline QString Timeline::classType() const
{
    return m_classType;
}

inline QString Timeline::data() const
{
    return m_data;
}

inline void Timeline::setId(long newId)
{
    if (m_id == newId)
        return;
    m_id = newId;
    // emit idChanged(m_id);
}

inline void Timeline::setTimestamp(const QDateTime &newTimestamp)
{
    if (m_timestamp == newTimestamp)
        return;
    m_timestamp = newTimestamp;
    // emit timestampChanged(m_timestamp);
}

inline void Timeline::setCamera(const QString &newCamera)
{
    if (m_camera == newCamera)
        return;
    m_camera = newCamera;
    // emit cameraChanged(m_camera);
}

inline void Timeline::setSource(const QString &newSource)
{
    if (m_source == newSource)
        return;
    m_source = newSource;
    // emit sourceChanged(m_source);
}

inline void Timeline::setSourceId(const QString &newSourceId)
{
    if (m_sourceId == newSourceId)
        return;
    m_sourceId = newSourceId;
    // emit sourceIdChanged(m_sourceId);
}

inline void Timeline::setClassType(const QString &newClassType)
{
    if (m_classType == newClassType)
        return;
    m_classType = newClassType;
    // emit classTypeChanged(m_classType);
}

inline void Timeline::setData(const QString &newData)
{
    if (m_data == newData)
        return;
    m_data = newData;
    // emit dataChanged(m_data);
}


// Q_OBJECT
// Q_PROPERTY(long id READ id WRITE setId NOTIFY idChanged FINAL)
// Q_PROPERTY(std::string timestamp READ timestamp WRITE setTimestamp NOTIFY timestampChanged FINAL)
// Q_PROPERTY(std::string camera READ camera WRITE setCamera NOTIFY cameraChanged FINAL)
// Q_PROPERTY(std::string source READ source WRITE setSource NOTIFY sourceChanged FINAL)
// Q_PROPERTY(std::string sourceId READ sourceId WRITE setSourceId NOTIFY sourceIdChanged FINAL)
// Q_PROPERTY(std::string classType READ classType WRITE setClassType NOTIFY classTypeChanged FINAL)
// Q_PROPERTY(std::string data READ data WRITE setData NOTIFY dataChanged FINAL)


// signals:
//     void idChanged(long id);
//     void timestampChanged(std::string timestamp);
//     void cameraChanged(std::string camera);
//     void sourceChanged(std::string source);
//     void sourceIdChanged(std::string sourceId);
//     void classTypeChanged(std::string classType);
//     void dataChanged(std::string data);
