#include "objectdetector.h"

#include "utils.h"

ObjectDetector::ObjectDetector(const PredictorConfig &config)
    : Predictor(config)
{}

void ObjectDetector::draw(cv::Mat &image, const PredictionList &predictions, float maskAlpha) const
{
    Utils::drawDetections(image, predictions, classNames(), classColors());
}

std::vector<PredictionList> ObjectDetector::postprocess(const MatList &originalImages, const cv::Size &resizedImageShape, const std::vector<Ort::Value> &outputTensors, float confThreshold, float iouThreshold)
{
    std::vector<prediction_vec> results_list;

    if (outputTensors.size() != 1)
        throw std::runtime_error("Insufficient outputs from the model. Expected 1 output.");

    // [N, 4 + num_classes, num_preds] (expected).
    const Ort::Value &tensor0 = outputTensors[0];
    const std::vector<int64_t> shape0 = tensor0.GetTensorTypeAndShapeInfo().GetShape();
    const float* output0_data = tensor0.GetTensorData<float>(); // Extract raw output data from the first output tensor

    if (shape0.size() != 3)
        throw std::runtime_error("Unexpected output tensor shape. Expected [N, 84, num_detections].");

    const size_t batch_size = shape0.at(0);
    const size_t num_features = shape0.at(1);
    const size_t num_detections = shape0.at(2);
    const int num_classes = static_cast<int>(num_features) - 4;

    results_list.reserve(batch_size);
    for (size_t b = 0; b < batch_size; ++b) {
        prediction_vec results;

        std::vector<cv::Rect> boxes;
        std::vector<float> confs;
        std::vector<int> class_ids;
        std::vector<cv::Rect> nms_boxes;
        boxes.reserve(num_detections);
        confs.reserve(num_detections);
        class_ids.reserve(num_detections);
        nms_boxes.reserve(num_detections);

        /*
         * We got 8400 detections, of each of 84 features, of each batch.
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
        const float *batch_offsetptr = output0_data + b * (num_features * num_detections); // Jumps b * 84 * 8400 for batch b.
        for (size_t i = 0; i < num_detections; ++i) {
            // Since its [B, F, D], not [B, D, F]. We hover over each feature's detections
            float cx = batch_offsetptr[0 * num_detections + i];
            float cy = batch_offsetptr[1 * num_detections + i];
            float w = batch_offsetptr[2 * num_detections + i];
            float h = batch_offsetptr[3 * num_detections + i];

            int class_id = -1;
            float max_score = 0.0f;
            for (int c = 0; c < num_classes; ++c) {
                const float score = batch_offsetptr[(4 + c) * num_detections + i];
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
                                                     cv::Size(orig_w, orig_h),
                                                     true
                                                     );

            // Round coordinates for integer pixel positions
            scaled_box.x = std::round(scaled_box.x);
            scaled_box.y = std::round(scaled_box.y);
            scaled_box.width = std::round(scaled_box.width);
            scaled_box.height = std::round(scaled_box.height);

            // Adjust NMS box coordinates to prevent overlap between classes
            cv::Rect nms_box = scaled_box;
                // Arbitrary offset to differentiate classes
            nms_box.x += class_id * 7880;
            nms_box.y += class_id * 7880;

            boxes.emplace_back(scaled_box);
            confs.emplace_back(max_score);
            class_ids.emplace_back(class_id);
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
            prediction.className = classNames()[prediction.classId];

            results.emplace_back(prediction);
        }

        results_list.emplace_back(results);
    }

    return results_list;
}
