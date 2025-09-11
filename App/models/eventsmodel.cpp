#include "eventsmodel.h"

#include <QtCore/QDateTime>
#include <QtSql/QSqlTableModel>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlError>
#include <QLoggingCategory>
#include <QUrl>
#include <qfileinfo.h>
#include <QDir>

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
    case StartTime: { // 4
        QDateTime datetime = QSqlTableModel::data(createIndex(index.row(), 4)).toDateTime();
        datetime.setTimeSpec(Qt::UTC);
        return datetime;
    }
    case EndTime: {  // 5
        QDateTime datetime = QSqlTableModel::data(createIndex(index.row(), 5)).toDateTime();
        datetime.setTimeSpec(Qt::UTC);
        return datetime;
    }
    case TimeInterval: {
        QDateTime start = QSqlTableModel::data(createIndex(index.row(), 4)).toDateTime();
        start.setTimeSpec(Qt::UTC);

        QDateTime end = QSqlTableModel::data(createIndex(index.row(), 5)).toDateTime();
        end.setTimeSpec(Qt::UTC);

        return formatRange(start, end);
    }
    case TrackerId: // 6
        return QSqlTableModel::data(createIndex(index.row(), 6));
    case TopScore:  // 7
        return static_cast<int>(QSqlTableModel::data(createIndex(index.row(), 7)).toFloat() * 100);
    case Score:     // 8
        return QSqlTableModel::data(createIndex(index.row(), 8));
    case Thumbnail: { // 11
        QString path = QSqlTableModel::data(createIndex(index.row(), 11)).toString();
        return QUrl::fromLocalFile(path);
    }
    case ReviewClip: // custom
        return QVariant();
    case LicensePlatePath: {
        QString path = QSqlTableModel::data(createIndex(index.row(), 11)).toString();
        QFileInfo file(path);
        QString file_name = QString("%1_lp.jpg").arg(file.baseName());
        path = file.dir().filePath(file_name);
        if (QFileInfo::exists(path))
            return QUrl::fromLocalFile(path);
        return QVariant();
    }
    case LicensePlateText:
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
        {TimeInterval, "timeinterval"},
        {TrackerId, "trackerid"},
        {TopScore, "topscore"},
        {Score, "score"},
        {Thumbnail, "thumbnail"},
        {ReviewClip, "reviewclip"},
        {LicensePlatePath, "lppath"},
        {LicensePlateText, "lptext"}
    };

    return roles;
}

QString EventsModel::formatRange(const QDateTime &startTime, const QDateTime &endTime) const
{
    QDate sDate = startTime.date();
    QDate eDate = endTime.date();
    QString sTime = startTime.toString("hh:mm:ss AP");
    QString eTime = endTime.toString("hh:mm:ss AP");

    // Same day
    if (sDate == eDate) {
        return QString("%1 %2 – %3")
            .arg(startTime.toString("ddd d"))
            .arg(sTime)
            .arg(eTime);
    }

    // Same year
    if (sDate.year() == eDate.year()) {
        if (sDate.month() == eDate.month()) {
            // Same month, different days
            return QString("%1 %2 – %3 %4")
                .arg(startTime.toString("ddd d"))
                .arg(sTime)
                .arg(endTime.toString("ddd d"))
                .arg(eTime);
        }
        else {
            // Same year, different months
            return QString("%1 %2 – %3 %4")
                .arg(startTime.toString("MMM d"))
                .arg(sTime)
                .arg(endTime.toString("MMM d"))
                .arg(eTime);
        }
    }

    // Different years
    return QString("%1 %2 – %3 %4")
        .arg(startTime.toString("MMM d yyyy"))
        .arg(sTime)
        .arg(endTime.toString("MMM d yyyy"))
        .arg(eTime);
}
