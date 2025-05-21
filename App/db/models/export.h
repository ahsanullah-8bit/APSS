#pragma once

class Export
{

    QX_REGISTER_FRIEND_CLASS(Export)

protected:

    QString m_id;
    QString m_camera;
    QString m_name;
    QDateTime m_date;
    QString m_videoPath;
    QString m_thumbPath;
    bool m_inProgress;

public:

    Export();
    Export(const QString & id);
    virtual ~Export();

    QString id() const;
    QString camera() const;
    QString name() const;
    QDateTime date() const;
    QString videoPath() const;
    QString thumbPath() const;
    bool inProgress() const;

    void setid(const QString & val);
    void setcamera(const QString & val);
    void setname(const QString & val);
    void setdate(const QDateTime & val);
    void setvideoPath(const QString & val);
    void setthumbPath(const QString & val);
    void setinProgress(const bool & val);

public:

    static QString column_id(bool key = false) { Q_UNUSED(key); return "id"; }
    static QString column_camera(bool key = false) { Q_UNUSED(key); return "camera"; }
    static QString column_name(bool key = false) { Q_UNUSED(key); return "name"; }
    static QString column_date(bool key = false) { Q_UNUSED(key); return "date"; }
    static QString column_videoPath(bool key = false) { Q_UNUSED(key); return "videoPath"; }
    static QString column_thumbPath(bool key = false) { Q_UNUSED(key); return "thumbPath"; }
    static QString column_inProgress(bool key = false) { Q_UNUSED(key); return "inProgress"; }

public:

    static QString table_name(bool key = false) { Q_UNUSED(key); return "Export"; }

};

typedef std::shared_ptr<Export> Export_ptr;
typedef qx::QxCollection<QString, Export_ptr> list_of_Export;
typedef std::shared_ptr<list_of_Export> list_of_Export_ptr;

QX_REGISTER_PRIMARY_KEY(Export, QString)
QX_REGISTER_COMPLEX_CLASS_NAME_HPP_APSS_ORM(Export, qx::trait::no_base_class_defined, 0, Export)
