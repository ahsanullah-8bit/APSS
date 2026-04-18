#pragma once

#include "config/licenseplateconfig.h"
#include <memory>
#include <cstddef>
#include <string>

#include <QObject>
#include <odb/database.hxx>
#include <onnxruntime_cxx_api.h>
#include <fmr/accelerators/accelerator.hpp>
#include <fmr/config/paddleocrconfig.hpp>
#include <fmr/paddle/ocr/detector.hpp>
#include <fmr/paddle/ocr/pipeline.hpp>

class LPRSessionWorker : public QObject 
{
    Q_OBJECT
public:
    explicit LPRSessionWorker(std::shared_ptr<Ort::Env> env, std::shared_ptr<odb::database> db, const LicensePlateConfig &config, QObject *parent = nullptr);

public slots:
    void init();
    void process(size_t eventDbId);

signals:
    void processed(size_t eventDbId);

private:
    // fmr::paddleocr_config readPaddleOCRDetYaml(const std::string &detModelPath);
    // fmr::paddleocr_config readPaddleOCRClsYaml(const std::string &clsModelPath);
    // fmr::paddleocr_config readPaddleOCRRecYaml(const std::string &recModelPath);
    std::string getModelYamlPath(const std::string &modelPath);

private:
    std::shared_ptr<Ort::Env> m_env;
    std::shared_ptr<odb::database> m_db;
    LicensePlateConfig m_lpConfig;

    template <typename T>
    struct PredictorPair {
        QSharedPointer<T> p;
        QSharedPointer<fmr::accelerator> ac;
    };

    PredictorPair<fmr::paddle::ocr::classifier> m_cls;
    PredictorPair<fmr::paddle::ocr::detector> m_det;
    PredictorPair<fmr::paddle::ocr::recognizer> m_rec;
    QSharedPointer<fmr::paddle::ocr::pipeline> m_ocrEngine;
};