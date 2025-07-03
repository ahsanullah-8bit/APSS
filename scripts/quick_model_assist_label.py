import cv2
import numpy as np
from ultralytics import YOLO
import os
import glob
from tqdm import tqdm

def annotate_images_with_detections(
    model_path,
    image_input_path,
    output_dir,
    confidence_threshold=0.25, # Default confidence threshold for detections
    line_thickness=2,
    font_scale=0.6,
    font_thickness=1
):
    """
    Loads a YOLO detection model, performs inference, and saves images
    with detected bounding boxes and class labels drawn on them.

    Args:
        model_path (str): Path to your custom-trained YOLO detection model (.pt file).
        image_input_path (str): Path to a single image file or a directory of images.
                                If a directory, it will recursively search for images.
        output_dir (str): Directory to save the annotated images.
        confidence_threshold (float): Minimum confidence score to display a detection.
        line_thickness (int): Thickness of the bounding box lines.
        font_scale (float): Font scale for the class label text.
        font_thickness (int): Font thickness for the class label text.
    """
    # Load the YOLO model
    try:
        model = YOLO(model_path)
        print(f"YOLO detection model loaded from: {model_path}")
    except Exception as e:
        print(f"❌ Error loading YOLO model: {e}")
        return

    # Ensure output directory exists
    os.makedirs(output_dir, exist_ok=True)

    # Get list of image files (recursive search)
    image_files = []
    if os.path.isfile(image_input_path):
        image_files.append(image_input_path)
    elif os.path.isdir(image_input_path):
        for ext in ['jpg', 'jpeg', 'png', 'bmp', 'tiff']:
            image_files.extend(glob.glob(os.path.join(image_input_path, '**', f'*.{ext}'), recursive=True))
        if not image_files:
            print(f"🚫 No image files found in directory: {image_input_path} or its subdirectories.")
            return
    else:
        print(f"❌ Invalid image_input_path: {image_input_path}. Must be a file or directory.")
        return

    # Get class names from the model
    class_names = model.names
    if not class_names:
        print("⚠️ Warning: Could not retrieve class names from the model. Labels will show class IDs.")

    # Process each image with a progress bar
    for img_path in tqdm(image_files, desc="🔃 Annotating Images"):
        img = cv2.imread(img_path)
        if img is None:
            tqdm.write(f"⚠️ Warning: Could not read image {img_path}. Skipping.")
            continue

        # Perform inference
        # Using conf=confidence_threshold to filter detections directly in inference
        results = model.predict(img, conf=confidence_threshold, verbose=False, batch=4)

        # Create a copy of the image to draw on
        annotated_img = img.copy()

        # Iterate through detections for the current image
        for r in results:
            for box in r.boxes:
                x1, y1, x2, y2 = map(int, box.xyxy[0].cpu().numpy())
                conf = float(box.conf)
                class_id = int(box.cls)

                # Get class name or use ID if not available
                label = class_names[class_id] if class_names and class_id < len(class_names) else f"Class {class_id}"
                display_text = f"{label} {conf:.2f}"

                # Define color (you might want to use different colors per class)
                # For simplicity, let's use a fixed color or a simple hash-based color
                color = (0, 255, 0) # Green for bounding boxes

                # Draw bounding box
                cv2.rectangle(annotated_img, (x1, y1), (x2, y2), color, line_thickness)

                # Put text label
                # Calculate text size to draw a background rectangle for better readability
                (text_width, text_height), baseline = cv2.getTextSize(display_text, cv2.FONT_HERSHEY_SIMPLEX, font_scale, font_thickness)
                cv2.rectangle(annotated_img, (x1, y1 - text_height - baseline), (x1 + text_width, y1), color, -1) # Filled rectangle
                cv2.putText(annotated_img, display_text, (x1, y1 - baseline), cv2.FONT_HERSHEY_SIMPLEX, font_scale, (0, 0, 0), font_thickness, cv2.LINE_AA) # Black text

        # Construct output filename, preserving subfolder structure
        relative_path = os.path.relpath(img_path, image_input_path)
        output_sub_dir = os.path.join(output_dir, os.path.dirname(relative_path))
        os.makedirs(output_sub_dir, exist_ok=True) # Ensure subdirectories exist in output

        output_filename = os.path.basename(img_path) # Keep original filename
        output_filepath = os.path.join(output_sub_dir, output_filename)

        cv2.imwrite(output_filepath, annotated_img)
        # tqdm.write(f"  ✅ Annotated and saved: {output_filepath}") # Uncomment for verbose output

    tqdm.write(f"\n✅ Annotation complete. Annotated images saved to: {output_dir}")


if __name__ == "__main__":
    # --- Configuration ---
    # Path to your custom-trained YOLO detection model (.pt file)
    # This should be a detection model (e.g., yolov8n.pt, or your custom-trained detection model)
    YOLO_MODEL_PATH = 'yolo11x_openvino_model' # <--- !!! CHANGE THIS to your actual model path !!!
                                    # You can download pre-trained models from Ultralytics:
                                    # https://docs.ultralytics.com/models/yolov8/#models

    # Path to the input images (can be a single image file or a directory)
    # If a directory, it will recursively search for images in subdirectories.
    IMAGE_INPUT_PATH = 'D:/Projects/FYP/Yolo/License Plates' # <--- !!! CHANGE THIS !!!
                                                                # Example: 'data/raw_images'

    # Directory where annotated images will be saved
    # The output will mirror the input subfolder structure.
    OUTPUT_ANNOTATED_DIR = 'annotated_images_for_review'

    # Confidence threshold for displaying detections (0.0 to 1.0)
    # Detections below this threshold will not be drawn.
    DETECTION_CONF_THRESHOLD = 0.5

    # --- Run the annotation process ---
    annotate_images_with_detections(
        model_path=YOLO_MODEL_PATH,
        image_input_path=IMAGE_INPUT_PATH,
        output_dir=OUTPUT_ANNOTATED_DIR,
        confidence_threshold=DETECTION_CONF_THRESHOLD
    )
