#pragma once

#include <mutex>

#include "wrappers/customallocator.h"
#include <onnxruntime_cxx_api.h>

#include "predictorconfig.h"

class ONNXInference
{
public:
    explicit ONNXInference(const PredictorConfig &config,
                  const std::shared_ptr<Ort::Env> &env,
                  const std::shared_ptr<CustomAllocator> &allocator,
                  const std::shared_ptr<Ort::MemoryInfo> &memoryInfo);

    std::vector<Ort::Value> predictRaw(std::vector<float> data,
                                       std::vector<int64_t> customInputTensorShape = {});
    void printModelMetadata() const;
    void printSessionMetadata() const;
    const Ort::ModelMetadata &modelMetadata() const;
    OrtAllocator *allocator() const;
    std::shared_ptr<CustomAllocator> customAllocator() const;
    std::shared_ptr<Ort::MemoryInfo> memoryInfo() const;
    std::vector<std::vector<int64_t> > inputTensorShapes() const;
    std::vector<std::vector<int64_t> > outputTensorShapes() const;
    size_t numInputNodes() const;
    size_t numOutputNodes() const;
    int modelStride() const;
    std::vector<std::string> classNames() const;
    std::mutex &mtx();

    std::vector<const char *> inputNames() const;
    std::vector<const char *> outputNames() const;

protected:
    const Ort::Session &session() const;

private:
    PredictorConfig m_config;
    std::shared_ptr<Ort::Env> m_env;
    Ort::Session m_session { nullptr };
    std::shared_ptr<CustomAllocator> m_allocator;
    std::shared_ptr<Ort::MemoryInfo> m_memoryInfo;
    std::vector<std::vector<int64_t>> m_inputTensorShapes;
    std::vector<std::vector<int64_t>> m_outputTensorShapes;
    size_t m_numInputNodes = 1, m_numOutputNodes = 1;
    bool m_hasDynamicBatch = false;
    bool m_hasDynamicShape = false;
    int m_modelStride = 32;
    Ort::ModelMetadata m_modelMetadata;

    // Vectors to hold allocated input and output node names
    std::vector<Ort::AllocatedStringPtr> m_inputNodeNameAllocatedStrings;
    std::vector<Ort::AllocatedStringPtr> m_outputNodeNameAllocatedStrings;
    std::vector<const char *> m_inputNames;
    std::vector<const char *> m_outputNames;
    std::vector<std::string> m_availableEPProviders;
    std::vector<std::string> m_selectedEPProviders;

    std::vector<std::string> m_classNames;            // Vector of class names loaded from file

    mutable std::mutex m_mtx;
};
