
#include <array>
#include <cstdint>
#include <qobject.h>
#include <string>
#include <memory>
#include <cstddef>
#include <filesystem>

#include <QUrl>
#include <QLoggingCategory>

#include <odb/sqlite/database.hxx>
#include <odb/transaction.hxx>
#include <onnxruntime_cxx_api.h>
#include <opencv2/core/mat.hpp>
#include <opencv2/imgcodecs.hpp>
#include <rfl/json/write.hpp>
#include <yaml-cpp/yaml.h>
#include <fmr/accelerators/onnxruntime.hpp>
#include <fmr/paddle/ocr/pipeline.hpp>
#include <fmr/config/paddleocrconfig.hpp>
#include <fmr/config/predictorconfig.hpp>
#include <fmr/paddle/ocr/detector.hpp>

#include <apss.h>
#include <db/event-odb.hxx>
#include <utils/rfl_opencv.hpp>
#include "lprsession.h"

Q_STATIC_LOGGING_CATEGORY(logger, "apss.sessions.lpr")

namespace pocr = fmr::paddle::ocr;

LPRSessionWorker::LPRSessionWorker(std::shared_ptr<Ort::Env> env, std::shared_ptr<odb::database> db, const LicensePlateConfig &config, QObject *parent)
    : QObject(parent)
    , m_env(env)
    , m_db(db)
    , m_lpConfig(config)
{
    setObjectName("apss.sessions.lpr");
}

void LPRSessionWorker::init()
{
    // In a new thread, probably.
    // Detector session
    fmr::predictor_config det_pconfig;
    det_pconfig.model_path = "models/PP-OCRv5_mobile_det_infer_slim_onnx/inference.onnx";

    std::unordered_map<std::string, std::string> det_ov_options;
    det_ov_options["device_type"] = "CPU";
    det_ov_options["precision"] = "ACCURACY";
    det_ov_options["num_of_threads"] = "1";
    det_ov_options["disable_dynamic_shapes"] = "false";

    std::shared_ptr<Ort::SessionOptions> det_options = std::make_shared<Ort::SessionOptions>();
    det_options->DisablePerSessionThreads();
    det_options->AppendExecutionProvider_OpenVINO_V2(det_ov_options);

// #ifdef APSS_USE_PADDLEOCR_YML
//     fmr::paddleocr_config det_config = readPaddleOCRDetYaml(det_pconfig.model_path.value());
// #else
    fmr::paddleocr_config det_config;
// #endif
    m_det.ac = QSharedPointer<fmr::onnxruntime>::create(det_pconfig, m_env, det_options);
    m_det.p = QSharedPointer<pocr::detector>::create(m_det.ac.get(), det_config);

    // Classifier session
    fmr::predictor_config cls_pconfig;
    cls_pconfig.model_path = "models/PP-LCNet_x1_0_textline_ori_infer_slim_onnx/inference.onnx";

    std::unordered_map<std::string, std::string> cls_ov_options;
    cls_ov_options["device_type"] = "CPU";
    cls_ov_options["precision"] = "ACCURACY";
    cls_ov_options["num_of_threads"] = "1";
    cls_ov_options["disable_dynamic_shapes"] = "false";

    std::shared_ptr<Ort::SessionOptions> cls_options = std::make_shared<Ort::SessionOptions>();
    cls_options->DisablePerSessionThreads();
    cls_options->AppendExecutionProvider_OpenVINO_V2(cls_ov_options);

// #ifdef APSS_USE_PADDLEOCR_YML
//     fmr::paddleocr_config cls_config = readPaddleOCRClsYaml(cls_pconfig.model_path.value());
// #else
    fmr::paddleocr_config cls_config;
// #endif
    m_cls.ac = QSharedPointer<fmr::onnxruntime>::create(cls_pconfig, m_env, cls_options);
    m_cls.p = QSharedPointer<pocr::classifier>::create(m_cls.ac.get(), cls_config);

    // Recognizer session
    fmr::predictor_config rec_pconfig;
    rec_pconfig.model_path = "models/en_PP-OCRv4_mobile_rec_infer_slim_onnx/inference.onnx";

    std::unordered_map<std::string, std::string> rec_ov_options;
    rec_ov_options["device_type"] = "CPU";
    rec_ov_options["precision"] = "ACCURACY";
    rec_ov_options["num_of_threads"] = "2";
    rec_ov_options["disable_dynamic_shapes"] = "false";

    std::shared_ptr<Ort::SessionOptions> rec_options = std::make_shared<Ort::SessionOptions>();
    rec_options->DisablePerSessionThreads();
    rec_options->AppendExecutionProvider_OpenVINO_V2(rec_ov_options);
    
// #ifdef APSS_USE_PADDLEOCR_YML 
//     fmr::paddleocr_config rec_config = readPaddleOCRRecYaml(rec_pconfig.model_path.value());
// #else
    fmr::paddleocr_config rec_config;
    // #endif
    // Some other options
    rec_config.thresh = m_lpConfig.recognition_threshold;
    m_rec.ac = QSharedPointer<fmr::onnxruntime>::create(rec_pconfig, m_env, rec_options);
    m_rec.p = QSharedPointer<pocr::recognizer>::create(m_rec.ac.get(), rec_config);

    m_ocrEngine = QSharedPointer<pocr::pipeline>::create(*m_det.p, *m_cls.p, *m_rec.p);
}

void LPRSessionWorker::process(size_t eventDbId)
{
    try {
        APSS::ODB::Event event;

        {
            odb::transaction t(m_db->begin());
            m_db->load(eventDbId, event);
            t.commit();
        }

        QString plate_path = QString("%1_%2_lp.jpg").arg(event.camera).arg(event.trackerId);
        plate_path = THUMB_DIR.filePath(plate_path);
        if (!QFile::exists(plate_path))
            return;

        cv::Mat plate = cv::imread(plate_path.toStdString());
        
        auto results_list = m_ocrEngine->predict({plate});
        const auto &results = results_list.at(0);
        event.licensePlateResults = QString::fromStdString(rfl::json::write(results));

        {
            odb::transaction t(m_db->begin());
            m_db->update(event);
            t.commit();
        }

        emit processed(eventDbId);
    } catch (const std::exception &e) {
        qCCritical(logger) << e.what();
    }
}

// namespace YAML {
//     template<>
//     struct convert<fmr::paddleocr_config::normalization_order> {
//         static bool decode(const Node& node, fmr::paddleocr_config::normalization_order& rhs) {
//             if (!node.IsScalar()) return false;
            
//             std::string val = node.as<std::string>();
//             if (val == "hwc") {
//                 rhs = fmr::paddleocr_config::normalization_order::hwc;
//             } else {
//                 return false; // Unknown value
//             }
//             return true;
//         }
//     };
//     template<>
//     struct convert<fmr::paddleocr_config::image_mode> {
//         static bool decode(const Node& node, fmr::paddleocr_config::image_mode& rhs) {
//             if (!node.IsScalar()) return false;
            
//             std::string val = node.as<std::string>();
//             if (val == "RGB") {
//                 rhs = fmr::paddleocr_config::image_mode::RGB;
//             } else if (val == "BGR") {
//                 rhs = fmr::paddleocr_config::image_mode::BGR;
//             } else {
//                 return false; // Unknown value
//             }
//             return true;
//         }
//     };
// }

// fmr::paddleocr_config LPRSessionWorker::readPaddleOCRDetYaml(const std::string &detModelPath)
// {
//     std::string file_path = getModelYamlPath(detModelPath);
//     if (!std::filesystem::exists(file_path)) {
//         qCCritical(logger) << "Model associated" << file_path << "file doesn't exist. This may cause problems as default values associated with PaddleOCR v4 and v5 will be used";
//         return {};
//     }

//     fmr::paddleocr_config config;
    
//     try {
//         YAML::Node yaml = YAML::LoadFile(file_path);

//         bool is_new_model = true;
//         const auto &global_node = yaml["Global"];
//         const auto &model_node = global_node["model_name"];
//         if (model_node) {
//             QString model_name = QString::fromStdString(model_node.as<std::string>());

//             if (!model_name.contains("OCRv4") && !model_name.contains("OCRv5"))
//                 is_new_model = false;
//         }

//         if (is_new_model) {
//             // preprocess
//             const auto &transform_ops = yaml["PreProcess"]["transform_ops"];

//             for (const auto &op : transform_ops) {
//                 if (op["DecodeImage"]) {
//                     const auto &img_mode = op["DecodeImage"]["img_mode"];
//                     if (img_mode)
//                         config.img_mode = img_mode.as<fmr::paddleocr_config::image_mode>();
//                 } else if (op["DetResizeForTest"]) {
//                     const auto &resize_long = op["DetResizeForTest"]["resize_long"];
//                     if (resize_long)
//                         config.limit_side_len = resize_long.as<int>();
//                 } else if (op["NormalizeImage"]) {
//                     YAML::Node sub_op = op["NormalizeImage"];
//                     const auto &mean = sub_op["mean"];
//                     if (mean)
//                         config.mean = mean.as<std::array<float, 3>>();

//                     const auto &order = sub_op["order"];
//                     if (order)
//                         config.norm_order = order.as<fmr::paddleocr_config::normalization_order>();

//                     const auto &scale = sub_op["scale"];
//                     if (scale)
//                         config.scale = scale.as<double>();

//                     const auto &std = sub_op["std"];
//                     if (std)
//                         config.std = std.as<std::array<float, 3>>();
//                 }
//             }

//             // postprocess
//             const auto &threshold = yaml["PostProcess"]["thresh"];
//             if (threshold)
//                 config.thresh = threshold.as<double>();

//             const auto &box_threshold = yaml["PostProcess"]["box_thresh"];
//             if (box_threshold)
//                 config.box_thresh = box_threshold.as<double>();

//             const auto &unclip_ratio = yaml["PostProcess"]["unclip_ratio"];
//             if (unclip_ratio)
//                 config.unclip_ratio = unclip_ratio.as<double>();

//         } else {
//             // TODO: Read earlier version
//         }
//     } catch(const YAML::Exception &e) {
//         qFatal() << "Failed to parse" << file_path << "file:" << e.what();
//     } catch(const std::exception &e) {
//         qFatal() << "Failed to parse" << file_path << "file:" << e.what();
//     }

//     return config;
// }

// fmr::paddleocr_config LPRSessionWorker::readPaddleOCRClsYaml(const std::string &clsModelPath)
// {
//     std::string file_path = getModelYamlPath(clsModelPath);
//     if (!std::filesystem::exists(file_path)) {
//         qCCritical(logger) << "Model associated" << file_path << "file doesn't exist. This may cause problems as default values associated with PaddleOCR v4 and v5 will be used";
//         return {};
//     }

//     fmr::paddleocr_config config;

//     try {
//         YAML::Node yaml = YAML::LoadFile(getModelYamlPath(clsModelPath));

//         bool is_new_model = true;
//         const auto &global_node = yaml["Global"];
//         const auto &model_node = global_node["model_name"];
//         if (model_node) {
//             QString model_name = QString::fromStdString(model_node.as<std::string>());

//             // TODO: Do a proper check for cls model
//             // if (!model_name.contains("OCRv4") && !model_name.contains("OCRv5"))
//             //     is_new_model = false;
//         }

//         if (is_new_model) {
//             // preprocess
//             const auto &transform_ops = yaml["PreProcess"]["transform_ops"];

//             for(const auto &op : transform_ops) {
//                 if (op["ResizeImage"]) {
//                     const auto &resize_img_size = op["ResizeImage"]["size"];
//                     if (resize_img_size)
//                         config.imgsz = resize_img_size.as<std::array<int, 2>>();
//                 } else if (op["NormalizeImage"]) {
//                     YAML::Node sub_op = op["NormalizeImage"];
//                     // const auto &channel_num = sub_op["channel_num"];
//                     // if (channel_num)
//                     //     config.channels = channel_num.as<int>();

//                     const auto &mean = sub_op["mean"];
//                     if (mean)
//                         config.mean = mean.as<std::array<float, 3>>();

//                     const auto &order = sub_op["order"];
//                     if (order)
//                         config.norm_order = order.as<fmr::paddleocr_config::normalization_order>();

//                     const auto &scale = sub_op["scale"];
//                     if (scale)
//                         config.scale = scale.as<double>();

//                     const auto &std = sub_op["std"];
//                     if (std)
//                         config.std = std.as<std::array<float, 3>>();
//                 }
//             }

//             // Postprocess
//             // We know these are two in our case
//             // const auto &label_list = yaml["PostProcess"]["Topk"]["label_list"];
//             // if (label_list) {
//             //     m_labels = label_list.as<std::vector<std::string>>();

//             //     // read from the .txt file, if empty
//             //     if (!m_labels.empty())
//             //         read_labels = false;
//             // }

//         } else {
//             // Read earlier version
//         }
//     } catch(const YAML::Exception &e) {
//         qFatal() << "Failed to parse" << file_path << "file:" << e.what();
//     } catch(const std::exception &e) {
//         qFatal() << "Failed to parse" << file_path << "file:" << e.what();
//     }

//     return config;
// }

// fmr::paddleocr_config LPRSessionWorker::readPaddleOCRRecYaml(const std::string &recModelPath)
// {
//     std::string file_path = getModelYamlPath(recModelPath);
//     if (!std::filesystem::exists(file_path)) {
//         qCCritical(logger) << "Model associated" << file_path << "file doesn't exist. This may cause problems as default values associated with PaddleOCR v4 and v5 will be used";
//         return {};
//     }

//     fmr::paddleocr_config config;

//     try {
//         YAML::Node yaml = YAML::LoadFile(getModelYamlPath(recModelPath));

//         bool is_new_model = true;
//         const auto &global_node = yaml["Global"];
//         const auto &model_node = global_node["model_name"];
//         if (model_node) {
//             QString model_name;
//             model_name = QString::fromStdString(model_node.as<std::string>());

//             if (!model_name.contains("OCRv4") && !model_name.contains("OCRv5"))
//                 is_new_model = false;
//         }

//         if (is_new_model) {
//             // preprocess
//             const auto &transform_ops = yaml["PreProcess"]["transform_ops"];
//             for (const auto &op : transform_ops) {
//                 YAML::Node sub_op;
//                 if (op["DecodeImage"]) {
//                     const auto &img_mode = op["DecodeImage"]["img_mode"];
//                     if (img_mode)
//                         config.img_mode = img_mode.as<fmr::paddleocr_config::image_mode>();
//                 } else if (op["RecResizeImg"]) {
//                     const auto &img_shape = op["RecResizeImg"]["image_shape"];
//                     if (img_shape) {
//                         const auto shape = img_shape.as<std::array<int, 3>>();
//                         // config.channels = shape.at(0);
//                         config.imgsz = {shape.at(1), shape.at(2)};
//                     }
//                 }
//             }

//             // postprocess
//             const auto &character_dict = yaml["PostProcess"]["character_dict"];
//             if (character_dict) {
//                 config.character_dict = character_dict.as<std::vector<char>>();

//                 // For some reason the character dictionary in the file has 95,
//                 // while model reports 97. So, we append 2 spaces just for safety.
//                 // Similar to how they do it at https://github.com/PaddlePaddle/PaddleOCR/blob/740a04dc4b9bb08108dca742c534e478a9a8904d/deploy/cpp_infer/src/modules/text_recognition/processors.cc#L140 
//                 config.character_dict->push_back(' ');
//                 config.character_dict->push_back(' ');
//             }

//         } else {
//             // Read earlier version
//         }        
//     } catch(const YAML::Exception &e) {
//         qFatal() << "Failed to parse" << file_path << "file:" << e.what();
//     } catch(const std::exception &e) {
//         qFatal() << "Failed to parse" << file_path << "file:" << e.what();
//     }

//     return config;
// }

std::string LPRSessionWorker::getModelYamlPath(const std::string &modelPath)
{
    const QFileInfo info = QFileInfo(QString::fromStdString(modelPath));
    return QString("%1/%2.yml").arg(info.path(), info.baseName()).toStdString();
}