#include "regions.h"

#include "orm_precompiled.h"

#include <QxOrm_Impl.h>

QX_REGISTER_COMPLEX_CLASS_NAME_CPP_APSS_ORM(Regions, Regions)

namespace qx {

template <>
inline void register_class(QxClass<Regions> & t)
{
    qx::IxDataMember * pData = NULL; Q_UNUSED(pData);
    qx::IxSqlRelation * pRelation = NULL; Q_UNUSED(pRelation);
    qx::IxFunction * pFct = NULL; Q_UNUSED(pFct);
    qx::IxValidator * pValidator = NULL; Q_UNUSED(pValidator);

    t.setName("Regions");

    pData = t.id(& Regions::m_camera, "camera", 0);

    pData = t.data(& Regions::m_grid, "grid", 0, true, true);
    pData = t.data(& Regions::m_lastUpdate, "lastUpdate", 0, true, true);

    qx::QxValidatorX<Regions> * pAllValidator = t.getAllValidator(); Q_UNUSED(pAllValidator);
    pAllValidator->add_MaxLength("camera", 20);
    pAllValidator->add_NotNull("grid");
    pAllValidator->add_NotNull("lastUpdate");
}

} // namespace qx

Regions::Regions() { ; }

Regions::Regions(const QString & id) : m_camera(id) { ; }

Regions::~Regions() { ; }

QString Regions::getcamera() const { return m_camera; }

QString Regions::getgrid() const { return m_grid; }

QDateTime Regions::getlastUpdate() const { return m_lastUpdate; }

void Regions::setcamera(const QString & val) { m_camera = val; }

void Regions::setgrid(const QString & val) { m_grid = val; }

void Regions::setlastUpdate(const QDateTime & val) { m_lastUpdate = val; }
