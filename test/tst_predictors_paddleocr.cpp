#include <gtest/gtest.h>

#include "config/predictorconfig.h"
#include "detectors/paddleocr.h"
#include "detectors/paddledet.h"
#include "detectors/paddlecls.h"
#include "detectors/paddlerec.h"

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

// TEST_F(TestPaddleOCR, LoadModel) {
//     std::shared_ptr<Ort::Env> env = std::make_shared<Ort::Env>(ORT_LOGGING_LEVEL_WARNING, "ONNX_Inference_Test");
//     std::shared_ptr<Ort::AllocatorWithDefaultOptions> allocator = std::make_shared<Ort::AllocatorWithDefaultOptions>();
//     std::shared_ptr<Ort::MemoryInfo> memory_info = std::make_shared<Ort::MemoryInfo>(Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault));

//     // det
//     PredictorConfig det_config;
//     det_config.model = ModelConfig();
//     det_config.model->path = "models/PP-OCRv5_mobile_det_infer.onnx";

//     PaddleDet det(det_config, env, allocator, memory_info);
//     det.inferSession().printSessionMetadata();
//     det.inferSession().printModelMetadata();
//     qDebug() << "\n";

//     // cls
//     PredictorConfig cls_config;
//     cls_config.model = ModelConfig();
//     cls_config.model->path = "models/PP-LCNet_x1_0_textline_ori_infer.onnx";

//     PaddleCls cls(cls_config, env, allocator, memory_info);
//     cls.inferSession().printSessionMetadata();
//     cls.inferSession().printModelMetadata();
//     qDebug() << "\n";

//     // rec
//     PredictorConfig rec_config;
//     rec_config.model = ModelConfig();
//     rec_config.model->path = "models/PP-OCRv5_mobile_rec_infer.onnx";

//     PaddleRec rec(rec_config, env, allocator, memory_info);
//     rec.inferSession().printSessionMetadata();
//     rec.inferSession().printModelMetadata();
//     qDebug() << "\n";
// }

TEST_F(TestPaddleOCR, RawInference) {
    std::shared_ptr<Ort::Env> env = std::make_shared<Ort::Env>(ORT_LOGGING_LEVEL_WARNING, "ONNX_Inference_Test");
    std::shared_ptr<Ort::AllocatorWithDefaultOptions> allocator = std::make_shared<Ort::AllocatorWithDefaultOptions>();
    // std::shared_ptr<Ort::MemoryInfo> memory_info = std::make_shared<Ort::MemoryInfo>(Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault));

    cv::Mat img = cv::imread("test/assets/vehicles2.jpg");

    // det
    PredictorConfig det_config;
    det_config.model = ModelConfig();
    det_config.model->path = "models/PP-OCRv5_mobile_det_infer_onnx/inference.onnx";
    PaddleDet det(det_config, env, allocator, nullptr);

    // cls
    PredictorConfig cls_config;
    cls_config.model = ModelConfig();
    cls_config.model->path = "models/PP-LCNet_x1_0_textline_ori_infer_onnx/inference.onnx";
    PaddleCls cls(cls_config, env, allocator, nullptr);

    // rec
    PredictorConfig rec_config;
    rec_config.model = ModelConfig();
    rec_config.model->path = "models/en_PP-OCRv4_mobile_rec_infer_onnx/inference.onnx";
    PaddleRec rec(rec_config, env, allocator, nullptr);

    // Inference
    std::vector<PaddleOCR::OCRPredictResult> ocr_result;

    // 1. det
    std::vector<Vector3d<int>> boxes_list = det.predict( { img } );
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
    std::vector<std::pair<int, float>> cls_results = cls.predict(crop_batch);
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
    const std::vector<std::pair<std::string, float>> rec_results = rec.predict(crop_batch);
    // output rec results
    for (size_t i = 0; i < rec_results.size(); ++i) {
        ocr_result[i].text = std::move(rec_results[i].first);
        ocr_result[i].score = rec_results[i].second;
    }

    // Parse ocr_result
}
