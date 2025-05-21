#include "export.h"

#include "orm_precompiled.h"

#include <QxOrm_Impl.h>

QX_REGISTER_COMPLEX_CLASS_NAME_CPP_APSS_ORM(Export, Export)

namespace qx {

template <>
inline void register_class(QxClass<Export> & t)
{
    qx::IxDataMember * pData = NULL; Q_UNUSED(pData);
    qx::IxSqlRelation * pRelation = NULL; Q_UNUSED(pRelation);
    qx::IxFunction * pFct = NULL; Q_UNUSED(pFct);
    qx::IxValidator * pValidator = NULL; Q_UNUSED(pValidator);

    t.setName("Export");

    pData = t.id(& Export::m_id, "id", 0);

    pData = t.data(& Export::m_camera, "camera", 0, true, true);
    pData->setIsIndex(true);
    pData = t.data(& Export::m_name, "name", 0, true, true);
    pData->setIsIndex(true);
    pData = t.data(& Export::m_date, "date", 0, true, true);
    pData = t.data(& Export::m_videoPath, "videoPath", 0, true, true);
    pData->setIsUnique(true);
    pData = t.data(& Export::m_thumbPath, "thumbPath", 0, true, true);
    pData->setIsUnique(true);
    pData = t.data(& Export::m_inProgress, "inProgress", 0, true, true);

    qx::QxValidatorX<Export> * pAllValidator = t.getAllValidator(); Q_UNUSED(pAllValidator);
    pAllValidator->add_MaxLength("id", 30);
    pAllValidator->add_MaxLength("camera", 20);
    pAllValidator->add_NotNull("camera");
    pAllValidator->add_MaxLength("name", 100);
    pAllValidator->add_NotNull("name");
    pAllValidator->add_NotNull("date");
    pAllValidator->add_NotNull("videoPath");
    pAllValidator->add_NotNull("thumbPath");
    pAllValidator->add_NotNull("inProgress");
}

} // namespace qx

Export::Export() : m_inProgress(false) { ; }

Export::Export(const QString & id) : m_id(id), m_inProgress(false) { ; }

Export::~Export() { ; }

QString Export::id() const { return m_id; }

QString Export::camera() const { return m_camera; }

QString Export::name() const { return m_name; }

QDateTime Export::date() const { return m_date; }

QString Export::videoPath() const { return m_videoPath; }

QString Export::thumbPath() const { return m_thumbPath; }

bool Export::inProgress() const { return m_inProgress; }

void Export::setid(const QString & val) { m_id = val; }

void Export::setcamera(const QString & val) { m_camera = val; }

void Export::setname(const QString & val) { m_name = val; }

void Export::setdate(const QDateTime & val) { m_date = val; }

void Export::setvideoPath(const QString & val) { m_videoPath = val; }

void Export::setthumbPath(const QString & val) { m_thumbPath = val; }

void Export::setinProgress(const bool & val) { m_inProgress = val; }
