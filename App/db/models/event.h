#pragma once

class Event
{
    QX_REGISTER_FRIEND_CLASS(Event)

protected:

    QString m_id;
    QString m_label;
    QString m_subLabel;
    QString m_camera;
    QDateTime m_startTime;
    QDateTime m_endTime;
    float m_topScore;
    float m_score;
    bool m_falsePositive;
    QString m_zones;
    QString m_thumbnail;
    bool m_hasClip;
    bool m_hasSnapshot;
    QString m_region;
    QString m_box;
    long m_area;
    bool m_retainIndefinitely;
    float m_ratio;
    QString m_plusId;
    QString m_modelHash;
    QString m_detectorType;
    QString m_modelType;
    QString m_data;

public:

    Event();
    Event(const QString & id);
    virtual ~Event();

    QString getid() const;
    QString getlabel() const;
    QString getsubLabel() const;
    QString getcamera() const;
    QDateTime getstartTime() const;
    QDateTime getendTime() const;
    float gettopScore() const;
    float getscore() const;
    bool getfalsePositive() const;
    QString getzones() const;
    QString getthumbnail() const;
    bool gethasClip() const;
    bool gethasSnapshot() const;
    QString getregion() const;
    QString getbox() const;
    long getarea() const;
    bool getretainIndefinitely() const;
    float getratio() const;
    QString getplusId() const;
    QString getmodelHash() const;
    QString getdetectorType() const;
    QString getmodelType() const;
    QString getdata() const;

    void setid(const QString & val);
    void setlabel(const QString & val);
    void setsubLabel(const QString & val);
    void setcamera(const QString & val);
    void setstartTime(const QDateTime & val);
    void setendTime(const QDateTime & val);
    void settopScore(const float & val);
    void setscore(const float & val);
    void setfalsePositive(const bool & val);
    void setzones(const QString & val);
    void setthumbnail(const QString & val);
    void sethasClip(const bool & val);
    void sethasSnapshot(const bool & val);
    void setregion(const QString & val);
    void setbox(const QString & val);
    void setarea(const long & val);
    void setretainIndefinitely(const bool & val);
    void setratio(const float & val);
    void setplusId(const QString & val);
    void setmodelHash(const QString & val);
    void setdetectorType(const QString & val);
    void setmodelType(const QString & val);
    void setdata(const QString & val);

public:

    static QString column_id(bool key = false) { Q_UNUSED(key); return "id"; }
    static QString column_label(bool key = false) { Q_UNUSED(key); return "label"; }
    static QString column_subLabel(bool key = false) { return (key ? QString("subLabel") : QString("sub_label")); }
    static QString column_camera(bool key = false) { Q_UNUSED(key); return "camera"; }
    static QString column_startTime(bool key = false) { return (key ? QString("startTime") : QString("start_time")); }
    static QString column_endTime(bool key = false) { return (key ? QString("endTime") : QString("end_time")); }
    static QString column_topScore(bool key = false) { return (key ? QString("topScore") : QString("top_score")); }
    static QString column_score(bool key = false) { Q_UNUSED(key); return "score"; }
    static QString column_falsePositive(bool key = false) { return (key ? QString("falsePositive") : QString("false_positive")); }
    static QString column_zones(bool key = false) { Q_UNUSED(key); return "zones"; }
    static QString column_thumbnail(bool key = false) { Q_UNUSED(key); return "thumbnail"; }
    static QString column_hasClip(bool key = false) { return (key ? QString("hasClip") : QString("has_clip")); }
    static QString column_hasSnapshot(bool key = false) { return (key ? QString("hasSnapshot") : QString("has_snapshot")); }
    static QString column_region(bool key = false) { Q_UNUSED(key); return "region"; }
    static QString column_box(bool key = false) { Q_UNUSED(key); return "box"; }
    static QString column_area(bool key = false) { Q_UNUSED(key); return "area"; }
    static QString column_retainIndefinitely(bool key = false) { return (key ? QString("retainIndefinitely") : QString("retain_indefinitely")); }
    static QString column_ratio(bool key = false) { Q_UNUSED(key); return "ratio"; }
    static QString column_plusId(bool key = false) { return (key ? QString("plusId") : QString("plus_id")); }
    static QString column_modelHash(bool key = false) { return (key ? QString("modelHash") : QString("model_hash")); }
    static QString column_detectorType(bool key = false) { return (key ? QString("detectorType") : QString("detector_type")); }
    static QString column_modelType(bool key = false) { return (key ? QString("modelType") : QString("model_type")); }
    static QString column_data(bool key = false) { Q_UNUSED(key); return "data"; }

public:

    static QString table_name(bool key = false) { return (key ? QString("Event") : QString("Events")); }

};

typedef std::shared_ptr<Event> Event_ptr;
typedef qx::QxCollection<QString, Event_ptr> list_of_Event;
typedef std::shared_ptr<list_of_Event> list_of_Event_ptr;

QX_REGISTER_PRIMARY_KEY(Event, QString)
QX_REGISTER_COMPLEX_CLASS_NAME_HPP_APSS_ORM(Event, qx::trait::no_base_class_defined, 0, Event)
