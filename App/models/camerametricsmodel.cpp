#include "camerametricsmodel.h"

CameraMetricsModel::CameraMetricsModel(QHash<QString, SharedCameraMetrics> &cameraMetrics,
                                       QObject *parent)
    : QAbstractListModel{parent}
    , m_cameraMetrics(cameraMetrics)
    , m_cameraMetKeys(cameraMetrics.keys())
{}

int CameraMetricsModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);

    return m_cameraMetrics.size();
}

QVariant CameraMetricsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_cameraMetKeys.size())
        return QVariant();

    const QString &key = m_cameraMetKeys[index.row()];

    switch (role) {
    case Name:
        return key;
        break;
    case VideoSink:
        return QVariant::fromValue<QVideoSink*>(m_cameraMetrics[key]->videoSink());
    default:
        break;
    }

    return QVariant();
}

bool CameraMetricsModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    Q_ASSERT(index.row() < m_cameraMetKeys.size());
    if (!index.isValid() || index.row() >= m_cameraMetKeys.size())
        return false;

    switch (role) {
    case VideoSink: {   // Only allow setting up VideoSink
        SharedCameraMetrics metrics = m_cameraMetrics[m_cameraMetKeys[index.row()]];
        QVideoSink *sink = value.value<QVideoSink *>();
        Q_ASSERT(metrics);
        Q_ASSERT(sink);
        metrics->setVideoSink(sink);
        break;
    }
    default:
        return false;
    };

    return true;
}

QHash<int, QByteArray> CameraMetricsModel::roleNames() const
{
    static QHash<int, QByteArray> roles = { { Name, "name" }, { VideoSink, "videosink" } };
    return roles;
}

#include "moc_camerametricsmodel.cpp"
