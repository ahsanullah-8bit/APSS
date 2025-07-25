#include <filesystem>

#include "onnxinference.h"

#include "image.h"

ONNXInference::ONNXInference(const PredictorConfig &config,
                             const std::shared_ptr<Ort::Env> &env,
                             const std::shared_ptr<CustomAllocator> &allocator,
                             const std::shared_ptr<Ort::MemoryInfo> &memoryInfo)
    : m_env(env)
    , m_allocator(allocator)
    , m_memoryInfo(memoryInfo)
    , m_config(config)
{
    if (!m_env)
        m_env = std::make_shared<Ort::Env>(ORT_LOGGING_LEVEL_WARNING, "ONNX_Inferece");

    if (!m_memoryInfo)
        m_memoryInfo = std::make_shared<Ort::MemoryInfo>(Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault));

    std::string model_path;
    if (!config.model.has_value())
        throw std::runtime_error("Model info is not specified!");

    model_path = config.model->path.value_or("");

    if (!std::filesystem::exists(model_path))
        throw std::runtime_error("Model is not available in the specified directory. Please download a model first and try again!");

    try {
        Ort::SessionOptions sessionOptions;
        int intraop_threads = std::min(4, static_cast<int>(std::thread::hardware_concurrency()));
        sessionOptions.SetIntraOpNumThreads(intraop_threads);
        sessionOptions.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);

        m_availableEPProviders = Ort::GetAvailableProviders();

#ifdef APSS_SUPPORT_CUDA_EP
        if (std::find(m_availableEPProviders.begin(), m_availableEPProviders.end(), "CUDAExecutionProvider")
            != m_availableEPProviders.end()) {

            OrtCUDAProviderOptions cudaOptions;
            sessionOptions.AppendExecutionProvider_CUDA(cudaOptions);

            m_selectedEPProviders.emplace_back("CUDAExecutionProvider");
        } else {
            qWarning() << std::format("Inference device {} not available", "CUDAExecutionProvider");
        }
#endif

#ifdef APSS_SUPPORT_OPENVINO_EP
        if (std::find(m_availableEPProviders.begin(), m_availableEPProviders.end(), "OpenVINOExecutionProvider")
            != m_availableEPProviders.end()) {

            std::unordered_map<std::string, std::string> options;
            options["device_type"] = "AUTO:GPU,CPU";
            options["precision"] = "ACCURACY";
            options["num_of_threads"] = "4";
            options["disable_dynamic_shapes"] = "false";
            sessionOptions.AppendExecutionProvider_OpenVINO_V2(options);

            m_selectedEPProviders.emplace_back("OpenVINOExecutionProvider");
        }
        else {
            qWarning() << std::format("Inference device {} not available", "OpenVINOExecutionProvider");
        }
#endif
        m_selectedEPProviders.emplace_back("CPUExecutionProvider");

#ifdef _WIN32
        std::wstring modelPath(model_path.begin(), model_path.end());
#else
        std::string modelPath(model_path);
#endif
        m_session = Ort::Session(*m_env, modelPath.c_str(), sessionOptions);

        if (!m_allocator)
            m_allocator = std::make_shared<CustomAllocator>(Ort::Allocator(m_session, *m_memoryInfo));

        m_numInputNodes = m_session.GetInputCount();
        m_numOutputNodes = m_session.GetOutputCount();

        // Input Nodes. Though all of them (YOLOs) have only a one input tensor.
        for (int i = 0; i < m_numInputNodes; ++i) {
            Ort::AllocatedStringPtr name_ptr = m_session.GetInputNameAllocated(i, m_allocator->get());
            m_inputNodeNameAllocatedStrings.emplace_back(std::move(name_ptr));
            m_inputNames.emplace_back(m_inputNodeNameAllocatedStrings.back().get());
        }

        // Output Nodes
        for (int i = 0; i < m_numOutputNodes; ++i) {
            Ort::AllocatedStringPtr name_ptr = m_session.GetOutputNameAllocated(i, m_allocator->get());
            m_outputNodeNameAllocatedStrings.emplace_back(std::move(name_ptr));
            m_outputNames.emplace_back(m_outputNodeNameAllocatedStrings.back().get());
        }

        // AFAIK, all the YOLO models have a single input tensor shape. So, we hardcoding.
        size_t input_count = m_session.GetInputCount();
        for (size_t i = 0; i < input_count; ++i) {
            Ort::TypeInfo input_type_info = m_session.GetInputTypeInfo(i);
            m_inputTensorShapes.emplace_back(input_type_info.GetTensorTypeAndShapeInfo().GetShape());
        }

        if (input_count > 1)
            qWarning() << "This inference session acknowledges multiple inputs, but will only use the first!!!";

        size_t output_count = m_session.GetOutputCount();
        for (size_t i = 0; i < output_count; ++i) {
            Ort::TypeInfo output_type_info = m_session.GetOutputTypeInfo(i);
            m_outputTensorShapes.emplace_back(output_type_info.GetTensorTypeAndShapeInfo().GetShape());
        }

        // METADATA
        // Read the stride
        m_modelMetadata = m_session.GetModelMetadata();
        Ort::AllocatedStringPtr stride = m_modelMetadata.LookupCustomMetadataMapAllocated("stride", m_allocator->get());
        if (stride) {
            std::string model_stride = stride.get();
            m_modelStride = std::stoi(model_stride);
        }

        // Names
        std::string labels_path = config.model->labelmap_path.value_or("");

        std::string model_names;
        Ort::AllocatedStringPtr names = m_modelMetadata.LookupCustomMetadataMapAllocated("names", m_allocator->get());
        if (names)
            model_names = names.get();

        // Load class names and generate corresponding colors
        if (!model_names.empty()){
            m_classNames = Utils::jsonToVecClassNames(model_names);
            qInfo() << "Classes found on the model's metadata, skip reading from file.";
        } else if (!labels_path.empty()) {
            m_classNames = Utils::readClassNames(labels_path);
        }
    }  catch (const Ort::Exception& e) {
        throw std::runtime_error(std::format("ONNXRuntime error {}: {}", static_cast<int>(e.GetOrtErrorCode()), e.what()));
    } catch (const std::exception& e) {
        throw std::runtime_error(std::format("Standard exception caught: {}", e.what()));
    }
}

std::vector<Ort::Value> ONNXInference::predictRaw(std::vector<float> data,
                                                  std::vector<int64_t> customInputTensorShape)
{
    if (customInputTensorShape.empty())
        customInputTensorShape = m_inputTensorShapes[0];

    // Check if the data is correctly placed
    if (data.size() != Utils::vectorProduct(customInputTensorShape)) {
        qWarning() << "Wrong size of pre-processed images provided, Skipping.";
        return {};
    }

    Ort::Value input_tensor = Ort::Value::CreateTensor<float>(
        *m_memoryInfo,
        data.data(),
        data.size(),
        customInputTensorShape.data(),
        customInputTensorShape.size()
        );

    std::vector<Ort::Value> output_tensors; // Should be
    {
        output_tensors = m_session.Run(
            Ort::RunOptions{nullptr},
            m_inputNames.data(),
            &input_tensor,
            m_numInputNodes,
            m_outputNames.data(),
            m_numOutputNodes
            );
    }

    return output_tensors;
}

void ONNXInference::printModelMetadata() const
{
    const Ort::ModelMetadata &model_metadata = modelMetadata();
    qDebug() << "Model metadata:";
    qDebug() << "\tFile:" << m_config.model->path.value_or("").c_str();
    qDebug() << "\tGraph Name:" << model_metadata.GetGraphNameAllocated(m_allocator->get()).get();

    std::vector<Ort::AllocatedStringPtr> keys = model_metadata.GetCustomMetadataMapKeysAllocated(m_allocator->get());
    qDebug() << "\tCustom Metadata:";
    for (const auto &key : keys) {
        qDebug() << "\t " << key.get() << ":" << model_metadata.LookupCustomMetadataMapAllocated(key.get(), m_allocator->get()).get();
    }

    qDebug() << "\tInputs:";
    for (size_t i = 0; i < m_numInputNodes; ++i) {
        qDebug() << "\t  Name:" << m_inputNames[i];
        qDebug() << "\t  Type:" << m_inputTensorShapes[i];
        qDebug();
    }

    qDebug() << "\tOutputs:";
    for (size_t i = 0; i < m_numOutputNodes; ++i) {
        qDebug() << "\t  Name:" << m_outputNames[i];
        qDebug() << "\t  Shape:" << m_outputTensorShapes[i];
        qDebug();
    }
}

void ONNXInference::printSessionMetadata() const
{
    qDebug() << "Session metadata:";
    qDebug() << "\tAvailable EPs:" << m_availableEPProviders;
    qDebug() << "\tSelected EPs:" << m_selectedEPProviders;
    qDebug() << "\tEmbedded EPs Info:";
    std::vector<Ort::ConstEpDevice> ep_devices = m_env->GetEpDevices();
    for (const auto &ep : ep_devices) {
        qDebug() << "\t  Name:" << ep.EpName();
        qDebug() << "\t  Vendor:" << ep.EpVendor();

        qDebug() << "\t  Metadata:";
        for (const auto &[key, val] : ep.EpMetadata().GetKeyValuePairs())
            qDebug() << "\t    " << key.c_str() << val.c_str();

        Ort::ConstHardwareDevice device = ep.Device();
        qDebug() << "\t  Device:";
        qDebug() << "\t    Id:" << device.DeviceId();
        qDebug() << "\t    Type:" << device.Type();

        qDebug() << "\t    Metadata:";
        for (const auto &[key, val] : device.Metadata().GetKeyValuePairs())
            qDebug() << "\t      " << key.c_str() << val.c_str();

        qDebug() << "\t    Vendor:" << device.Vendor();
        qDebug() << "\t    Vendor Id:" << device.VendorId();

        qDebug() << "\t  Options:";
        for (const auto &[key, val] : ep.EpOptions().GetKeyValuePairs())
            qDebug() << "\t    " << key.c_str() << val.c_str();

        qDebug();
    }
}

const Ort::ModelMetadata &ONNXInference::modelMetadata() const
{
    return m_modelMetadata;
}

OrtAllocator* ONNXInference::allocator() const
{
    return m_allocator->get();
}

std::shared_ptr<CustomAllocator> ONNXInference::customAllocator() const
{
    return m_allocator;
}

std::shared_ptr<Ort::MemoryInfo> ONNXInference::memoryInfo() const
{
    return m_memoryInfo;
}

std::vector<std::vector<int64_t> > ONNXInference::inputTensorShapes() const
{
    return m_inputTensorShapes;
}

std::vector<std::vector<int64_t> > ONNXInference::outputTensorShapes() const
{
    return m_outputTensorShapes;
}

size_t ONNXInference::numInputNodes() const
{
    return m_numInputNodes;
}

size_t ONNXInference::numOutputNodes() const
{
    return m_numOutputNodes;
}

int ONNXInference::modelStride() const
{
    return m_modelStride;
}

std::vector<std::string> ONNXInference::classNames() const
{
    return m_classNames;
}

std::mutex &ONNXInference::mtx()
{
    return m_mtx;
}

std::vector<const char *> ONNXInference::inputNames() const
{
    return m_inputNames;
}

std::vector<const char *> ONNXInference::outputNames() const
{
    return m_outputNames;
}

const Ort::Session &ONNXInference::session() const
{
    return m_session;
}
