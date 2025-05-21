#pragma once

class Regions
{

    QX_REGISTER_FRIEND_CLASS(Regions)

protected:

    QString m_camera;
    QString m_grid;
    QDateTime m_lastUpdate;

public:

    Regions();
    Regions(const QString & id);
    virtual ~Regions();

    QString getcamera() const;
    QString getgrid() const;
    QDateTime getlastUpdate() const;

    void setcamera(const QString & val);
    void setgrid(const QString & val);
    void setlastUpdate(const QDateTime & val);

public:

    static QString column_camera(bool key = false) { Q_UNUSED(key); return "camera"; }
    static QString column_grid(bool key = false) { Q_UNUSED(key); return "grid"; }
    static QString column_lastUpdate(bool key = false) { Q_UNUSED(key); return "lastUpdate"; }

public:

    static QString table_name(bool key = false) { Q_UNUSED(key); return "Regions"; }

};

typedef std::shared_ptr<Regions> Regions_ptr;
typedef qx::QxCollection<QString, Regions_ptr> list_of_Regions;
typedef std::shared_ptr<list_of_Regions> list_of_Regions_ptr;

QX_REGISTER_PRIMARY_KEY(Regions, QString)
QX_REGISTER_COMPLEX_CLASS_NAME_HPP_APSS_ORM(Regions, qx::trait::no_base_class_defined, 0, Regions)
