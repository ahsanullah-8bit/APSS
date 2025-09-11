#pragma once

#include <QAbstractListModel>
#include <QtSql/QSqlTableModel>

class EventsModel : public QSqlTableModel
{
public:
    enum EventRoles {
        Id = Qt::UserRole + 1,
        Label,
        SubLabel,
        Camera,
        StartTime,
        EndTime,
        TimeInterval,
        TrackerId,
        TopScore,
        Score,
        FalsePositive,
        Zones,
        Thumbnail,
        HasClip,
        Data,
        ReviewClip,
        LicensePlatePath,
        LicensePlateText
    };

    explicit EventsModel(const QSqlDatabase &db, QObject *parent = nullptr);

    // QAbstractItemModel interface
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE QString formatRange(const QDateTime &startTime, const QDateTime &endTime) const;
};
