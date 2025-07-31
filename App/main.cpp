// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QtSql/QSqlError>

#include "autogen/environment.h"

#include <rfl/yaml.hpp>

#include "config/apssconfig.h"
#include "engine/apssengine.h"
#include "models/eventsmodel.h"

APSSConfig loadConfig(const QString &filepath);

int main(int argc, char *argv[])
{
    qSetMessagePattern("%{time HH:mm:ss.zzz} [%{category}] %{message}");
    set_qt_environment();
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;

    // APSSEngine Setup -------------

    APSSConfig config = loadConfig("config.yml");
    APSSEngine *apssEngine = new APSSEngine(&config, &engine);
    apssEngine->start();
    engine.rootContext()->setContextProperty("apssEngine", apssEngine);
    QObject::connect(&app, &QGuiApplication::aboutToQuit, apssEngine, &APSSEngine::stop);

    // --------------------------

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "event_reader_conn");
    db.setDatabaseName("apss.db");

    if (!db.open() || !db.isValid()) {
        qCritical() << "Failed to open a database connection:" << db.lastError().text();
    }
    EventsModel events_model(db);
    engine.rootContext()->setContextProperty("eventsModel", &events_model);

    // --------------------------

    const QUrl url(mainQmlFile);
    QObject::connect(
                &engine, &QQmlApplicationEngine::objectCreated, &app,
                [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);

    engine.addImportPath(QCoreApplication::applicationDirPath() + "/qml");
    engine.addImportPath(":/");
    engine.load(url);

    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}


APSSConfig loadConfig(const QString &filepath)
{
    if (filepath.isEmpty() || !QFile::exists(filepath))
        qFatal() << "No config.yml found, please create one!";

    try {
        // CameraConfig cam_config;
        // cam_config.enabled = true;
        // cam_config.ffmpeg.inputs.emplace_back(CameraInput( "C:/Users/MadGuy/Videos/ny_street2.mp4", {CameraRoleEnum::Detect}));
        // cam_config.objects = ObjectConfig();
        // cam_config.objects->track = DEFAULT_TRACKED_OBJECTS;
        // cam_config.pull_based_order = true;

        // PredictorConfig pred_config;
        // pred_config.model = ModelConfig();
        // pred_config.model->path = "models/yolo11n.onnx";

        QFile file(filepath);
        if (file.open(QIODeviceBase::ReadOnly | QIODeviceBase::Text)) {
            std::string conf_str = file.readAll().toStdString();

            const auto yaml_config = rfl::yaml::read<APSSConfig>(conf_str);
            if (yaml_config)
                return APSSConfig(yaml_config.value());
            else if (yaml_config.error())
                qWarning() << yaml_config.error()->what();
        }
    } catch (const std::exception &e) {
        qFatal() << e.what();
    }

    return APSSConfig();
}
