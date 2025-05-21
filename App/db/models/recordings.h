#pragma once

class Recordings
{

    QX_REGISTER_FRIEND_CLASS(Recordings)

protected:

    QString m_id;
    QString m_camera;
    QString m_path;
    QDateTime m_startTime;
    QDateTime m_endTime;
    float m_duration;
    long m_motion;
    long m_objects;
    long m_dBFS;
    float m_segmentSize;
    long m_regions;

public:

    Recordings();
    Recordings(const QString & id);
    virtual ~Recordings();

    QString getid() const;
    QString getcamera() const;
    QString getpath() const;
    QDateTime getstartTime() const;
    QDateTime getendTime() const;
    float getduration() const;
    long getmotion() const;
    long getobjects() const;
    long getdBFS() const;
    float getsegmentSize() const;
    long getregions() const;

    void setid(const QString & val);
    void setcamera(const QString & val);
    void setpath(const QString & val);
    void setstartTime(const QDateTime & val);
    void setendTime(const QDateTime & val);
    void setduration(const float & val);
    void setmotion(const long & val);
    void setobjects(const long & val);
    void setdBFS(const long & val);
    void setsegmentSize(const float & val);
    void setregions(const long & val);

public:

    static QString column_id(bool key = false) { Q_UNUSED(key); return "id"; }
    static QString column_camera(bool key = false) { Q_UNUSED(key); return "camera"; }
    static QString column_path(bool key = false) { Q_UNUSED(key); return "path"; }
    static QString column_startTime(bool key = false) { Q_UNUSED(key); return "startTime"; }
    static QString column_endTime(bool key = false) { Q_UNUSED(key); return "endTime"; }
    static QString column_duration(bool key = false) { Q_UNUSED(key); return "duration"; }
    static QString column_motion(bool key = false) { Q_UNUSED(key); return "motion"; }
    static QString column_objects(bool key = false) { Q_UNUSED(key); return "objects"; }
    static QString column_dBFS(bool key = false) { Q_UNUSED(key); return "dBFS"; }
    static QString column_segmentSize(bool key = false) { Q_UNUSED(key); return "segmentSize"; }
    static QString column_regions(bool key = false) { Q_UNUSED(key); return "regions"; }

public:

    static QString table_name(bool key = false) { return (key ? QString("Recordings") : QString("t_Recordings")); }

};

typedef std::shared_ptr<Recordings> Recordings_ptr;
typedef qx::QxCollection<QString, Recordings_ptr> list_of_Recordings;
typedef std::shared_ptr<list_of_Recordings> list_of_Recordings_ptr;

QX_REGISTER_PRIMARY_KEY(Recordings, QString)
QX_REGISTER_COMPLEX_CLASS_NAME_HPP_APSS_ORM(Recordings, qx::trait::no_base_class_defined, 0, Recordings)
