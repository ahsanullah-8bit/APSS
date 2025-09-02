#pragma once

#include <onnxruntime_cxx_api.h>

#include <apss.h>
#include <config/predictorconfig.h>
#include <detectors/onnxinference.h>
#include <detectors/licensed/preprocess_op.h>
#include <detectors/licensed/postprocess_op.h>

class PaddleCls
{
public:
    explicit PaddleCls(const PredictorConfig &config,
                       const std::shared_ptr<Ort::Env> &env,
                       const std::shared_ptr<CustomAllocator> &allocator,
                       const std::shared_ptr<Ort::MemoryInfo> &memoryInfo);

    // Returns cls_labels, cls_scores
    std::vector<std::pair<int, float>> predict(const MatList &batch);
    double threshold() const;
    const ONNXInference &inferSession() const;

private:
    ONNXInference m_inferSession;

    // pre-process
    PaddleOCR::ClsResizeImg m_resizeOp;
    PaddleOCR::Normalize m_normalizeOp;
    PaddleOCR::PermuteBatch m_permuteOp;

    std::vector<float> m_mean = {0.485f, 0.456f, 0.406f};
    std::vector<float> m_std = {1 / 0.229f, 1 / 0.224f, 1 / 0.225f};
    double m_scale = 0.003921568627451;
    std::string m_precision = "fp32";
    int m_maxBatchSize = 1;
    int m_channelNum = 3;
    std::vector<int> m_resizeImgSize = { 160, 80 };
    std::string m_normImgOrder;

    // post-process
    PaddleOCR::DBPostProcessor m_postProcessor;

    double m_threshold = 0.4;
    std::vector<std::string> m_labels;
};
