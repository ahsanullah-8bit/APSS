#include <yaml-cpp/yaml.h>

#include "poseestimator.h"

#include "image.h"

PoseEstimator::PoseEstimator(const PredictorConfig &config,
                             const std::shared_ptr<Ort::Env> &env,
                             const std::shared_ptr<CustomAllocator> &allocator,
                             const std::shared_ptr<Ort::MemoryInfo> &memoryInfo)
    : Predictor(config, env, allocator, memoryInfo)
{
    const auto &infer_session = inferSession();
    const auto &model_metadata = infer_session.modelMetadata();
    Ort::AllocatedStringPtr kpt_shape = model_metadata.LookupCustomMetadataMapAllocated("kpt_shape", infer_session.allocator());
    if (kpt_shape) {
        YAML::Node kpt_yaml = YAML::Load(kpt_shape.get());
        if (kpt_yaml)
            m_kptShape = kpt_yaml.as<std::vector<int>>();
    } else if (config.kpt_shape) {
        m_kptShape = config.kpt_shape.value();
    } else {
        qWarning() << "kpt_shape not found for model" << config.model->path << "assuming" << m_kptShape << "instead.";
    }

    m_classColors = Utils::generateColors(inferSession().classNames());
}

void PoseEstimator::draw(cv::Mat &image, const PredictionList &predictions, float maskAlpha) const
{
    if (m_skeleton.empty()) {
        qWarning() << "No skeleton is set. Please set a pose skeleton before drawing!";
        return;
    }

    Utils::drawPoseEstimation(image, predictions, inferSession().classNames(), m_skeleton);
}

void PoseEstimator::setPoseSkeleton(const std::vector<std::pair<int, int>> &poseSkeleton)
{
    m_skeleton = poseSkeleton;
}

std::vector<PredictionList> PoseEstimator::postprocess(const MatList &originalImages, const cv::Size &resizedImageShape, const std::vector<Ort::Value> &outputTensors, float confThreshold, float iouThreshold)
{
    std::vector<PredictionList> results_list;

    if (outputTensors.size() != 1)
        throw std::runtime_error("Insufficient outputs from the model. Expected 1 output.");

    static const std::vector<std::string> class_names = inferSession().classNames();
    const Ort::Value &tensor0 = outputTensors[0];
    // [N, 4 + num_classes + kpts * 3, num_preds]
    const std::vector<int64_t> shape0 = tensor0.GetTensorTypeAndShapeInfo().GetShape();
    // [N, cxcywh + classes_scores + kpts * xyv, num_preds] (expected).
    const float* output0_data = tensor0.GetTensorData<float>(); // Extract raw output data from the first output tensor

    if (shape0.size() != 3)
        throw std::runtime_error("Unexpected output tensor shape. Expected [N, 84, num_detections].");

    const size_t batch_size = shape0.at(0);
    const size_t num_features = shape0.at(1);
    const size_t num_predictions = shape0.at(2);
    const int num_classes = static_cast<int>(class_names.size());
    static const int features_per_keypoint = m_kptShape.empty() ? 3 : m_kptShape[1];
    const int num_keypoints = static_cast<int>(num_features - num_classes - 4) / features_per_keypoint;

    results_list.reserve(batch_size);
    for (size_t b = 0; b < batch_size; ++b) {
        PredictionList results;

        std::vector<cv::Rect> boxes;
        std::vector<float> confs;
        std::vector<int> class_ids;
        std::vector<std::vector<cv::Point3f>> keypoints_list;
        std::vector<cv::Rect> nms_boxes;
        boxes.reserve(num_predictions);
        confs.reserve(num_predictions);
        class_ids.reserve(num_predictions);
        keypoints_list.reserve(num_predictions);
        nms_boxes.reserve(num_predictions);

        /*
         * We got 8400 detections, of each of 17 features, of each batch.
         * D (detections) being the fastest-varying, then F (features) and then B (batch)
         * The structure of each batch is laid out something like this for [B, F, D]
            Batch 0
                Feature 0
                    Detections 0, 1, ... D-1
                Feature 1
                    Detections 0, 1, ... D-1
                ...
            Batch 1
                Feature 0
                    Detections 0, 1, ... D-1
                Feature 1
                    Detections 0, 1, ... D-1
                ...
            ...
        */
        const float *batch_offsetptr = output0_data + b * (num_features * num_predictions); // Jumps b * features * predictions for batch b.
        for (size_t i = 0; i < num_predictions; ++i) {
            // Since its [B, F, D], not [B, D, F]. We hover over each feature's detections
            float cx = batch_offsetptr[0 * num_predictions + i];
            float cy = batch_offsetptr[1 * num_predictions + i];
            float w = batch_offsetptr[2 * num_predictions + i];
            float h = batch_offsetptr[3 * num_predictions + i];

            // x0, y1, w2, h3, con4, 17 * 3
            int class_id = -1;
            float max_score = 0.0f;
            for (int c = 0; c < num_classes; ++c) {
                const float score = batch_offsetptr[(4 + c) * num_predictions + i];
                if (max_score < score) {
                    max_score = score;
                    class_id = c;
                }
            }

            if (max_score < confThreshold)
                continue;

            const int orig_w = originalImages[b].cols;
            const int orig_h = originalImages[b].rows;
            // cxcywh to xywh, and scaled
            cv::Rect scaled_box = Utils::scaleCoords(resizedImageShape,
                                                     cv::Rect(cx - w / 2.0f, cy - h / 2.0f, w, h),
                                                     cv::Size(orig_w, orig_h));

            // Adjust NMS box coordinates to prevent overlap between classes
            cv::Rect nms_box = scaled_box;
            // Arbitrary offset to differentiate classes
            nms_box.x += class_id * 7880;
            nms_box.y += class_id * 7880;

            // Extracting Keypoints
            std::vector<cv::Point3f> keypoints;
            for (int k = 0; k < num_keypoints; ++k) {
                const int offset = 4 + num_classes + k * features_per_keypoint;
                const int x = batch_offsetptr[offset * num_predictions + i];
                const int y = batch_offsetptr[(1 + offset) * num_predictions + i];
                const int conf = batch_offsetptr[(2 + offset) * num_predictions + i];

                cv::Point2f kpt = Utils::scaleCoords(resizedImageShape,
                                                  cv::Point2f(x, y),
                                                  cv::Size(orig_w, orig_h));
                keypoints.emplace_back(cv::Point3f(kpt.x, kpt.y, conf));
            }

            boxes.emplace_back(scaled_box);
            confs.emplace_back(max_score);
            class_ids.emplace_back(class_id);
            keypoints_list.emplace_back(keypoints);
            nms_boxes.emplace_back(nms_box);
        }

        // Apply Non-Maximum Suppression (NMS) to eliminate redundant detections
        std::vector<int> indices;
        Utils::NMSBoxes(nms_boxes, confs, confThreshold, iouThreshold, indices);

        results.reserve(indices.size());
        // Collect filtered detections into the result vector
        for (const int idx : indices) {
            Prediction prediction;
            prediction.box = boxes[idx];
            prediction.conf = confs[idx];
            prediction.classId = class_ids[idx];
            prediction.className = class_names[prediction.classId];
            prediction.points = keypoints_list[idx];

            results.emplace_back(prediction);
        }

        results_list.emplace_back(results);
    }

    return results_list;
}
