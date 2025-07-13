#include "image.h"

cv::Mat Utils::sigmoid(const cv::Mat &src) {
    cv::Mat dst;
    cv::exp(-src, dst);
    dst = 1.0 / (1.0 + dst);
    return dst;
}

std::vector<std::string> Utils::readClassNames(const std::string &path) {
    std::vector<std::string> classNames;
    std::ifstream infile(path);

    if (infile) {
        std::string line;
        while (getline(infile, line)) {
            // Remove carriage return if present (for Windows compatibility)
            if (!line.empty() && line.back() == '\r')
                line.pop_back();
            classNames.emplace_back(line);
        }
    } else {
        qCritical() << "Failed to access labelmap path: " << path;
    }

    qInfo() << "Loaded " << classNames.size() << " labels from " + path;
    return classNames;
}

std::vector<std::string> Utils::jsonToVecClassNames(const std::string &namesStd) {
    std::vector<std::string> result;
    // I know we should use one way or another (std or Qt), but I'm kinda busy to look at an alternative to QRegularExpression
    QString names = QString::fromStdString(namesStd);

    // Some slicing to be done.
    constexpr const char* CLASS_PATTERN = R"((\d+):\s'([^']+)')";

    QRegularExpression regex(CLASS_PATTERN);
    QRegularExpressionMatchIterator it = regex.globalMatch(names);
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        if (!match.hasMatch()) {
            qInfo() << "No match for this iteration.";
        }

        // QString indx = match.captured(1);
        QString val = match.captured(2);
        result.emplace_back(val.toStdString());
    }

    return result;
}

size_t Utils::vectorProduct(const std::vector<int64_t> &vector) {
    return std::accumulate(vector.begin(), vector.end(), 1ull, std::multiplies<size_t>());
}

void Utils::letterBox(const cv::Mat &image, cv::Mat &outImage, const cv::Size &newShape, const cv::Scalar &color, bool scale) {
    // Calculate the scaling ratio to fit the image within the new shape
    float ratio = std::min(static_cast<float>(newShape.height) / image.rows,
                           static_cast<float>(newShape.width) / image.cols);

    // Prevent scaling if not allowed
    if (!scale)
        ratio = std::min(ratio, 1.0f);

    // Calculate new dimensions after scaling
    cv::Size size_unpdd(std::round(image.cols * ratio), std::round(image.rows * ratio));

    // Calculate padding needed to reach the desired shape
    int pad_hori = newShape.width - size_unpdd.width;
    int pad_vert = newShape.height - size_unpdd.height;
    int pad_top = pad_vert / 2;
    int pad_bottom = pad_vert - pad_top;
    int pad_left = pad_hori / 2;
    int pad_right = pad_hori - pad_left;

    // Resize the image
    cv::resize(image, outImage, size_unpdd);
    // Add padding
    cv::copyMakeBorder(outImage, outImage, pad_top, pad_bottom, pad_left, pad_right, cv::BORDER_CONSTANT, color);
}

cv::Rect Utils::scaleCoords(const cv::Size &resizedImageShape, cv::Rect coords, const cv::Size &originalImageShape, bool p_Clip) {
    cv::Rect result;
    float gain = std::min(static_cast<float>(resizedImageShape.height) / static_cast<float>(originalImageShape.height),
                          static_cast<float>(resizedImageShape.width) / static_cast<float>(originalImageShape.width));

    int padX = static_cast<int>(std::round((resizedImageShape.width - originalImageShape.width * gain) / 2.0f));
    int padY = static_cast<int>(std::round((resizedImageShape.height - originalImageShape.height * gain) / 2.0f));

    result.x = static_cast<int>(std::round((coords.x - padX) / gain));
    result.y = static_cast<int>(std::round((coords.y - padY) / gain));
    result.width = static_cast<int>(std::round(coords.width / gain));
    result.height = static_cast<int>(std::round(coords.height / gain));

    if (p_Clip) {
        result.x = clamp(result.x, 0, originalImageShape.width);
        result.y = clamp(result.y, 0, originalImageShape.height);
        result.width = clamp(result.width, 0, originalImageShape.width - result.x);
        result.height = clamp(result.height, 0, originalImageShape.height - result.y);
    }
    return result;
}

cv::Point2f Utils::scaleCoords(const cv::Size &resizedImageShape, cv::Point2f point, const cv::Size &originalImageShape, bool p_Clip) {
    cv::Point2f result;
    float gain = std::min(static_cast<float>(resizedImageShape.height) / static_cast<float>(originalImageShape.height),
                          static_cast<float>(resizedImageShape.width) / static_cast<float>(originalImageShape.width));

    int padX = static_cast<int>(std::round((resizedImageShape.width - originalImageShape.width * gain) / 2.0f));
    int padY = static_cast<int>(std::round((resizedImageShape.height - originalImageShape.height * gain) / 2.0f));

    result.x = std::round((point.x - padX) / gain);
    result.y = std::round((point.y - padY) / gain);

    if (p_Clip) {
        result.x = clamp(result.x, 0.0f, (float)originalImageShape.width);
        result.y = clamp(result.y, 0.0f, (float)originalImageShape.height);
    }
    return result;
}

std::vector<cv::Point2f> Utils::scaleCoords(const cv::Size &resizedImageShape, const std::vector<cv::Point2f> &points, const cv::Size &originalImageShape, bool p_Clip) {
    std::vector<cv::Point2f> results;
    float gain = std::min(static_cast<float>(resizedImageShape.height) / static_cast<float>(originalImageShape.height),
                          static_cast<float>(resizedImageShape.width) / static_cast<float>(originalImageShape.width));

    int padX = static_cast<int>(std::round((resizedImageShape.width - originalImageShape.width * gain) / 2.0f));
    int padY = static_cast<int>(std::round((resizedImageShape.height - originalImageShape.height * gain) / 2.0f));

    for (const auto &point : points) {
        cv::Point2f result;
        result.x = std::round((point.x - padX) / gain);
        result.y = std::round((point.y - padY) / gain);

        if (p_Clip) {
            result.x = clamp(result.x, 0.0f, (float)originalImageShape.width);
            result.y = clamp(result.y, 0.0f, (float)originalImageShape.height);
        }

        results.emplace_back(result);
    }
    return results;
}

void Utils::NMSBoxes(const std::vector<cv::Rect> &boundingBoxes, const std::vector<float> &scores, float scoreThreshold, float nmsThreshold, std::vector<int> &indices)
{
    indices.clear();

    const size_t numBoxes = boundingBoxes.size();
    if (numBoxes == 0) {
        // DEBUG_PRINT("No bounding boxes to process in NMS");
        return;
    }

    // Step 1: Filter out boxes with scores below the threshold
    // and create a list of indices sorted by descending scores
    std::vector<int> sortedIndices;
    sortedIndices.reserve(numBoxes);
    for (size_t i = 0; i < numBoxes; ++i) {
        if (scores[i] >= scoreThreshold) {
            sortedIndices.push_back(static_cast<int>(i));
        }
    }

    // If no boxes remain after thresholding
    if (sortedIndices.empty()) {
        // DEBUG_PRINT("No bounding boxes above score threshold");
        return;
    }

    // Sort the indices based on scores in descending order
    std::sort(sortedIndices.begin(), sortedIndices.end(),
              [&scores](int idx1, int idx2) {
                  return scores[idx1] > scores[idx2];
              });

    // Step 2: Precompute the areas of all boxes
    std::vector<float> areas(numBoxes, 0.0f);
    for (size_t i = 0; i < numBoxes; ++i) {
        areas[i] = boundingBoxes[i].width * boundingBoxes[i].height;
    }

    // Step 3: Suppression mask to mark boxes that are suppressed
    std::vector<bool> suppressed(numBoxes, false);

    // Step 4: Iterate through the sorted list and suppress boxes with high IoU
    for (size_t i = 0; i < sortedIndices.size(); ++i) {
        int currentIdx = sortedIndices[i];
        if (suppressed[currentIdx]) {
            continue;
        }

        // Select the current box as a valid detection
        indices.push_back(currentIdx);

        const cv::Rect& currentBox = boundingBoxes[currentIdx];
        const float x1_max = currentBox.x;
        const float y1_max = currentBox.y;
        const float x2_max = currentBox.x + currentBox.width;
        const float y2_max = currentBox.y + currentBox.height;
        const float area_current = areas[currentIdx];

        // Compare IoU of the current box with the rest
        for (size_t j = i + 1; j < sortedIndices.size(); ++j) {
            int compareIdx = sortedIndices[j];
            if (suppressed[compareIdx]) {
                continue;
            }

            const cv::Rect& compareBox = boundingBoxes[compareIdx];
            const float x1 = std::max(x1_max, static_cast<float>(compareBox.x));
            const float y1 = std::max(y1_max, static_cast<float>(compareBox.y));
            const float x2 = std::min(x2_max, static_cast<float>(compareBox.x + compareBox.width));
            const float y2 = std::min(y2_max, static_cast<float>(compareBox.y + compareBox.height));

            const float interWidth = x2 - x1;
            const float interHeight = y2 - y1;

            if (interWidth <= 0 || interHeight <= 0) {
                continue;
            }

            const float intersection = interWidth * interHeight;
            const float unionArea = area_current + areas[compareIdx] - intersection;
            const float iou = (unionArea > 0.0f) ? (intersection / unionArea) : 0.0f;

            if (iou > nmsThreshold) {
                suppressed[compareIdx] = true;
            }
        }
    }

    // DEBUG_PRINT("NMS completed with " + std::to_string(indices.size()) + " indices remaining");
}

std::vector<cv::Scalar> Utils::generateColors(const std::vector<std::string> &classNames, int seed) {
    // Static cache to store colors based on class names to avoid regenerating
    static std::unordered_map<size_t, std::vector<cv::Scalar>> colorCache;

    // Compute a hash key based on class names to identify unique class configurations
    size_t hashKey = 0;
    for (const auto& name : classNames) {
        hashKey ^= std::hash<std::string>{}(name) + 0x9e3779b9 + (hashKey << 6) + (hashKey >> 2);
    }

    // Check if colors for this class configuration are already cached
    auto it = colorCache.find(hashKey);
    if (it != colorCache.end()) {
        return it->second;
    }

    // Generate unique random colors for each class
    std::vector<cv::Scalar> colors;
    colors.reserve(classNames.size());

    std::mt19937 rng(seed); // Initialize random number generator with fixed seed
    std::uniform_int_distribution<int> uni(0, 255); // Define distribution for color values

    for (size_t i = 0; i < classNames.size(); ++i) {
        colors.emplace_back(cv::Scalar(uni(rng), uni(rng), uni(rng))); // Generate random BGR color
    }

    // Cache the generated colors for future use
    colorCache.emplace(hashKey, colors);

    return colorCache[hashKey];
}

void Utils::drawBoundingBox(cv::Mat &image, const std::vector<Prediction> &predictions, const std::vector<std::string> &classNames, const std::vector<cv::Scalar> &colors) {
    for (const auto& prediction : predictions) {
        // Ensure the object ID is within valid range
        if (prediction.classId < 0 || static_cast<size_t>(prediction.classId) >= classNames.size())
            continue;

        // Select color based on object ID for consistent coloring
        const cv::Scalar& color = colors[prediction.classId % colors.size()];

        // Draw the bounding box rectangle
        cv::rectangle(image,
                      cv::Point(prediction.box.x, prediction.box.y),
                      cv::Point(prediction.box.x + prediction.box.width, prediction.box.y + prediction.box.height),
                      color, 2, cv::LINE_AA);
        // cv::rectangle(image, prediction.box, color, 2, cv::LINE_AA);

        // Prepare label text with class name and confidence percentage
        // std::string label = classNames[prediction.classId] + ": " + std::to_string(static_cast<int>(prediction.conf * 100)) + "%";
        std::string label = std::format("{} - {} - {}%", classNames[prediction.classId], prediction.trackerId,  (int)(prediction.conf * 100));

        // Define text properties for labels
        int fontFace = cv::FONT_HERSHEY_SIMPLEX;
        double fontScale = std::min(image.rows, image.cols) * 0.0008;
        const int thickness = std::max(1, static_cast<int>(std::min(image.rows, image.cols) * 0.002));
        int baseline = 0;

        // Calculate text size for background rectangles
        cv::Size textSize = cv::getTextSize(label, fontFace, fontScale, thickness, &baseline);

        // Define positions for the label
        int labelY = std::max(prediction.box.y, textSize.height + 5);
        cv::Point labelTopLeft(prediction.box.x, labelY - textSize.height - 5);
        cv::Point labelBottomRight(prediction.box.x + textSize.width + 5, labelY + baseline - 5);

        // Draw background rectangle for label
        cv::rectangle(image, labelTopLeft, labelBottomRight, color, cv::FILLED);

        // Put label text
        cv::putText(image, label, cv::Point(prediction.box.x + 2, labelY - 2), fontFace, fontScale, cv::Scalar(255, 255, 255), thickness, cv::LINE_AA);
    }
}

void Utils::drawBoundingBoxMask(cv::Mat &image, const std::vector<Prediction> &predictions, const std::vector<std::string> &classNames, const std::vector<cv::Scalar> &classColors, float maskAlpha) {
    // Validate input image
    if (image.empty()) {
        qWarning() << "Empty image provided to drawBoundingBoxMask, skipping!";
        return;
    }

    const int imgHeight = image.rows;
    const int imgWidth = image.cols;

    // Precompute dynamic font size and thickness based on image dimensions
    const double fontSize = std::min(imgHeight, imgWidth) * 0.0006;
    const int textThickness = std::max(1, static_cast<int>(std::min(imgHeight, imgWidth) * 0.001));

    // Create a mask image for blending (initialized to zero)
    cv::Mat maskImage(image.size(), image.type(), cv::Scalar::all(0));

    // Pre-filter detections to include only those above the confidence threshold and with valid class IDs
    std::vector<const Prediction*> filteredDetections;
    for (const auto& prediction : predictions) {
        if (prediction.classId >= 0
            && static_cast<size_t>(prediction.classId) < classNames.size()) {
            filteredDetections.emplace_back(&prediction);
        }
    }

    // Draw filled rectangles on the mask image for the semi-transparent overlay
    for (const auto* detection : filteredDetections) {
        cv::Rect box(detection->box.x, detection->box.y, detection->box.width, detection->box.height);
        const cv::Scalar &color = classColors[detection->classId];
        cv::rectangle(maskImage, detection->box, color, cv::FILLED);
    }

    // Blend the maskImage with the original image to apply the semi-transparent masks
    cv::addWeighted(maskImage, maskAlpha, image, 1.0f, 0, image);

    // Draw bounding boxes and labels on the original image
    for (const auto* detection : filteredDetections) {
        cv::Rect box = detection->box;
        const cv::Scalar &color = classColors[detection->classId];
        cv::rectangle(image, box, color, 2, cv::LINE_AA);

        // std::string label = classNames[detection->classId] + ": " + std::to_string(static_cast<int>(detection->conf * 100)) + "%";
        std::string label = std::format("{} - {} - {}%", classNames[detection->classId], detection->trackerId,  (int)(detection->conf * 100));
        int baseLine = 0;
        cv::Size labelSize = cv::getTextSize(label, cv::FONT_HERSHEY_SIMPLEX, fontSize, textThickness, &baseLine);

        int labelY = std::max(detection->box.y, labelSize.height + 5);
        cv::Point labelTopLeft(detection->box.x, labelY - labelSize.height - 5);
        cv::Point labelBottomRight(detection->box.x + labelSize.width + 5, labelY + baseLine - 5);

        // Draw background rectangle for label
        cv::rectangle(image, labelTopLeft, labelBottomRight, color, cv::FILLED);

        // Put label text
        cv::putText(image, label, cv::Point(detection->box.x + 2, labelY - 2), cv::FONT_HERSHEY_SIMPLEX, fontSize, cv::Scalar(255, 255, 255), textThickness, cv::LINE_AA);
    }
}

void Utils::drawDetections(cv::Mat &image, const std::vector<Prediction> &predictions, const std::vector<std::string> &classNames, const std::vector<cv::Scalar> &classColors, bool drawMask, float maskAlpha, bool drawBoxes, bool drawLabels) {
    // Validate inputs
    if (image.empty()) {
        qWarning() << "Empty image provided to drawDetections, skipping!";
        return;
    }

    const int imgHeight = image.rows;
    const int imgWidth = image.cols;

    // Precompute dynamic text properties once
    const double fontSize = std::min(imgHeight, imgWidth) * 0.0006;
    const int textThickness = std::max(1, static_cast<int>(std::min(imgHeight, imgWidth) * 0.001));
    const int boxThickness = std::max(1, static_cast<int>(std::min(imgHeight, imgWidth) * 0.002));

    // Create mask layer if needed
    cv::Mat maskLayer;
    if (drawMask) {
        maskLayer = cv::Mat::zeros(image.size(), image.type());
    }

    // Process each detection
    for (const auto& pred : predictions) {
        // Validate class ID
        if (pred.classId < 0 || static_cast<size_t>(pred.classId) >= classNames.size()) {
            continue;
        }

        const cv::Scalar& color = classColors[pred.classId % classColors.size()];
        const cv::Rect& box = pred.box;

        // Draw mask if enabled
        if (drawMask) {
            cv::rectangle(maskLayer, box, color, cv::FILLED);
        }

        // Draw bounding box if enabled
        if (drawBoxes) {
            cv::rectangle(image, box, color, boxThickness, cv::LINE_AA);
        }

        // Draw label if enabled
        if (drawLabels) {
            std::string label = std::format("{}:{} ({}%)",
                                            classNames[pred.classId],
                                            pred.trackerId,
                                            static_cast<int>(pred.conf * 100));

            int baseline = 0;
            cv::Size labelSize = cv::getTextSize(label, cv::FONT_HERSHEY_SIMPLEX,
                                                 fontSize, textThickness, &baseline);

            // Calculate label position (avoid going off-screen)
            int labelY = std::max(box.y, labelSize.height + 2);
            cv::Point textOrg(box.x, labelY - 2);

            // Draw label background
            cv::rectangle(image,
                          cv::Point(box.x, labelY - labelSize.height - 2),
                          cv::Point(box.x + labelSize.width + 2, labelY + baseline),
                          color, cv::FILLED);

            // Draw text
            cv::putText(image, label, textOrg,
                        cv::FONT_HERSHEY_SIMPLEX, fontSize,
                        cv::Scalar(255, 255, 255), textThickness, cv::LINE_AA);
        }
    }

    // Blend mask layer if created
    if (drawMask && !maskLayer.empty()) {
        cv::addWeighted(maskLayer, maskAlpha, image, 1.0f, 0, image);
    }
}

void Utils::drawOCR(cv::Mat &image,
                    const std::vector<PaddleOCR::OCRPredictResult> &predictions,
                    bool drawMask, float maskAlpha,
                    bool drawBoxes, bool drawLabels)
{
    if (image.empty()) {
        qWarning() << "Empty image provided to drawOCR, skipping!";
        return;
    }

    const int imgHeight = image.rows;
    const int imgWidth = image.cols;

    const double fontSize = std::min(imgHeight, imgWidth) * 0.0006;
    const int textThickness = std::max(1, static_cast<int>(std::min(imgHeight, imgWidth) * 0.001));
    const int boxThickness = std::max(1, static_cast<int>(std::min(imgHeight, imgWidth) * 0.002));

    cv::Mat maskLayer;
    if (drawMask) {
        maskLayer = cv::Mat::zeros(image.size(), image.type());
    }

    const cv::Scalar defaultBoxColor(0, 255, 0); // Green color for boxes and masks

    for (const auto& pred : predictions) {
        if (pred.box.size() != 4) {
            qWarning() << "OCR prediction box does not have 4 points, skipping!";
            continue;
        }

        std::vector<cv::Point> contour;
        for (const auto& point_pair : pred.box) {
            if (point_pair.size() == 2) {
                contour.emplace_back(point_pair[0], point_pair[1]);
            } else {
                qWarning() << "OCR box point is not a pair (x,y), skipping!";
                continue;
            }
        }

        if (contour.size() != 4) {
            continue;
        }

        const std::vector<std::vector<cv::Point>> contours = {contour};

        if (drawMask) {
            cv::fillPoly(maskLayer, contours, defaultBoxColor);
        }

        if (drawBoxes) {
            cv::polylines(image, contours, true, defaultBoxColor, boxThickness, cv::LINE_AA);
        }

        // Always draw text on top, similar to drawLabels
        std::string label;
        try {
            label = std::format("{} ({:.1f}%)", pred.text, pred.score * 100);
        } catch (const std::bad_alloc&) {
            std::stringstream ss;
            ss << pred.text << " (" << std::fixed << std::setprecision(1) << pred.score * 100 << "%)";
            label = ss.str();
        }

        int baseline = 0;
        cv::Size labelSize = cv::getTextSize(label, cv::FONT_HERSHEY_SIMPLEX,
                                             fontSize, textThickness, &baseline);

        int labelX = pred.box[0][0];
        int labelY = std::max(pred.box[0][1], labelSize.height + 2);

        cv::Point textOrg(labelX, labelY - 2);

        cv::rectangle(image,
                      cv::Point(labelX, labelY - labelSize.height - 2),
                      cv::Point(labelX + labelSize.width + 2, labelY + baseline),
                      defaultBoxColor, cv::FILLED);

        cv::putText(image, label, textOrg,
                    cv::FONT_HERSHEY_SIMPLEX, fontSize,
                    cv::Scalar(255, 255, 255), textThickness, cv::LINE_AA);
    }

    if (drawMask && !maskLayer.empty()) {
        cv::addWeighted(maskLayer, maskAlpha, image, 1.0f, 0, image);
    }
}

void Utils::drawSegmentationsAndBoxes(cv::Mat &image, const std::vector<Prediction> &predictions, const std::vector<std::string> &classNames, const std::vector<cv::Scalar> &classColors, float maskAlpha)
{
    for (const auto &seg : predictions) {
        const cv::Scalar &color = classColors[seg.classId % classColors.size()];

        // -----------------------------
        // 1. Draw Bounding Box
        // -----------------------------
        cv::rectangle(image, seg.box, color, 2);

        // -----------------------------
        // 2. Draw Label
        // -----------------------------
        std::string label = classNames[seg.classId] + " " + std::to_string(static_cast<int>(seg.conf * 100)) + "%";
        int baseLine = 0;
        constexpr double fontScale = 0.5;
        constexpr int thickness = 1;
        cv::Size labelSize = cv::getTextSize(label, cv::FONT_HERSHEY_SIMPLEX, fontScale, thickness, &baseLine);
        int top = std::max(seg.box.y, labelSize.height + 5);
        cv::rectangle(image,
                      cv::Point(seg.box.x, top - labelSize.height - 5),
                      cv::Point(seg.box.x + labelSize.width + 5, top),
                      color, cv::FILLED);
        cv::putText(image, label,
                    cv::Point(seg.box.x + 2, top - 2),
                    cv::FONT_HERSHEY_SIMPLEX,
                    fontScale,
                    cv::Scalar(255, 255, 255),
                    thickness);

        // -----------------------------
        // 3. Apply Segmentation Mask
        // -----------------------------
        if (!seg.mask.empty()) {
            // Ensure the mask is single-channel
            cv::Mat mask_gray;
            if (seg.mask.channels() == 3) {
                cv::cvtColor(seg.mask, mask_gray, cv::COLOR_BGR2GRAY);
            } else {
                mask_gray = seg.mask.clone();
            }

            // Threshold the mask to binary (object: 255, background: 0)
            cv::Mat mask_binary;
            cv::threshold(mask_gray, mask_binary, 127, 255, cv::THRESH_BINARY);

            // Create a colored version of the mask
            cv::Mat colored_mask;
            cv::cvtColor(mask_binary, colored_mask, cv::COLOR_GRAY2BGR);
            colored_mask.setTo(color, mask_binary); // Apply color where mask is present

            // Blend the colored mask with the original image
            cv::addWeighted(image, 1.0, colored_mask, maskAlpha, 0, image);
        }
    }
}

void Utils::drawSegmentations(cv::Mat &image, const std::vector<Prediction> &predictions, const std::vector<std::string> &classNames, const std::vector<cv::Scalar> &classColors, float maskAlpha)
{
    Q_UNUSED(classNames);

    for (const auto &seg : predictions) {
        const cv::Scalar &color = classColors[seg.classId % classColors.size()];

        // -----------------------------
        // Draw Segmentation Mask Only
        // -----------------------------
        if (!seg.mask.empty()) {
            // Ensure the mask is single-channel
            cv::Mat mask_gray;
            if (seg.mask.channels() == 3) {
                cv::cvtColor(seg.mask, mask_gray, cv::COLOR_BGR2GRAY);
            } else {
                mask_gray = seg.mask.clone();
            }

            // Threshold the mask to binary (object: 255, background: 0)
            cv::Mat mask_binary;
            cv::threshold(mask_gray, mask_binary, 127, 255, cv::THRESH_BINARY);

            // Create a colored version of the mask
            cv::Mat colored_mask;
            cv::cvtColor(mask_binary, colored_mask, cv::COLOR_GRAY2BGR);
            colored_mask.setTo(color, mask_binary); // Apply color where mask is present

            // Blend the colored mask with the original image
            cv::addWeighted(image, 1.0, colored_mask, maskAlpha, 0, image);
        }
    }
}

void Utils::drawsSegmentations(cv::Mat &image, const std::vector<Prediction> &predictions, const std::vector<std::string> &classNames, const std::vector<cv::Scalar> &classColors, bool drawMasks, float maskAlpha, bool drawBoxes, bool drawLabels) {
    // Validate input
    if (image.empty()) {
        qWarning() << "Empty image provided to drawDetections, skipping!";
        return;
    }

    const int imgHeight = image.rows;
    const int imgWidth = image.cols;

    // Precompute dynamic properties once
    const double fontScale = std::min(imgHeight, imgWidth) * 0.0006;
    const int textThickness = std::max(1, static_cast<int>(std::min(imgHeight, imgWidth) * 0.001));
    const int boxThickness = std::max(1, static_cast<int>(std::min(imgHeight, imgWidth) * 0.002));

    // Create mask layer if needed
    cv::Mat maskLayer;
    if (drawMasks) {
        maskLayer = cv::Mat::zeros(image.size(), image.type());
    }

    // Process each detection
    for (const auto& pred : predictions) {
        // Validate class ID
        if (pred.classId < 0 || static_cast<size_t>(pred.classId) >= classNames.size()) {
            continue;
        }

        const cv::Scalar& color = classColors[pred.classId % classColors.size()];
        const cv::Rect& box = pred.box;

        // -----------------------------
        // Process segmentation mask
        // -----------------------------
        if (drawMasks && !pred.mask.empty()) {
            cv::Mat maskGray;
            // Convert to single channel if needed
            if (pred.mask.channels() == 3) {
                cv::cvtColor(pred.mask, maskGray, cv::COLOR_BGR2GRAY);
            } else {
                maskGray = pred.mask;
            }

            // Threshold to binary mask
            cv::Mat maskBinary;
            cv::threshold(maskGray, maskBinary, 127, 255, cv::THRESH_BINARY);

            // Create colored mask
            cv::Mat coloredMask;
            cv::cvtColor(maskBinary, coloredMask, cv::COLOR_GRAY2BGR);
            coloredMask.setTo(color, maskBinary);

            // Add to mask layer
            cv::addWeighted(maskLayer, 1.0, coloredMask, 1.0, 0, maskLayer);
        }

        // -----------------------------
        // Draw bounding box
        // -----------------------------
        if (drawBoxes) {
            cv::rectangle(image, box, color, boxThickness, cv::LINE_AA);
        }

        // -----------------------------
        // Draw label
        // -----------------------------
        if (drawLabels) {
            // Format label text
            std::string label;
            if (pred.trackerId >= 0) {
                label = std::format("{}:{} ({}%)",
                                    classNames[pred.classId],
                                    pred.trackerId,
                                    static_cast<int>(pred.conf * 100));
            } else {
                label = std::format("{} ({}%)",
                                    classNames[pred.classId],
                                    static_cast<int>(pred.conf * 100));
            }

            // Calculate text size
            int baseline = 0;
            cv::Size labelSize = cv::getTextSize(label, cv::FONT_HERSHEY_SIMPLEX,
                                                 fontScale, textThickness, &baseline);

            // Calculate label position (avoid top edge)
            int labelY = std::max(box.y, labelSize.height + 2);
            cv::Point textOrg(box.x, labelY - 2);

            // Draw label background
            cv::rectangle(image,
                          cv::Point(box.x, labelY - labelSize.height - 2),
                          cv::Point(box.x + labelSize.width + 5, labelY + baseline),
                          color, cv::FILLED);

            // Draw text
            cv::putText(image, label, textOrg,
                        cv::FONT_HERSHEY_SIMPLEX, fontScale,
                        cv::Scalar(255, 255, 255), textThickness, cv::LINE_AA);
        }
    }

    // Apply mask layer if created
    if (drawMasks && !maskLayer.empty()) {
        cv::addWeighted(maskLayer, maskAlpha, image, 1.0, 0, image);
    }
}

void Utils::drawPoseEstimation(cv::Mat &image, const std::vector<Prediction> &predictions, const std::vector<std::pair<int, int> > &poseSkeleton, bool bbox, const std::vector<cv::Scalar> posePalette)
{
    if (predictions.empty())
        return;

    // Calculate dynamic sizes based on image dimensions
    const float scale_factor = std::min(image.rows, image.cols) / 1280.0f;  // Reference 1280px size

    // Dynamic sizing parameters
    const int line_thickness = std::max(2, static_cast<int>(3 * scale_factor));
    const int kpt_radius = std::max(3, static_cast<int>(5 * scale_factor));
    // const float font_scale = 0.5f * scale_factor;
    // const int text_thickness = std::max(1, static_cast<int>(1 * scale_factor));
    // const int text_offset = static_cast<int>(10 * scale_factor);

    // Define per-keypoint color indices (for keypoints 0 to 16)
    static const std::vector<int> kpt_color_indices = {16,16,16,16,16,0,0,0,0,0,0,9,9,9,9,9,9};
    // Define per-limb color indices for each skeleton connection.
    // Make sure the number of entries here matches the number of pairs in POSE_SKELETON.
    static const std::vector<int> limb_color_indices = {9,9,9,9,7,7,7,0,0,0,0,0,16,16,16,16,16,16,16};

    // Loop through each prediction
    for (const auto& prediction : predictions) {
        // Draw bounding box (optional)
        if (bbox) {
            cv::rectangle(image, prediction.box, cv::Scalar(0, 255, 0), line_thickness);
        }

        // Prepare a vector to hold keypoint positions and validity flags.
        const size_t num_kpts = prediction.points.size();
        std::vector<cv::Point> kpt_points(num_kpts, cv::Point(-1, -1));
        std::vector<bool> valid(num_kpts, false);

        // Draw keypoints using the corresponding palette colors
        for (size_t i = 0; i < num_kpts; i++) {
            int x = std::round(prediction.points[i].x);
            int y = std::round(prediction.points[i].y);
            kpt_points[i] = cv::Point(x, y);
            valid[i] = true;
            int color_index = (i < kpt_color_indices.size()) ? kpt_color_indices[i] : 0;
            cv::circle(image, cv::Point(x, y), kpt_radius, posePalette[color_index], -1, cv::LINE_AA);
        }

        // Draw skeleton connections based on a predefined POSE_SKELETON (vector of pairs)
        // Make sure that POSE_SKELETON is defined with 0-indexed keypoint indices.
        for (size_t j = 0; j < poseSkeleton.size(); j++) {
            auto [src, dst] = poseSkeleton[j];
            if (src < num_kpts && dst < num_kpts && valid[src] && valid[dst]) {
                // Use the corresponding limb color from the palette
                int limb_color_index = (j < limb_color_indices.size()) ? limb_color_indices[j] : 0;
                cv::line(image, kpt_points[src], kpt_points[dst],
                         posePalette[limb_color_index],
                         line_thickness, cv::LINE_AA);
            }
        }

        // (Optional) Add text labels such as confidence scores here if desired.
    }
}

void Utils::drawPoseEstimation(cv::Mat &image, const std::vector<Prediction> &predictions, const std::vector<std::string> &classNames, const std::vector<cv::Scalar> &classColors, const std::vector<std::pair<int, int> > &poseSkeleton, bool drawBox, bool drawLabels, bool drawKeypoints, bool drawSkeleton, const std::vector<cv::Scalar> &posePalette)
{
    if (predictions.empty()) return;

    // Calculate dynamic sizes based on image dimensions
    const float scale_factor = std::min(image.rows, image.cols) / 1280.0f;
    const int line_thickness = std::max(1, static_cast<int>(2 * scale_factor));
    const int kpt_radius = std::max(2, static_cast<int>(4 * scale_factor));
    const double font_scale = 0.6 * scale_factor;
    const int text_thickness = std::max(1, static_cast<int>(1 * scale_factor));

    // Per-keypoint color indices
    static const std::vector<int> kpt_color_indices = {
        16, 16, 16, 16, 16, 0, 0, 0, 0, 0, 0, 9, 9, 9, 9, 9, 9
    };

    // Per-limb color indices
    static const std::vector<int> limb_color_indices = {
        9, 9, 9, 9, 7, 7, 7, 0, 0, 0, 0, 0, 16, 16, 16, 16, 16, 16, 16
    };

    for (const auto& pred : predictions) {
        // Validate class ID
        const bool validClass = pred.classId >= 0 &&
                                static_cast<size_t>(pred.classId) < classNames.size();

        const cv::Scalar& boxColor = validClass ?
                                         classColors[pred.classId % classColors.size()] : cv::Scalar(0, 255, 0);

        // -----------------------------
        // Draw bounding box (optional)
        // -----------------------------
        if (drawBox) {
            cv::rectangle(image, pred.box, boxColor, line_thickness);
        }

        // -----------------------------
        // Prepare keypoint data
        // -----------------------------
        const size_t num_kpts = pred.points.size();
        std::vector<cv::Point> kpt_points;
        std::vector<bool> valid_kpt;

        kpt_points.reserve(num_kpts);
        valid_kpt.reserve(num_kpts);

        for (const auto& pt : pred.points) {
            const bool valid = pt.x >= 0 && pt.y >= 0;
            kpt_points.emplace_back(std::round(pt.x), std::round(pt.y));
            valid_kpt.push_back(valid);
        }

        // -----------------------------
        // Draw skeleton connections
        // -----------------------------
        if (drawSkeleton) {
            for (size_t j = 0; j < poseSkeleton.size(); j++) {
                const auto& [src_idx, dst_idx] = poseSkeleton[j];

                if (src_idx < num_kpts && dst_idx < num_kpts &&
                    valid_kpt[src_idx] && valid_kpt[dst_idx]) {

                    const int color_idx = (j < limb_color_indices.size()) ?
                                              limb_color_indices[j] : 0;

                    cv::line(image, kpt_points[src_idx], kpt_points[dst_idx],
                             posePalette[color_idx % posePalette.size()],
                             line_thickness, cv::LINE_AA);
                }
            }
        }

        // -----------------------------
        // Draw keypoints
        // -----------------------------
        if (drawKeypoints) {
            for (size_t i = 0; i < num_kpts; i++) {
                if (!valid_kpt[i]) continue;

                const int color_idx = (i < kpt_color_indices.size()) ?
                                          kpt_color_indices[i] : 0;

                cv::circle(image, kpt_points[i], kpt_radius,
                           posePalette[color_idx % posePalette.size()],
                           -1, cv::LINE_AA);
            }
        }

        // -----------------------------
        // Draw label (optional)
        // -----------------------------
        if (drawLabels && validClass && !pred.box.empty()) {
            std::string label;
            if (pred.trackerId >= 0) {
                label = std::format("{}:{} ({}%)",
                                    classNames[pred.classId],
                                    pred.trackerId,
                                    static_cast<int>(pred.conf * 100));
            } else {
                label = std::format("{} ({}%)",
                                    classNames[pred.classId],
                                    static_cast<int>(pred.conf * 100));
            }

            // Calculate text position (top center above box)
            int baseline = 0;
            cv::Size labelSize = cv::getTextSize(label, cv::FONT_HERSHEY_SIMPLEX,
                                                 font_scale, text_thickness, &baseline);

            const int textX = pred.box.x + (pred.box.width - labelSize.width) / 2;
            const int textY = std::max(pred.box.y - 5, labelSize.height + 2);

            // Draw background
            cv::rectangle(image,
                          cv::Point(textX - 2, textY - labelSize.height - 2),
                          cv::Point(textX + labelSize.width + 2, textY + baseline),
                          boxColor, cv::FILLED);

            // Draw text
            cv::putText(image, label, cv::Point(textX, textY - 2),
                        cv::FONT_HERSHEY_SIMPLEX, font_scale,
                        cv::Scalar(255, 255, 255), text_thickness, cv::LINE_AA);
        }
    }
}

void Utils::crop(const cv::Mat &img, cv::Mat &res, const cv::Rect &box) {
    int x1 = box.x;
    int y1 = box.y;
    int x2 = box.x + box.width;
    int y2 = box.y + box.height;
    res = img(cv::Range(y1, y2), cv::Range(x1, x2)).clone();
}

void Utils::perspectiveCrop(const cv::Mat &img, cv::Mat &res, const std::vector<cv::Point2f> &srcPoints, const std::vector<cv::Point2f> &dstPoints) {
    int h = dstPoints.at(3).y - dstPoints.at(0).y;
    int w = dstPoints.at(1).x - dstPoints.at(0).x;

    cv::Mat tr_mat = cv::getPerspectiveTransform(srcPoints, dstPoints);
    cv::warpPerspective(img, res, tr_mat,  cv::Size(w, h));
}

void Utils::perspectiveCrop(const cv::Mat &img, cv::Mat &res, const std::vector<cv::Point2f> &srcPoints, float sizeGain) {

    // Find heights/widths on each side
    float h_left  = cv::norm(srcPoints[3] - srcPoints[0]);
    float h_right = cv::norm(srcPoints[2] - srcPoints[1]);
    float w_top   = cv::norm(srcPoints[1] - srcPoints[0]);
    float w_bot   = cv::norm(srcPoints[2] - srcPoints[3]);

    // Expected height/width
    float exp_h = std::max(h_left, h_right) / sizeGain;
    float exp_w = std::max(w_top, w_bot) / sizeGain;

    const std::vector<cv::Point2f> dst_points = {
        {0.0f,      0.0f},
        {exp_w,     0.0f},
        {exp_w,     exp_h},
        {0.0f,      exp_h}
    };
    perspectiveCrop(img, res, srcPoints, dst_points);
}

void Utils::perspectiveCrop(const cv::Mat &img, cv::Mat &res, const std::vector<KeyPoint> &srcPoints, const std::vector<KeyPoint> &dstPoints) {
    perspectiveCrop(img, res, KeyPoint::toPoints(srcPoints), KeyPoint::toPoints(dstPoints));
}

void Utils::perspectiveCrop(const cv::Mat &img, cv::Mat &res, const std::vector<KeyPoint> &srcPoints) {
    perspectiveCrop(img, res, KeyPoint::toPoints(srcPoints));
}
