#include "recordings.h"

#include "orm_precompiled.h"

#include <QxOrm_Impl.h>

QX_REGISTER_COMPLEX_CLASS_NAME_CPP_APSS_ORM(Recordings, Recordings)

namespace qx {

template <>
inline void register_class(QxClass<Recordings> & t)
{
    qx::IxDataMember * pData = NULL; Q_UNUSED(pData);
    qx::IxSqlRelation * pRelation = NULL; Q_UNUSED(pRelation);
    qx::IxFunction * pFct = NULL; Q_UNUSED(pFct);
    qx::IxValidator * pValidator = NULL; Q_UNUSED(pValidator);

    t.setName("t_Recordings");

    pData = t.id(& Recordings::m_id, "id", 0);

    pData = t.data(& Recordings::m_camera, "camera", 0, true, true);
    pData->setIsIndex(true);
    pData = t.data(& Recordings::m_path, "path", 0, true, true);
    pData->setIsUnique(true);
    pData = t.data(& Recordings::m_startTime, "startTime", 0, true, true);
    pData = t.data(& Recordings::m_endTime, "endTime", 0, true, true);
    pData = t.data(& Recordings::m_duration, "duration", 0, true, true);
    pData = t.data(& Recordings::m_motion, "motion", 0, true, true);
    pData = t.data(& Recordings::m_objects, "objects", 0, true, true);
    pData = t.data(& Recordings::m_dBFS, "dBFS", 0, true, true);
    pData = t.data(& Recordings::m_segmentSize, "segmentSize", 0, true, true);
    pData = t.data(& Recordings::m_regions, "regions", 0, true, true);

    qx::QxValidatorX<Recordings> * pAllValidator = t.getAllValidator(); Q_UNUSED(pAllValidator);
    pAllValidator->add_MaxLength("id", 30);
    pAllValidator->add_MaxLength("camera", 20);
    pAllValidator->add_NotNull("camera");
    pAllValidator->add_NotNull("path");
    pAllValidator->add_NotNull("startTime");
    pAllValidator->add_NotNull("endTime");
    pAllValidator->add_NotNull("duration");
}

} // namespace qx

Recordings::Recordings() : m_duration(0.0), m_motion(0), m_objects(0), m_dBFS(0), m_segmentSize(0.0), m_regions(0) { ; }

Recordings::Recordings(const QString & id) : m_id(id), m_duration(0.0), m_motion(0), m_objects(0), m_dBFS(0), m_segmentSize(0.0), m_regions(0) { ; }

Recordings::~Recordings() { ; }

QString Recordings::getid() const { return m_id; }

QString Recordings::getcamera() const { return m_camera; }

QString Recordings::getpath() const { return m_path; }

QDateTime Recordings::getstartTime() const { return m_startTime; }

QDateTime Recordings::getendTime() const { return m_endTime; }

float Recordings::getduration() const { return m_duration; }

long Recordings::getmotion() const { return m_motion; }

long Recordings::getobjects() const { return m_objects; }

long Recordings::getdBFS() const { return m_dBFS; }

float Recordings::getsegmentSize() const { return m_segmentSize; }

long Recordings::getregions() const { return m_regions; }

void Recordings::setid(const QString & val) { m_id = val; }

void Recordings::setcamera(const QString & val) { m_camera = val; }

void Recordings::setpath(const QString & val) { m_path = val; }

void Recordings::setstartTime(const QDateTime & val) { m_startTime = val; }

void Recordings::setendTime(const QDateTime & val) { m_endTime = val; }

void Recordings::setduration(const float & val) { m_duration = val; }

void Recordings::setmotion(const long & val) { m_motion = val; }

void Recordings::setobjects(const long & val) { m_objects = val; }

void Recordings::setdBFS(const long & val) { m_dBFS = val; }

void Recordings::setsegmentSize(const float & val) { m_segmentSize = val; }

void Recordings::setregions(const long & val) { m_regions = val; }
