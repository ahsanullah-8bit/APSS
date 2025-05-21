#include "event.h"

#include "orm_precompiled.h"

#include <QxOrm_Impl.h>

QX_REGISTER_COMPLEX_CLASS_NAME_CPP_APSS_ORM(Event, Event)

namespace qx {

template <>
inline void register_class(QxClass<Event> & t)
{
    qx::IxDataMember * pData = NULL; Q_UNUSED(pData);
    qx::IxSqlRelation * pRelation = NULL; Q_UNUSED(pRelation);
    qx::IxFunction * pFct = NULL; Q_UNUSED(pFct);
    qx::IxValidator * pValidator = NULL; Q_UNUSED(pValidator);

    t.setName("Events");

    pData = t.id(& Event::m_id, "id", 0);

    pData = t.data(& Event::m_label, "label", 0, true, true);
    pData->setIsIndex(true);
    pData = t.data(& Event::m_subLabel, "subLabel", 0, true, true);
    pData->setName("sub_label");
    pData = t.data(& Event::m_camera, "camera", 0, true, true);
    pData->setIsIndex(true);
    pData = t.data(& Event::m_startTime, "startTime", 0, true, true);
    pData->setName("start_time");
    pData = t.data(& Event::m_endTime, "endTime", 0, true, true);
    pData->setName("end_time");
    pData = t.data(& Event::m_topScore, "topScore", 0, true, true);
    pData->setName("top_score");
    pData = t.data(& Event::m_score, "score", 0, true, true);
    pData = t.data(& Event::m_falsePositive, "falsePositive", 0, true, true);
    pData->setName("false_positive");
    pData = t.data(& Event::m_zones, "zones", 0, true, true);
    pData = t.data(& Event::m_thumbnail, "thumbnail", 0, true, true);
    pData = t.data(& Event::m_hasClip, "hasClip", 0, true, true);
    pData->setName("has_clip");
    pData = t.data(& Event::m_hasSnapshot, "hasSnapshot", 0, true, true);
    pData->setName("has_snapshot");
    pData = t.data(& Event::m_region, "region", 0, true, true);
    pData = t.data(& Event::m_box, "box", 0, true, true);
    pData = t.data(& Event::m_area, "area", 0, true, true);
    pData = t.data(& Event::m_retainIndefinitely, "retainIndefinitely", 0, true, true);
    pData->setName("retain_indefinitely");
    pData = t.data(& Event::m_ratio, "ratio", 0, true, true);
    pData = t.data(& Event::m_plusId, "plusId", 0, true, true);
    pData->setName("plus_id");
    pData = t.data(& Event::m_modelHash, "modelHash", 0, true, true);
    pData->setName("model_hash");
    pData = t.data(& Event::m_detectorType, "detectorType", 0, true, true);
    pData->setName("detector_type");
    pData = t.data(& Event::m_modelType, "modelType", 0, true, true);
    pData->setName("model_type");
    pData = t.data(& Event::m_data, "data", 0, true, true);

    qx::QxValidatorX<Event> * pAllValidator = t.getAllValidator(); Q_UNUSED(pAllValidator);
    pAllValidator->add_MaxLength("id", 30);
    pAllValidator->add_MaxLength("label", 20);
    pAllValidator->add_MaxLength("subLabel", 100);
    pAllValidator->add_MaxLength("camera", 20);
    pAllValidator->add_MaxLength("plusId", 30);
    pAllValidator->add_MaxLength("modelHash", 32);
    pAllValidator->add_MaxLength("detectorType", 32);
    pAllValidator->add_MaxLength("modelType", 32);
}

} // namespace qx

Event::Event() : m_topScore(0.0), m_score(0.0), m_falsePositive(false), m_hasClip(true), m_hasSnapshot(true), m_area(0), m_retainIndefinitely(false), m_ratio(1.0) { ; }

Event::Event(const QString & id) : m_id(id), m_topScore(0.0), m_score(0.0), m_falsePositive(false), m_hasClip(true), m_hasSnapshot(true), m_area(0), m_retainIndefinitely(false), m_ratio(1.0) { ; }

Event::~Event() { ; }

QString Event::getid() const { return m_id; }

QString Event::getlabel() const { return m_label; }

QString Event::getsubLabel() const { return m_subLabel; }

QString Event::getcamera() const { return m_camera; }

QDateTime Event::getstartTime() const { return m_startTime; }

QDateTime Event::getendTime() const { return m_endTime; }

float Event::gettopScore() const { return m_topScore; }

float Event::getscore() const { return m_score; }

bool Event::getfalsePositive() const { return m_falsePositive; }

QString Event::getzones() const { return m_zones; }

QString Event::getthumbnail() const { return m_thumbnail; }

bool Event::gethasClip() const { return m_hasClip; }

bool Event::gethasSnapshot() const { return m_hasSnapshot; }

QString Event::getregion() const { return m_region; }

QString Event::getbox() const { return m_box; }

long Event::getarea() const { return m_area; }

bool Event::getretainIndefinitely() const { return m_retainIndefinitely; }

float Event::getratio() const { return m_ratio; }

QString Event::getplusId() const { return m_plusId; }

QString Event::getmodelHash() const { return m_modelHash; }

QString Event::getdetectorType() const { return m_detectorType; }

QString Event::getmodelType() const { return m_modelType; }

QString Event::getdata() const { return m_data; }

void Event::setid(const QString & val) { m_id = val; }

void Event::setlabel(const QString & val) { m_label = val; }

void Event::setsubLabel(const QString & val) { m_subLabel = val; }

void Event::setcamera(const QString & val) { m_camera = val; }

void Event::setstartTime(const QDateTime & val) { m_startTime = val; }

void Event::setendTime(const QDateTime & val) { m_endTime = val; }

void Event::settopScore(const float & val) { m_topScore = val; }

void Event::setscore(const float & val) { m_score = val; }

void Event::setfalsePositive(const bool & val) { m_falsePositive = val; }

void Event::setzones(const QString & val) { m_zones = val; }

void Event::setthumbnail(const QString & val) { m_thumbnail = val; }

void Event::sethasClip(const bool & val) { m_hasClip = val; }

void Event::sethasSnapshot(const bool & val) { m_hasSnapshot = val; }

void Event::setregion(const QString & val) { m_region = val; }

void Event::setbox(const QString & val) { m_box = val; }

void Event::setarea(const long & val) { m_area = val; }

void Event::setretainIndefinitely(const bool & val) { m_retainIndefinitely = val; }

void Event::setratio(const float & val) { m_ratio = val; }

void Event::setplusId(const QString & val) { m_plusId = val; }

void Event::setmodelHash(const QString & val) { m_modelHash = val; }

void Event::setdetectorType(const QString & val) { m_detectorType = val; }

void Event::setmodelType(const QString & val) { m_modelType = val; }

void Event::setdata(const QString & val) { m_data = val; }

