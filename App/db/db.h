#pragma once

// Helper function to force `ODB's read custom time` to UTC, by appending Z at the end explicitly and re-read the thing again.
inline QDateTime forceToUTC(const QDateTime &dateTime, Qt::TimeSpec timeSpec = Qt::UTC) {
    QString local_time = dateTime.toString("yyyy-MM-ddTHH:mm:ss.zzz");
    if (timeSpec == Qt::UTC
        && dateTime.timeSpec() != Qt::UTC
        && local_time.back().toLower() != 'z'
        ) {
        local_time.append('Z');
        // qDebug() << QDateTime::fromString(local_time, Qt::ISODateWithMs);
        return QDateTime::fromString(local_time, Qt::ISODateWithMs);
    }

    return dateTime;
}
