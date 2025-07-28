#pragma once

// #include <algorithm>
#include <numeric>
#include <fstream>
#include <qjsondocument.h>
#include <qjsonobject.h>
#include <random>
#include <vector>

#include <onnxruntime_cxx_api.h>
#include <opencv2/opencv.hpp>

#include <QRegularExpression>
#include <QDebug>

#include "apss.h"
#include "utils/prediction.h"
#include "licensed/utility.h"

class Utils {
public:

    /**
     * @brief A robust implementation of a clamp function.
     *        Restricts a value to lie within a specified range [low, high].
     *
     * @tparam T The type of the value to clamp. Should be an arithmetic type (int, float, etc.).
     * @param value The value to clamp.
     * @param low The lower bound of the range.
     * @param high The upper bound of the range.
     * @return const T& The clamped value, constrained to the range [low, high].
     *
     * @note If low > high, the function swaps the bounds automatically to ensure valid behavior.
     */
    template <typename T>
    static typename std::enable_if<std::is_arithmetic<T>::value, T>::type
        clamp(const T &value, const T &low, const T &high)
    {
        // Ensure the range [low, high] is valid; swap if necessary
        T validLow = low < high ? low : high;
        T validHigh = low < high ? high : low;

        // Clamp the value to the range [validLow, validHigh]
        if (value < validLow)
            return validLow;
        if (value > validHigh)
            return validHigh;
        return value;
    }

    static cv::Mat sigmoid(const cv::Mat &src);

    /**
     * @brief Loads class names from a given file path.
     *
     * @param path Path to the file containing class names.
     * @return std::vector<std::string> Vector of class names.
     */
    static std::vector<std::string> readClassNames(const std::string &path);

    static std::vector<std::string> jsonToVecClassNames(const std::string &namesStd);

    /**
     * @brief Computes the product of elements in a vector.
     *
     * @param vector Vector of integers.
     * @return size_t Product of all elements.
     */
    static size_t vectorProduct(const std::vector<int64_t> &vector);

    /**
     * @brief Resizes an image with letterboxing to maintain aspect ratio.
     *
     * @param image Input image.
     * @param outImage Output resized and padded image.
     * @param newShape Desired output size.
     * @param color Padding color (default is gray).
     * @param scale Whether to allow scaling of the image.
     */
    static void letterBox(const cv::Mat& image, cv::Mat& outImage,
                          const cv::Size& newShape,
                          const cv::Scalar& color = cv::Scalar(114, 114, 114),
                          bool scale = true);

    /**
     * @brief Scales detection coordinates back to the original image size.
     *
     * @param resizedImageShape Shape of the resized image used for inference.
     * @param bbox Detection bounding box to be scaled.
     * @param originalImageShape Original image size before resizing.
     * @param p_Clip Whether to clip the coordinates to the image boundaries.
     * @return BoundingBox Scaled bounding box.
     */
    static cv::Rect scaleCoords(const cv::Size &resizedImageShape, cv::Rect coords,
                                const cv::Size &originalImageShape, bool p_Clip = true);

    /**
     * @brief Scales keypoint coordinates back to the original image size.
     *
     * @param resizedImageShape Shape of the resized image used for inference.
     * @param point Keypoint to be scaled.
     * @param originalImageShape Original image size before resizing.
     * @param p_Clip Whether to clip the coordinates to the image boundaries.
     * @return Point2f Scaled keypoint.
     */
    static cv::Point2f scaleCoords(const cv::Size &resizedImageShape, cv::Point2f point,
                                   const cv::Size &originalImageShape, bool p_Clip = true);

    /**
     * @brief Scales keypoint coordinates back to the original image size.
     *
     * @param resizedImageShape Shape of the resized image used for inference.
     * @param points Keypoints to be scaled.
     * @param originalImageShape Original image size before resizing.
     * @param p_Clip Whether to clip the coordinates to the image boundaries.
     * @return Vector of Points, Scaled keypoints.
     */
    static std::vector<cv::Point2f> scaleCoords(const cv::Size &resizedImageShape, const std::vector<cv::Point2f> &points,
                                                const cv::Size &originalImageShape, bool p_Clip = true);

    /**
     * @brief Performs Non-Maximum Suppression (NMS) on the bounding boxes.
     *
     * @param boundingBoxes Vector of bounding boxes.
     * @param scores Vector of confidence scores corresponding to each bounding box.
     * @param scoreThreshold Confidence threshold to filter boxes.
     * @param nmsThreshold IoU threshold for NMS.
     * @param indices Output vector of indices that survive NMS.
     */
    // Optimized Non-Maximum Suppression Function
    static void NMSBoxes(const std::vector<cv::Rect>& boundingBoxes,
                         const std::vector<float>& scores,
                         float scoreThreshold,
                         float nmsThreshold,
                         std::vector<int>& indices);


    /**
     * @brief Generates a vector of colors for each class name.
     *
     * @param classNames Vector of class names.
     * @param seed Seed for random color generation to ensure reproducibility.
     * @return std::vector<cv::Scalar> Vector of colors.
     */
    static std::vector<cv::Scalar> generateColors(const std::vector<std::string> &classNames, int seed = 42);

    /**
     * @brief Draws bounding boxes and labels on the image based on detections.
     *
     * @param image Image on which to draw.
     * @param detections Vector of detections.
     * @param classNames Vector of class names corresponding to object IDs.
     * @param colors Vector of colors for each class.
     */
    static void drawBoundingBox(cv::Mat &image, const std::vector<Prediction> &predictions,
                                const std::vector<std::string> &classNames,
                                const std::vector<cv::Scalar> &colors);

    /**
     * @brief Draws bounding boxes and semi-transparent masks on the image based on detections.
     *
     * @param image Image on which to draw.
     * @param detections Vector of detections.
     * @param classNames Vector of class names corresponding to object IDs.
     * @param classColors Vector of colors for each class.
     * @param maskAlpha Alpha value for the mask transparency.
     */
    static void drawBoundingBoxMask(cv::Mat &image, const std::vector<Prediction> &predictions,
                                    const std::vector<std::string> &classNames,
                                    const std::vector<cv::Scalar> &classColors,
                                    float maskAlpha = 0.3f);

    /**
     * @brief Draws bounding boxes, masks, and labels on the image based on detections
     *
     * @param image Image to draw on (modified in-place)
     * @param predictions Vector of detection predictions
     * @param classNames Class names corresponding to class IDs
     * @param classColors Colors for each class
     * @param drawMask Whether to draw semi-transparent masks
     * @param maskAlpha Transparency level for masks (0-1)
     * @param drawBoxes Whether to draw bounding boxes
     * @param drawLabels Whether to draw class labels
     */
    static void drawDetections(cv::Mat &image,
                               const std::vector<Prediction> &predictions,
                               const std::vector<std::string> &classNames,
                               const std::vector<cv::Scalar> &classColors,
                               bool drawMask = false,
                               float maskAlpha = 0.4f,
                               bool drawBoxes = true,
                               bool drawLabels = true);

    static void drawOCR(cv::Mat &image,
                               const std::vector<PaddleOCR::OCRPredictResult> &predictions,
                               bool drawMask = false,
                               float maskAlpha = 0.4f,
                               bool drawBoxes = true,
                               bool drawLabels = true);

    // static void drawSegmentationsAndBoxes(cv::Mat &image, const std::vector<Prediction> &predictions,
    //                                       const std::vector<std::string> &classNames,
    //                                       const std::vector<cv::Scalar> &classColors,
    //                                       float maskAlpha = 0.3f);

    // static void drawSegmentations(cv::Mat &image, const std::vector<Prediction> &predictions,
    //                               const std::vector<std::string> &classNames,
    //                               const std::vector<cv::Scalar> &classColors,
    //                               float maskAlpha = 0.3f);

    // /**
    //  * @brief Draws segmentation masks, bounding boxes, and labels on the image
    //  *
    //  * @param image Image to draw on (modified in-place)
    //  * @param predictions Vector of detection predictions
    //  * @param classNames Class names corresponding to class IDs
    //  * @param classColors Colors for each class
    //  * @param drawMasks Whether to draw segmentation masks (default true)
    //  * @param drawBoxes Whether to draw bounding boxes (default true)
    //  * @param drawLabels Whether to draw class labels (default true)
    //  * @param maskAlpha Transparency level for masks (0-1, default 0.4)
    //  */
    // static void drawsSegmentations(cv::Mat &image,
    //                                const std::vector<Prediction> &predictions,
    //                                const std::vector<std::string> &classNames,
    //                                const std::vector<cv::Scalar> &classColors,
    //                                bool drawMasks = true,
    //                                float maskAlpha = 0.4f,
    //                                bool drawBoxes = true,
    //                                bool drawLabels = true);

    /**
     * @brief Draws pose estimations including bounding boxes, keypoints, and skeleton
     * @authors Abdalrahman M. Amer, Mohamed Samir
     *
     * @param image Input/output image
     * @param predictions Vector of pose predictions
     * @param poseSkeleton Connections between each keypoint
     * @param posePalette Actual palette of colors in BGR for each keypoint. Default is the one from Ultralytics COCO-pose in BGR.
     *          // Define the Ultralytics pose palette (BGR format)
     *          // Original RGB values: [255,128,0], [255,153,51], [255,178,102], [230,230,0], [255,153,255],
                // [153,204,255], [255,102,255], [255,51,255], [102,178,255], [51,153,255],
                // [255,153,153], [255,102,102], [255,51,51], [153,255,153], [102,255,102],
                // [51,255,51], [0,255,0], [0,0,255], [255,0,0], [255,255,255]
     */
    static void drawPoseEstimation(cv::Mat &image,
                                   const std::vector<Prediction> &predictions,
                                   const std::vector<std::pair<int, int>> &poseSkeleton,
                                   const std::vector<std::string> &classNames,
                                   bool bbox = false,
                                   const std::vector<cv::Scalar> posePalette = {
                                       cv::Scalar(0,128,255),    // 0
                                       cv::Scalar(51,153,255),   // 1
                                       cv::Scalar(102,178,255),  // 2
                                       cv::Scalar(0,230,230),    // 3
                                       cv::Scalar(255,153,255),  // 4
                                       cv::Scalar(255,204,153),  // 5
                                       cv::Scalar(255,102,255),  // 6
                                       cv::Scalar(255,51,255),   // 7
                                       cv::Scalar(255,178,102),  // 8
                                       cv::Scalar(255,153,51),   // 9
                                       cv::Scalar(153,153,255),  // 10
                                       cv::Scalar(102,102,255),  // 11
                                       cv::Scalar(51,51,255),    // 12
                                       cv::Scalar(153,255,153),  // 13
                                       cv::Scalar(102,255,102),  // 14
                                       cv::Scalar(51,255,51),    // 15
                                       cv::Scalar(0,255,0),      // 16
                                       cv::Scalar(255,0,0),      // 17
                                       cv::Scalar(0,0,255),      // 18
                                       cv::Scalar(255,255,255)   // 19
                                   }
                                   );

    /**
     * @brief Draws pose estimations including keypoints, skeleton, and optional bounding boxes
     *
     * @param image Input/output image
     * @param predictions Vector of pose predictions
     * @param classNames Class names for labeling
     * @param classColors Colors for bounding boxes and labels
     * @param poseSkeleton Connections between keypoints
     * @param options Drawing configuration flags
     * @param posePalette Color palette for keypoints and skeleton
     */
    static void drawPoseEstimation(cv::Mat &image,
                                   const std::vector<Prediction> &predictions,
                                   const std::vector<std::string> &classNames,
                                   const std::vector<std::pair<int, int>> &poseSkeleton,
                                   bool drawBox = false,
                                   bool drawLabels = false,
                                   bool drawKeypoints = true,
                                   bool drawSkeleton = true,
                                   const std::vector<cv::Scalar> &posePalette = {
                                       {0,128,255}, {51,153,255}, {102,178,255}, {0,230,230}, {255,153,255},
                                       {255,204,153}, {255,102,255}, {255,51,255}, {255,178,102}, {255,153,51},
                                       {153,153,255}, {102,102,255}, {51,51,255}, {153,255,153}, {102,255,102},
                                       {51,255,51}, {0,255,0}, {255,0,0}, {0,0,255}, {255,255,255}
                                   });

    /**
     * @brief Crops an image from the source points to the destination points.
     *
     * This function creates an independent copy of the crop.
     *
     * @param img Image to crop from.
     * @param res Image to write to, resultant image.
     * @param srcPoints Vector of 4 points to crop.
     * @param dstPoints Vector of 4 points to crop to, meaning just the resulting coordinates of the crop.
     */
    static void crop(const cv::Mat &img, cv::Mat &res, const cv::Rect &box);

    /**
     * @brief Crops a warped image from the source points to the destination points.
     *
     * @param img Image to crop from.
     * @param res Image to write to, resultant image.
     * @param srcPoints Vector of 4 points to crop.
     * @param dstPoints Vector of 4 points to crop to, meaning just the resulting coordinates of the crop.
     */
    static void perspectiveCrop(const cv::Mat &img, cv::Mat &res,
                                const std::vector<cv::Point2f> &srcPoints,
                                const std::vector<cv::Point2f> &dstPoints);

    /**
     * @brief Crops a warped image from the source points to the destination points calculated from the srcPoints, by taking the longest height and width.
     *
     * @param img Image to crop from.
     * @param res Image to write to, resultant image.
     * @param srcPoints Vector of 4 points to crop.
     */
    static void perspectiveCrop(const cv::Mat &img, cv::Mat &res,
                                const std::vector<cv::Point2f> &srcPoints,
                                float sizeGain = 0.4f);

    /**
     * @brief Crops a warped image from the source points to the destination points.
     *
     * @param img Image to crop from.
     * @param res Image to write to, resultant image.
     * @param srcPoints Vector of 4 points to crop.
     * @param dstPoints Vector of 4 points to crop to, meaning just the resulting coordinates of the crop.
     */
    static void perspectiveCrop(const cv::Mat &img, cv::Mat &res,
                                const std::vector<cv::Point3f> &srcPoints,
                                const std::vector<cv::Point2f> &dstPoints);

    /**
     * @brief Crops a warped image from the source points to the destination points calculated from the srcPoints, by taking the longest height and width.
     *
     * @param img Image to crop from.
     * @param res Image to write to, resultant image.
     * @param srcPoints Vector of 4 points to crop.
     */
    static void perspectiveCrop(const cv::Mat &img, cv::Mat &res,
                                const std::vector<cv::Point3f> &srcPoints);
};
