#include "timeline.h"

#include "orm_precompiled.h"

#include <QxOrm_Impl.h>

QX_REGISTER_COMPLEX_CLASS_NAME_CPP_APSS_ORM(Timeline, Timeline)

namespace qx {

template <>
inline void register_class(QxClass<Timeline> & t)
{
    qx::IxDataMember * pData = NULL; Q_UNUSED(pData);
    qx::IxSqlRelation * pRelation = NULL; Q_UNUSED(pRelation);
    qx::IxFunction * pFct = NULL; Q_UNUSED(pFct);
    qx::IxValidator * pValidator = NULL; Q_UNUSED(pValidator);

    t.setName("Timeline");

    pData = t.id(& Timeline::m_id, "id", 0);

    pData = t.data(& Timeline::m_timestamp, "timestamp", 0, true, true);
    pData = t.data(& Timeline::m_camera, "camera", 0, true, true);
    pData->setIsIndex(true);
    pData = t.data(& Timeline::m_source, "source", 0, true, true);
    pData->setIsIndex(true);
    pData = t.data(& Timeline::m_sourceId, "sourceId", 0, true, true);
    pData->setIsIndex(true);
    pData = t.data(& Timeline::m_classType, "classType", 0, true, true);
    pData = t.data(& Timeline::m_data, "data", 0, true, true);

    qx::QxValidatorX<Timeline> * pAllValidator = t.getAllValidator(); Q_UNUSED(pAllValidator);
    pAllValidator->add_NotNull("timestamp");
    pAllValidator->add_MaxLength("camera", 20);
    pAllValidator->add_NotNull("camera");
    pAllValidator->add_MaxLength("source", 20);
    pAllValidator->add_NotNull("source");
    pAllValidator->add_MaxLength("sourceId", 30);
    pAllValidator->add_NotNull("sourceId");
    pAllValidator->add_MaxLength("classType", 50);
    pAllValidator->add_NotNull("classType");
    pAllValidator->add_NotNull("data");
}

} // namespace qx

Timeline::Timeline() : m_id(0) { ; }

Timeline::Timeline(const long & id) : m_id(id) { ; }

Timeline::~Timeline() { ; }

long Timeline::getid() const { return m_id; }

QDateTime Timeline::gettimestamp() const { return m_timestamp; }

QString Timeline::getcamera() const { return m_camera; }

QString Timeline::getsource() const { return m_source; }

QString Timeline::getsourceId() const { return m_sourceId; }

QString Timeline::getclassType() const { return m_classType; }

QString Timeline::getdata() const { return m_data; }

void Timeline::setid(const long & val) { m_id = val; }

void Timeline::settimestamp(const QDateTime & val) { m_timestamp = val; }

void Timeline::setcamera(const QString & val) { m_camera = val; }

void Timeline::setsource(const QString & val) { m_source = val; }

void Timeline::setsourceId(const QString & val) { m_sourceId = val; }

void Timeline::setclassType(const QString & val) { m_classType = val; }

void Timeline::setdata(const QString & val) { m_data = val; }
