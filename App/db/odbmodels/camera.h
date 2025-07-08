#pragma once

#pragma db object
class Camera {
public:
    Camera() = default;

    long id() const;
    void setId(long newId);
    QString name() const;
    void setName(const QString &newName);
    QString hash() const;
    void setHash(const QString &newHash);

private:
    #pragma db id
    long m_id;
    QString m_name;
    QString m_hash;
};

inline long Camera::id() const
{
    return m_id;
}

inline void Camera::setId(long newId)
{
    m_id = newId;
}

inline QString Camera::name() const
{
    return m_name;
}

inline void Camera::setName(const QString &newName)
{
    m_name = newName;
}

inline QString Camera::hash() const
{
    return m_hash;
}

inline void Camera::setHash(const QString &newHash)
{
    m_hash = newHash;
}
