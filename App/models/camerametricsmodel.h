#pragma once

#include <QAbstractListModel>

#include "camera/camerametrics.h"

class CameraMetricsModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum CameraMetricsDataRole {
        Name = Qt::UserRole + 1,
        VideoSink,
        CameraFPS,
        DetectionFPS,
        ProcessFPS,
        SkippedFPS
    };

    explicit CameraMetricsModel(QHash<QString, SharedCameraMetrics> &cameraMetrics,
                                QObject *parent = nullptr);

    // QAbstractItemModel interface
    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    QHash<int, QByteArray> roleNames() const override;

private:
    QHash<QString, SharedCameraMetrics> &m_cameraMetrics;
    QList<QString> m_cameraMetKeys;
};

using SharedCameraMetricsModel = QSharedPointer<CameraMetricsModel>;
