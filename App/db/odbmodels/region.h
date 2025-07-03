#pragma once

// Hide includes from ODB_COMPILER
#ifndef ODB_COMPILER
#include <QtCore/QString>
#include <QtCore/QDateTime>
#endif

#pragma db object
class Region
{
public:
    Region();
    QString camera() const;
    QString grid() const;
    QDateTime lastUpdate() const;

    void setCamera(const QString &newCamera);
    void setGrid(const QString &newGrid);
    void setLastUpdate(const QDateTime &newLastUpdate);

private:
    #pragma db id
    QString m_camera;
    QString m_grid;
    QDateTime m_lastUpdate;
};

inline Region::Region() {}

inline QString Region::camera() const
{
    return m_camera;
}

inline QString Region::grid() const
{
    return m_grid;
}

inline QDateTime Region::lastUpdate() const
{
    return m_lastUpdate;
}

inline void Region::setCamera(const QString &newCamera)
{
    if (m_camera == newCamera)
        return;
    m_camera = newCamera;
    // emit cameraChanged(m_camera);
}

inline void Region::setGrid(const QString &newGrid)
{
    if (m_grid == newGrid)
        return;
    m_grid = newGrid;
    // emit gridChanged(m_grid);
}

inline void Region::setLastUpdate(const QDateTime &newLastUpdate)
{
    if (m_lastUpdate == newLastUpdate)
        return;
    m_lastUpdate = newLastUpdate;
    // emit lastUpdateChanged(m_lastUpdate);
}

// Q_OBJECT
// Q_PROPERTY(std::string camera READ camera WRITE setCamera NOTIFY cameraChanged FINAL)
// Q_PROPERTY(std::string grid READ grid WRITE setGrid NOTIFY gridChanged FINAL)
// Q_PROPERTY(std::string lastUpdate READ lastUpdate WRITE setLastUpdate NOTIFY lastUpdateChanged FINAL)

// signals:
//     void cameraChanged(std::string camera);
//     void gridChanged(std::string grid);
//     void lastUpdateChanged(std::string lastUpdate);
