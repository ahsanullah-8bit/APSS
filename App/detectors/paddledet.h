// Disclaimer: This implementation is heavily inspired by PaddleOCR cpp_infer example's implementation

#pragma once

#include <onnxruntime_cxx_api.h>

#include "apss.h"
#include "config/predictorconfig.h"
#include "onnxinference.h"
#include "licensed/preprocess_op.h"
#include "licensed/postprocess_op.h"

class PaddleDet {
public:
    explicit PaddleDet(const PredictorConfig &config,
                       const std::shared_ptr<Ort::Env> &env,
                       const std::shared_ptr<CustomAllocator> &allocator,
                       const std::shared_ptr<Ort::MemoryInfo> &memoryInfo);
    std::vector<Vector3d<int>> predict(const MatList &batch);
    const ONNXInference &inferSession() const;

private:
    ONNXInference m_inferSession;

    // pre-process
    PaddleOCR::ResizeImgType0 m_resizeOp;
    PaddleOCR::Normalize m_normalizeOp;
    PaddleOCR::Permute m_permuteOp;

    int m_resizeLong = 960;
    std::vector<float> m_mean = {0.485f, 0.456f, 0.406f};
    std::vector<float> m_std = {1 / 0.229f, 1 / 0.224f, 1 / 0.225f};
    double m_scale = 1.0 / 255;
    std::string m_imgMode = "BGR";
    std::string m_normImgOrder = "hwc";
    std::string m_precision = "fp32";

    // post-process
    PaddleOCR::DBPostProcessor m_postProcessor;

    double m_threshold = 0.3;
    double m_boxThresh = 0.5;
    double m_unclipRatio = 2.0;
    std::string m_limitType = "max";
    std::string m_detDbScoreMode = "slow";
    bool m_useDilation = false;
};
