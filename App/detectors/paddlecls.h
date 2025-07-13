#pragma once

#include <onnxruntime_cxx_api.h>

#include "apss.h"
#include "predictorconfig.h"
#include "onnxinference.h"
#include "licensed/preprocess_op.h"
#include "licensed/postprocess_op.h"

using ClassificationTuple = std::tuple<std::vector<int>, std::vector<float>>;

class PaddleCls
{
public:
    explicit PaddleCls(const PredictorConfig &config,
                       const std::shared_ptr<Ort::Env> &env,
                       const std::shared_ptr<Ort::AllocatorWithDefaultOptions> &allocator,
                       const std::shared_ptr<Ort::MemoryInfo> &memoryInfo);

    // Returns cls_labels, cls_scores
    std::vector<std::pair<int, float>> predict(const MatList &batch);
    bool readInCharDict(const std::string &filepath) noexcept;
    double clsThreshold() const;
    const ONNXInference &inferSession() const;

private:
    ONNXInference m_inferSession;

    // pre-process
    PaddleOCR::ClsResizeImg m_resizeOp;
    PaddleOCR::Normalize m_normalizeOp;
    PaddleOCR::PermuteBatch m_permuteOp;

    // post-process
    PaddleOCR::DBPostProcessor m_postProcessor;

    double m_clsThreshold = 0.9;
    bool use_gpu_ = false;
    int gpu_id_ = 0;
    int gpu_mem_ = 4000;
    int cpu_math_library_num_threads_ = 4;
    bool use_mkldnn_ = false;

    std::vector<float> m_mean = {0.485f, 0.456f, 0.406f};
    std::vector<float> m_std = {1 / 0.229f, 1 / 0.224f, 1 / 0.225f};
    double m_scale = 0.00392156862745098;
    bool m_isScale = true;
    bool m_useTensorRT = false;
    std::string precision_ = "fp32";
    int m_maxBatchSize = 1;

    std::vector<int> m_clsImageShape;
    std::vector<std::string> m_labels;

    std::vector<int> m_resizeImgSize = { 160, 80 };
    int m_channelNum = 3;
    std::string m_normImgOrder;
};
