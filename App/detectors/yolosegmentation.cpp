#include "yolosegmentation.h"

#include "utils.h"

YOLOSegmentation::YOLOSegmentation(const std::string &modelPath, const std::string &labelsPath)
    : YOLOInference(modelPath, labelsPath)
{}

void YOLOSegmentation::drawPredictions(cv::Mat &image, const prediction_vec &predictions, float maskAlpha) const
{
    Utils::drawSegmentations(image, predictions, classNames(), classColors(), maskAlpha);
}

void YOLOSegmentation::drawPredictionsMask(cv::Mat &image, const prediction_vec &predictions, float maskAlpha) const
{
    Utils::drawSegmentationsAndBoxes(image, predictions, classNames(), classColors(), maskAlpha);
}

mat_vec YOLOSegmentation::cropPredictions(const cv::Mat &image, const prediction_vec &predictions) const
{
    mat_vec results;
    results.reserve(predictions.size());
    for (const Prediction& prediction : predictions) {
        cv::Mat crop;
        Utils::perspectiveCrop(image, crop, prediction.points);
        results.emplace_back(crop);
    }
    return results;
}

void YOLOSegmentation::drawPredictions(mat_vec &images, const std::vector<prediction_vec> &predictionsList, float maskAlpha) const
{
    YOLOInference::drawPredictions(images, predictionsList, maskAlpha);
}

void YOLOSegmentation::drawPredictionsMask(mat_vec &images, const std::vector<prediction_vec> &predictionsList, float maskAlpha) const
{
    YOLOInference::drawPredictionsMask(images, predictionsList, maskAlpha);
}

std::vector<mat_vec> YOLOSegmentation::cropPredictions(const mat_vec &images, const std::vector<prediction_vec> &predictionsList) const
{
    return YOLOInference::cropPredictions(images, predictionsList);
}

std::vector<prediction_vec> YOLOSegmentation::postprocess(const mat_vec &originalImages,
                                                                   const cv::Size &letterBoxedSize,
                                                                   const std::vector<Ort::Value> &outputTensors,
                                                                   float confThreshold,
                                                                   float iouThreshold)
{
    std::vector<prediction_vec> results_list;

    if (outputTensors.size() < 2)
        throw std::runtime_error("Insufficient outputs from the model. Expected at least 2 outputs.");

    // According to https://github.com/ultralytics/ultralytics/issues/14765
    // Outputs should be [N, (4 + 80 + 32 = 116), num_detections] + [N, 32, maskH, maskW]
    const Ort::Value &tensor0 = outputTensors[0];
    const Ort::Value &tensor1 = outputTensors[1];
    const std::vector<int64_t> shape0 = tensor0.GetTensorTypeAndShapeInfo().GetShape();
    const std::vector<int64_t> shape1 = tensor1.GetTensorTypeAndShapeInfo().GetShape();

    if (shape0.size() != 3 || shape1.size() != 4)
        throw std::runtime_error("Unexpected output1 shape. Expected [N, 116, num_detections] + [N, 32, maskH, maskW].");

    const float* output0_data = tensor0.GetTensorData<float>();
    const float* output1_data = tensor1.GetTensorData<float>();

    constexpr size_t num_bbox_params = 4;                                           // [N, 4✅ + 80 + 32, num_detections]
    constexpr size_t num_mask_coeffs = 32;                                          // [N, 4 + 80 + 32✅, num_detections]
    const size_t batch_size = shape0.at(0);                                         // [N✅, 116, num_detections]
    const size_t num_features = shape0.at(1);                                       // [N, 116✅, num_detections]
    const size_t num_detections = shape0.at(2);                                     // [N, 116, num_detections✅]
    const size_t num_classes = num_features - num_bbox_params - num_mask_coeffs;    // [N, 4 + 80✅ + 32, num_detections]
    const size_t num_feature_maps = shape1.at(1);                                   // [N, 32✅, maskH, maskW]
    const int mask_h = shape1.at(2);                                                // [N, 32, maskH✅, maskW]
    const int mask_w = shape1.at(3);                                                // [N, 32, maskH, maskW✅]

    constexpr int BOX_OFFSET = 0;
    constexpr int CLASS_CONF_OFFSET = BOX_OFFSET + num_bbox_params;
    const int MASK_COEFF_OFFSET = CLASS_CONF_OFFSET + num_classes;


    results_list.reserve(batch_size);
    for (size_t b = 0; b < batch_size; ++b) {
        prediction_vec results;

        // 1. Process prototype masks
        // Store all prototype masks in a vector for easy access
        const float *batch1_offsetptr = output1_data + b * (num_feature_maps * mask_h * mask_w);
        mat_vec feature_maps;
        feature_maps.reserve(num_feature_maps);
        for (size_t i = 0; i < num_feature_maps; ++i) {
            size_t offset = i * mask_h * mask_w;
            cv::Mat map(mask_h, mask_w, CV_32F, const_cast<float*>(batch1_offsetptr + offset));
            feature_maps.emplace_back(map);
        }

        // 2. Process detections
        std::vector<cv::Rect> boxes;
        std::vector<float> confidences;
        std::vector<int> class_ids;
        std::vector<std::vector<float>> mask_coeffs_list;
        boxes.reserve(num_detections);
        confidences.reserve(num_detections);
        class_ids.reserve(num_detections);
        mask_coeffs_list.reserve(num_detections);

        /*
         * Similar to that of detection outputs, we got 8400 detections, of each of 116 features, of each batch.
         * The structure of each batch laid out would look like this for [B, F, D]
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

        const float *batch0_offsetptr = output0_data + b * (num_features * num_detections); // We jump b * 116 * 8400 for each batch b.
        for (int i = 0; i < num_detections; ++i) {

            // bbox: cxcywh to xywh
            cv::Rect box;
            box.width =     std::round(batch0_offsetptr[2 * num_detections + i]);
            box.height =    std::round(batch0_offsetptr[3 * num_detections + i]);
            box.x =         std::round(batch0_offsetptr[0 * num_detections + i] - box.width / 2.0f);
            box.y =         std::round(batch0_offsetptr[1 * num_detections + i] - box.height / 2.0f);

            // Confidence: Find the max confidence among all
            float max_conf = 0.0f;
            int class_id = -1;
            for (int j = 0; j < num_classes; ++j) {
                float conf = batch0_offsetptr[(CLASS_CONF_OFFSET + j) * num_detections + i];
                if (max_conf < conf) {
                    max_conf = conf;
                    class_id = j;
                }
            }

            if (max_conf < confThreshold)
                continue;

            // Mask Coefficients
            std::vector<float> mask_coeffs(num_mask_coeffs);
            for (size_t j = 0; j < num_mask_coeffs; ++j)
                mask_coeffs[j] = batch0_offsetptr[(MASK_COEFF_OFFSET + j) * num_detections + i];

            boxes.emplace_back(box);
            confidences.emplace_back(max_conf);
            class_ids.push_back(class_id);
            mask_coeffs_list.emplace_back(mask_coeffs);
        }

        if (boxes.empty()) {
            results_list.emplace_back(results);
            continue;
        }

        // 3. Apply NMS
        std::vector<int> nms_indices;
        Utils::NMSBoxes(boxes, confidences, confThreshold, iouThreshold, nms_indices);

        if (nms_indices.empty()) {
            results_list.emplace_back(results);
            continue;
        }

        // 4. Prepare final results
        results.reserve(nms_indices.size());

        // Calculate letterbox parameters
        int orig_w = originalImages[b].cols;
        int orig_h = originalImages[b].rows;
        const float gain = std::min(static_cast<float>(letterBoxedSize.height) / orig_h,
                                    static_cast<float>(letterBoxedSize.width) / orig_w);
        const int scaled_w = static_cast<int>(orig_w * gain);
        const int scaled_h = static_cast<int>(orig_h * gain);
        const float pad_w = (letterBoxedSize.width - scaled_w) / 2.0f;
        const float pad_h = (letterBoxedSize.height - scaled_h) / 2.0f;
        // Precompute mask scaling factors
        const float mask_scale_x = static_cast<float>(mask_w) / letterBoxedSize.width;
        const float mask_scale_y = static_cast<float>(mask_h) / letterBoxedSize.height;

        for (const int indx : nms_indices) {
            Prediction prediction;
            prediction.box = boxes[indx];
            prediction.conf = confidences[indx];
            prediction.classId = class_ids[indx];
            prediction.className = classNames()[prediction.classId];

            // 1. Scale box to original image
            prediction.box = Utils::scaleCoords(letterBoxedSize, prediction.box, cv::Size(orig_w, orig_h), true);

            // 2. Process mask
            const std::vector<float>& mask_coeffs = mask_coeffs_list[indx];
            cv::Mat final_mask = cv::Mat::zeros(mask_h, mask_w, CV_32F);
            for (int i = 0; i < num_mask_coeffs; ++i)
                final_mask += mask_coeffs[i] * feature_maps[i];

            // Apply sigmoid activation
            final_mask = Utils::sigmoid(final_mask);

            // Crop mask to letterbox area with a slight padding to avoid border issues
            int x1 = static_cast<int>(std::round((pad_w - 0.1f) * mask_scale_x));
            int y1 = static_cast<int>(std::round((pad_h - 0.1f) * mask_scale_y));
            int x2 = static_cast<int>(std::round((letterBoxedSize.width - pad_w + 0.1f) * mask_scale_x));
            int y2 = static_cast<int>(std::round((letterBoxedSize.height - pad_h + 0.1f) * mask_scale_y));

            // Ensure coordinates are within mask bounds
            x1 = std::max(0, std::min(x1, mask_w - 1));
            y1 = std::max(0, std::min(y1, mask_h - 1));
            x2 = std::max(x1, std::min(x2, mask_w));
            y2 = std::max(y1, std::min(y2, mask_h));

            // Handle cases where cropping might result in zero area
            if (x2 <= x1 || y2 <= y1)
                continue; // Skip this mask as cropping is invalid

            cv::Rect crop_rect(x1, y1, x2 - x1, y2 - y1);
            cv::Mat cropped_mask = final_mask(crop_rect).clone(); // Clone to ensure data integrity

            // Resize to original dimensions
            cv::Mat resized_mask;
            cv::resize(cropped_mask, resized_mask, cv::Size(orig_w, orig_h), 0, 0, cv::INTER_LINEAR);

            // Threshold and convert to binary
            cv::Mat binary_mask;
            cv::threshold(resized_mask, binary_mask, 0.5, 255.0, cv::THRESH_BINARY);
            binary_mask.convertTo(binary_mask, CV_8U);

            // Crop to bounding box
            cv::Mat final_binary_mask = cv::Mat::zeros(cv::Size(orig_w, orig_h), CV_8U);
            cv::Rect roi = prediction.box & cv::Rect(0, 0, binary_mask.cols, binary_mask.rows); // Ensure ROI is within mask
            if (roi.area() > 0)
                binary_mask(roi).copyTo(final_binary_mask(roi));

            prediction.mask = final_binary_mask;
            results.emplace_back(prediction);
        }

        results_list.emplace_back(results);
    }

    return results_list;
}
