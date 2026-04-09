#include <memory>

#include <onnxruntime_cxx_api.h>
#include <opencv2/core.hpp>

#include <detectors/onnxinference.h>
#include "paddleocr.h"

PaddleOCREngine::PaddleOCREngine(std::shared_ptr<Ort::Env> env,
                                 std::shared_ptr<CustomAllocator> allocator,
                                 std::unique_ptr<PaddleDet> det,
                                 std::unique_ptr<PaddleCls> cls,
                                 std::unique_ptr<PaddleRec> rec)
    : m_det(std::move(det))
    , m_cls(std::move(cls))
    , m_rec(std::move(rec))
{
    if (!env)
        env = std::make_shared<Ort::Env>();

    if (!allocator)
        allocator = std::make_shared<CustomAllocator>(Ort::AllocatorWithDefaultOptions());

    if (!m_det) {
        PredictorConfig det_config;
        det_config.model = ModelConfig();
        det_config.model->path = "models/PP-OCRv5_mobile_det_infer_slim_onnx/inference.onnx";

        std::unordered_map<std::string, std::string> ov_options;
        ov_options["device_type"] = "CPU";
        ov_options["precision"] = "ACCURACY";
        ov_options["num_of_threads"] = "1";
        ov_options["disable_dynamic_shapes"] = "false";

        std::shared_ptr<Ort::SessionOptions> session_options = std::make_shared<Ort::SessionOptions>();
        session_options->DisablePerSessionThreads();
        session_options->AppendExecutionProvider_OpenVINO_V2(ov_options);

        std::unique_ptr<ONNXInference> infer = std::make_unique<ONNXInference>(det_config, env, session_options, allocator, nullptr);
        m_det = std::make_unique<PaddleDet>(det_config, std::move(infer));
    }

    if (!m_cls) {
        PredictorConfig cls_config;
        cls_config.model = ModelConfig();
        cls_config.model->path = "models/PP-LCNet_x1_0_textline_ori_infer_slim_onnx/inference.onnx";

        std::unordered_map<std::string, std::string> ov_options;
        ov_options["device_type"] = "CPU";
        ov_options["precision"] = "ACCURACY";
        ov_options["num_of_threads"] = "1";
        ov_options["disable_dynamic_shapes"] = "false";

        std::shared_ptr<Ort::SessionOptions> session_options = std::make_shared<Ort::SessionOptions>();
        session_options->DisablePerSessionThreads();
        session_options->AppendExecutionProvider_OpenVINO_V2(ov_options);

        std::unique_ptr<ONNXInference> infer = std::make_unique<ONNXInference>(cls_config, env, session_options, allocator, nullptr);
        m_cls = std::make_unique<PaddleCls>(cls_config, std::move(infer));
    }

    if (!m_rec) {
        PredictorConfig rec_config;
        rec_config.model = ModelConfig();
        rec_config.model->path = "models/en_PP-OCRv4_mobile_rec_infer_slim_onnx/inference.onnx";

        std::unordered_map<std::string, std::string> ov_options;
        ov_options["device_type"] = "CPU";
        ov_options["precision"] = "ACCURACY";
        ov_options["num_of_threads"] = "2";
        ov_options["disable_dynamic_shapes"] = "false";

        std::shared_ptr<Ort::SessionOptions> session_options = std::make_shared<Ort::SessionOptions>();
        session_options->DisablePerSessionThreads();
        session_options->AppendExecutionProvider_OpenVINO_V2(ov_options);

        std::unique_ptr<ONNXInference> infer = std::make_unique<ONNXInference>(rec_config, env, session_options, allocator, nullptr);
        m_rec = std::make_unique<PaddleRec>(rec_config, std::move(infer));
    }
}

std::vector<PaddleOCR::OCRPredictResultList> PaddleOCREngine::predict(const MatList &batch)
{
    std::vector<PaddleOCR::OCRPredictResultList> ocr_results_list(batch.size(), {});

    for (size_t b = 0; b < batch.size(); ++b) {
        cv::Mat img = batch.at(b);
        std::vector<PaddleOCR::OCRPredictResult> ocr_result;

        // 1. det
        std::vector<Vector3d<int>> boxes_list = m_det->predict( { img } );
        Vector3d<int> boxes = boxes_list.at(0);
        if (boxes.empty())
            continue;

        for (size_t i = 0; i < boxes.size(); ++i) {
            PaddleOCR::OCRPredictResult res;
            res.box = std::move(boxes[i]);
            ocr_result.emplace_back(std::move(res));
        }

        PaddleOCR::Utility::sort_boxes(ocr_result);

        // crop the image
        MatList crop_batch;
        for (size_t i = 0; i < ocr_result.size(); ++i) {
            cv::Mat crop_img = PaddleOCR::Utility::GetRotateCropImage(img, ocr_result[i].box);
            crop_batch.emplace_back(std::move(crop_img));
        }

        // 2. cls
        const std::vector<std::pair<int, float>> cls_results = m_cls->predict(crop_batch);
        // output cls results
        for (size_t i = 0; i < cls_results.size(); ++i) {
            ocr_result[i].cls_label = cls_results[i].first;
            ocr_result[i].cls_score = cls_results[i].second;
        }

        for (size_t i = 0; i < crop_batch.size(); ++i) {
            if (ocr_result[i].cls_label % 2 == 1 &&
                ocr_result[i].cls_score > m_cls->threshold()) {
                cv::rotate(crop_batch[i], crop_batch[i], cv::ROTATE_180);
            }
        }

        // 3. rec
        const std::vector<std::pair<std::string, float>> rec_results = m_rec->predict(crop_batch);
        // output rec results
        for (size_t i = 0; i < rec_results.size(); ++i) {
            ocr_result[i].text = std::move(rec_results[i].first);
            ocr_result[i].score = rec_results[i].second;
        }

        ocr_results_list[b] = std::move(ocr_result);
    }

    return ocr_results_list;
}
