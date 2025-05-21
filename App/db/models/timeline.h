#pragma once

class Timeline
{

    QX_REGISTER_FRIEND_CLASS(Timeline)

protected:

    long m_id;
    QDateTime m_timestamp;
    QString m_camera;
    QString m_source;
    QString m_sourceId;
    QString m_classType;
    QString m_data;

public:

    Timeline();
    Timeline(const long & id);
    virtual ~Timeline();

    long getid() const;
    QDateTime gettimestamp() const;
    QString getcamera() const;
    QString getsource() const;
    QString getsourceId() const;
    QString getclassType() const;
    QString getdata() const;

    void setid(const long & val);
    void settimestamp(const QDateTime & val);
    void setcamera(const QString & val);
    void setsource(const QString & val);
    void setsourceId(const QString & val);
    void setclassType(const QString & val);
    void setdata(const QString & val);

public:

    static QString column_id(bool key = false) { Q_UNUSED(key); return "id"; }
    static QString column_timestamp(bool key = false) { Q_UNUSED(key); return "timestamp"; }
    static QString column_camera(bool key = false) { Q_UNUSED(key); return "camera"; }
    static QString column_source(bool key = false) { Q_UNUSED(key); return "source"; }
    static QString column_sourceId(bool key = false) { Q_UNUSED(key); return "sourceId"; }
    static QString column_classType(bool key = false) { Q_UNUSED(key); return "classType"; }
    static QString column_data(bool key = false) { Q_UNUSED(key); return "data"; }

public:

    static QString table_name(bool key = false) { Q_UNUSED(key); return "Timeline"; }

};

typedef std::shared_ptr<Timeline> Timeline_ptr;
typedef qx::QxCollection<long, Timeline_ptr> list_of_Timeline;
typedef std::shared_ptr<list_of_Timeline> list_of_Timeline_ptr;

QX_REGISTER_COMPLEX_CLASS_NAME_HPP_APSS_ORM(Timeline, qx::trait::no_base_class_defined, 0, Timeline)
