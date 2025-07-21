// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "autogen/environment.h"

#include <rfl/yaml.hpp>

#include "config/apssconfig.h"
#include "engine/apssengine.h"

int main(int argc, char *argv[])
{
    qSetMessagePattern("%{time HH:mm:ss.zzz} [%{category}] %{message}");
    set_qt_environment();
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;

    // APSSEngine Setup -------------

    CameraConfig cam_config;
    cam_config.enabled = true;
    cam_config.ffmpeg.inputs.emplace_back(CameraInput( "C:/Users/MadGuy/Videos/ny_street2.mp4", {CameraRoleEnum::Detect}));
    cam_config.objects = ObjectConfig();
    cam_config.objects->track = DEFAULT_TRACKED_OBJECTS;
    cam_config.pull_based_order = true;

    PredictorConfig pred_config;
    pred_config.model = ModelConfig();
    pred_config.model->path = "models/yolo11n.onnx";

    APSSConfig config;
    config.version = "0.1";
    config.cameras["local_file"] = cam_config;
    config.predictors["yolo11_det"] = pred_config;

    APSSEngine *apssEngine = new APSSEngine(&config, &engine);
    apssEngine->start();
    engine.rootContext()->setContextProperty("apssEngine", apssEngine);
    QObject::connect(&app, &QGuiApplication::aboutToQuit, apssEngine, &APSSEngine::stop);

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
