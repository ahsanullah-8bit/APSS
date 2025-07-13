#pragma once

#include <onnxruntime_cxx_api.h>

#include "apss.h"
#include "paddleocrconfig.h"
#include "predictorconfig.h"
#include "onnxinference.h"
#include "licensed/preprocess_op.h"

using RecognitionTuple = std::tuple<std::vector<std::string>, std::vector<float>>;

class PaddleRec
{
public:
    explicit PaddleRec(const PredictorConfig &config,
                       const std::shared_ptr<Ort::Env> &env,
                       const std::shared_ptr<Ort::AllocatorWithDefaultOptions> &allocator,
                       const std::shared_ptr<Ort::MemoryInfo> &memoryInfo);
    // Returns rec_text, rec_texts_score pairs
    std::vector<std::pair<std::string, float>> predict(const MatList &batch);
    const ONNXInference &inferSession() const;

private:
    ONNXInference m_inferSession;

    // pre-process
    PaddleOCR::CrnnResizeImg m_resizeOp;
    PaddleOCR::Normalize m_normalizeOp;
    PaddleOCR::PermuteBatch m_permuteOp;

    bool use_gpu_ = false;
    int gpu_id_ = 0;
    int gpu_mem_ = 4000;
    int cpu_math_library_num_threads_ = 4;
    bool use_mkldnn_ = false;

    std::vector<std::string> m_labels;

    std::vector<float> m_mean = {0.5f, 0.5f, 0.5f};
    std::vector<float> m_std = {1 / 0.5f, 1 / 0.5f, 1 / 0.5f};
    double m_scale = 1.0 / 255;
    bool m_useTensorRT = false;
    std::string precision_ = "fp32";
    int m_recBatchNum = 6;
    int m_maxBatchSize = 6;
    int m_recImgH = 32;
    int m_recImgW = 320;
    std::vector<int> m_recImageShape; // provided by OCRv4's or OCRv5's .yml file

    std::string m_imgMode = "BGR";
    std::vector<int> m_imgShape = {3, 48, 320};
};
