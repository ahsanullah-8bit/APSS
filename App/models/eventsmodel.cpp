#include "eventsmodel.h"

#include <QtSql/QSqlTableModel>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlError>
#include <QLoggingCategory>
#include <QUrl>

Q_STATIC_LOGGING_CATEGORY(logger, "apss.models.event")

EventsModel::EventsModel(const QSqlDatabase &db, QObject *parent)
    : QSqlTableModel(parent, db)
{
    // QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "event_reader_conn");
    // db.setDatabaseName("apss.db");

    // if (!db.open()) {
    //     qCCritical(logger) << "Failed to open a database connection:" << db.lastError().text();
    // } else {
    setTable("Event");
    // no edit strategy
    // no header
    select();
    // }
}

QVariant EventsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    switch(role) {
    case Id:        // 0
        return QSqlTableModel::data(createIndex(index.row(), 0));
    case Label:     // 1
        return QSqlTableModel::data(createIndex(index.row(), 1));
    case Camera:    // 3
        return QSqlTableModel::data(createIndex(index.row(), 3));
    case StartTime: // 4
        return QSqlTableModel::data(createIndex(index.row(), 4));
    case EndTime:   // 5
        return QSqlTableModel::data(createIndex(index.row(), 5));
    case TrackerId: // 6
        return QSqlTableModel::data(createIndex(index.row(), 6));
    case TopScore:  // 7
        return QSqlTableModel::data(createIndex(index.row(), 7));
    case Score:     // 8
        return QSqlTableModel::data(createIndex(index.row(), 8));
    case Thumbnail: { // 11
        QString path = QSqlTableModel::data(createIndex(index.row(), 11)).toString();
        return QUrl::fromLocalFile(path);
    }
    case ReviewClip: // custom
        return QVariant();
    }

    return QVariant();
}

QHash<int, QByteArray> EventsModel::roleNames() const
{
    static QHash<int, QByteArray> roles = {
        {Id, "id"},
        {Label, "label"},
        {SubLabel, "sublabel"},
        {Camera, "camera"},
        {StartTime, "starttime"},
        {EndTime, "endtime"},
        {TrackerId, "trackerid"},
        {TopScore, "topscore"},
        {Score, "score"},
        {Thumbnail, "thumbnail"},
        {ReviewClip, "reviewclip"}
    };

    return roles;
}
