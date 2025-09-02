#pragma once

#include <onnxruntime_cxx_api.h>

#include <apss.h>
#include <config/predictorconfig.h>
#include <detectors/onnxinference.h>
#include <detectors/licensed/preprocess_op.h>

class PaddleRec
{
public:
    explicit PaddleRec(const PredictorConfig &config,
                       const std::shared_ptr<Ort::Env> &env,
                       const std::shared_ptr<CustomAllocator> &allocator,
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

    std::vector<float> m_mean = {0.5f, 0.5f, 0.5f};
    std::vector<float> m_std = {1 / 0.5f, 1 / 0.5f, 1 / 0.5f};
    double m_scale = 1.0 / 255;
    std::string m_precision = "fp32";
    int m_maxBatchSize = 6;
    std::string m_imgMode = "BGR";
    std::vector<int> m_imgShape = {3, 48, 320};

    // post-process
    std::vector<std::string> m_labels;
};
