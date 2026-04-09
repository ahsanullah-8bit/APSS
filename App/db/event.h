#pragma once

// Hide includes from ODB_COMPILER
#ifndef ODB_COMPILER
#include <cstddef>
#include <QtCore/QString>
#include <QtCore/QDateTime>
#endif

#include <odb/forward.hxx>

#pragma db model version(1, 1)

namespace APSS::ODB {

#pragma db object
class Event
{
public:
    #pragma db id auto
    size_t id;
    QString label;
    QString subLabel;
    QString camera;
    QDateTime startTime;
    QDateTime endTime;
    int trackerId = -1;
    float topScore = 0.0f;
    float score = 0.0f;
    QString thumbnail;
    bool hasClip = false;
};

}