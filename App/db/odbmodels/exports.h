#pragma once

#pragma db object
class Exports   // Named Exports not Export, because it conflicts with Odb's type.
{
public:
    Exports();
    QString id() const;
    QString camera() const;
    QString name() const;
    QDateTime date() const;
    QString videoPath() const;
    QString thumbPath() const;
    bool inProgress() const;

    void setCamera(const QString &newCamera);
    void setId(const QString &newId);
    void setName(const QString &newName);
    void setDate(const QDateTime &newDate);
    void setVideoPath(const QString &newVideoPath);
    void setThumbPath(const QString &newThumbPath);
    void setInProgress(bool newInProgress);

private:
    #pragma db id
    QString m_id;
    QString m_camera;
    QString m_name;
    QDateTime m_date;
    QString m_videoPath;
    QString m_thumbPath;
    bool m_inProgress;
};

inline Exports::Exports() {}

inline QString Exports::id() const
{
    return m_id;
}

inline QString Exports::camera() const
{
    return m_camera;
}

inline QString Exports::name() const
{
    return m_name;
}

inline QDateTime Exports::date() const
{
    return m_date;
}

inline QString Exports::videoPath() const
{
    return m_videoPath;
}

inline QString Exports::thumbPath() const
{
    return m_thumbPath;
}

inline bool Exports::inProgress() const
{
    return m_inProgress;
}

inline void Exports::setCamera(const QString &newCamera)
{
    if (m_camera == newCamera)
        return;
    m_camera = newCamera;
    // emit cameraChanged(m_camera);
}

inline void Exports::setId(const QString &newId)
{
    if (m_id == newId)
        return;
    m_id = newId;
    // emit idChanged(m_id);
}

inline void Exports::setName(const QString &newName)
{
    if (m_name == newName)
        return;
    m_name = newName;
    // emit nameChanged(m_name);
}

inline void Exports::setDate(const QDateTime &newDate)
{
    if (m_date == newDate)
        return;
    m_date = newDate;
    // emit dateChanged(m_date);
}

inline void Exports::setVideoPath(const QString &newVideoPath)
{
    if (m_videoPath == newVideoPath)
        return;
    m_videoPath = newVideoPath;
    // emit videoPathChanged(m_videoPath);
}

inline void Exports::setThumbPath(const QString &newThumbPath)
{
    if (m_thumbPath == newThumbPath)
        return;
    m_thumbPath = newThumbPath;
    // emit thumbPathChanged(m_thumbPath);
}

inline void Exports::setInProgress(bool newInProgress)
{
    if (m_inProgress == newInProgress)
        return;
    m_inProgress = newInProgress;
    // emit inProgressChanged(m_inProgress);
}

// Q_OBJECT
// Q_PROPERTY(std::string id READ id WRITE setId NOTIFY idChanged FINAL)
// Q_PROPERTY(std::string camera READ camera WRITE setCamera NOTIFY cameraChanged FINAL)
// Q_PROPERTY(std::string name READ name WRITE setName NOTIFY nameChanged FINAL)
// Q_PROPERTY(std::string date READ date WRITE setDate NOTIFY dateChanged FINAL)
// Q_PROPERTY(std::string videoPath READ videoPath WRITE setVideoPath NOTIFY videoPathChanged FINAL)
// Q_PROPERTY(std::string thumbPath READ thumbPath WRITE setThumbPath NOTIFY thumbPathChanged FINAL)
// Q_PROPERTY(bool inProgress READ inProgress WRITE setInProgress NOTIFY inProgressChanged FINAL)

// signals:
//     void idChanged(std::string id);
//     void cameraChanged(std::string camera);
//     void nameChanged(std::string name);
//     void dateChanged(std::string date);
//     void videoPathChanged(std::string videoPath);
//     void thumbPathChanged(std::string thumbPath);
//     void inProgressChanged(bool inProgress);
