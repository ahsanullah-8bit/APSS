#pragma once

// Hide includes from ODB_COMPILER
#include <cstddef>
#ifndef ODB_COMPILER
#include <QtCore/QString>
#include <QtCore/QDateTime>
#endif

#pragma db model version(1, 2)

namespace APSS::ODB {
    
#pragma db object
class Prediction {
public:
    #pragma db id auto
    size_t id;
    size_t eventId;
    QString frameId;
    QDateTime videoTimestamp;
    QDateTime streamTimestamp;
    QString data;

    #pragma db added(2)
    bool hasSubPredictions;
};

}