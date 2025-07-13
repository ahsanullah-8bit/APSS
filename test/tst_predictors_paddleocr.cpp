#include <memory.h>

#include <gtest/gtest.h>
#include <onnxruntime_cxx_api.h>
#include <opencv2/core/mat.hpp>
#include <opencv2/imgcodecs.hpp>

#include "config/predictorconfig.h"
#include "detectors/wrappers/customallocator.h"
#include "detectors/paddleocr.h"
#include "detectors/paddledet.h"
#include "detectors/paddlecls.h"
#include "detectors/paddlerec.h"
#include "detectors/image.h"

class TestPaddleOCR : public ::testing::Test {
protected:

    static void SetUpTestSuite() {}

    static void TearDownTestSuite() {
        // optional cleanup
    }

    void SetUp() override {}

    void TearDown() override {
        // optional
    }
};

TEST_F(TestPaddleOCR, RawInferenceBenchmark) {
    std::shared_ptr<Ort::Env> env = std::make_shared<Ort::Env>(ORT_LOGGING_LEVEL_WARNING, "ONNX_Inference_Test");
    std::shared_ptr<CustomAllocator> allocator = std::make_shared<CustomAllocator>();


    cv::Mat img = cv::imread("test/assets/kkkuk.jpg");

    // det
    auto start_time = std::chrono::high_resolution_clock::now();
    PredictorConfig det_config;
    det_config.model = ModelConfig();
    det_config.model->path = "models/PP-OCRv5_mobile_det_infer_slim_onnx/inference.onnx";
    PaddleDet det(det_config, env, allocator, nullptr);
    auto det_loading_time = std::chrono::high_resolution_clock::now() - start_time;

    // cls
    start_time = std::chrono::high_resolution_clock::now();
    PredictorConfig cls_config;
    cls_config.model = ModelConfig();
    cls_config.model->path = "models/PP-LCNet_x1_0_textline_ori_infer_slim_onnx/inference.onnx";
    PaddleCls cls(cls_config, env, allocator, nullptr);
    auto cls_loading_time = std::chrono::high_resolution_clock::now() - start_time;

    // rec
    start_time = std::chrono::high_resolution_clock::now();
    PredictorConfig rec_config;
    rec_config.model = ModelConfig();
    rec_config.model->path = "models/en_PP-OCRv4_mobile_rec_infer_slim_onnx/inference.onnx";
    PaddleRec rec(rec_config, env, allocator, nullptr);
    auto rec_loading_time = std::chrono::high_resolution_clock::now() - start_time;

    // Inference
    auto total_infer_start_time = std::chrono::high_resolution_clock::now();

    std::vector<PaddleOCR::OCRPredictResult> ocr_result;

    // 1. det
    start_time = std::chrono::high_resolution_clock::now();
    std::vector<Vector3d<int>> boxes_list = det.predict( { img } );
    auto det_infer_time = std::chrono::high_resolution_clock::now() - start_time;

    Vector3d<int> boxes = boxes_list.at(0);
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
    start_time = std::chrono::high_resolution_clock::now();
    std::vector<std::pair<int, float>> cls_results = cls.predict(crop_batch);
    auto cls_infer_time = std::chrono::high_resolution_clock::now() - start_time;
    // output cls results
    for (size_t i = 0; i < cls_results.size(); ++i) {
        ocr_result[i].cls_label = cls_results[i].first;
        ocr_result[i].cls_score = cls_results[i].second;
    }

    for (size_t i = 0; i < crop_batch.size(); ++i) {
        if (ocr_result[i].cls_label % 2 == 1 &&
            ocr_result[i].cls_score > cls.clsThreshold()) {
            cv::rotate(crop_batch[i], crop_batch[i], cv::ROTATE_180);
        }
    }

    // 3. rec
    start_time = std::chrono::high_resolution_clock::now();
    const std::vector<std::pair<std::string, float>> rec_results = rec.predict(crop_batch);
    auto rec_infer_time = std::chrono::high_resolution_clock::now() - start_time;

    // End timing for the inference pipeline
    auto total_infer_time = std::chrono::high_resolution_clock::now() - total_infer_start_time;

    // output rec results
    for (size_t i = 0; i < rec_results.size(); ++i) {
        ocr_result[i].text = std::move(rec_results[i].first);
        ocr_result[i].score = rec_results[i].second;
    }

    // Parse ocr_result
    for (const auto &ocr : ocr_result) {
        qDebug() << ocr.score << ocr.text;
    }
    Utils::drawOCR(img, ocr_result);

    cv::imwrite("ocr.jpg", img);

    // Print the benchmark result using qDebug
    qDebug() << "Det Loading:" << std::chrono::duration<double, std::milli>(det_loading_time).count();
    qDebug() << "Cls Loading:" << std::chrono::duration<double, std::milli>(cls_loading_time).count();
    qDebug() << "Rec Loading:" << std::chrono::duration<double, std::milli>(rec_loading_time).count();
    qDebug();

    qDebug() << "Det Inference:" << std::chrono::duration<double, std::milli>(det_infer_time).count();
    qDebug() << "Cls Inference:" << std::chrono::duration<double, std::milli>(cls_infer_time).count();
    qDebug() << "Rec Inference:" << std::chrono::duration<double, std::milli>(rec_infer_time).count();
    qDebug() << "Total Inference:" << std::chrono::duration<double, std::milli>(total_infer_time).count();
}
