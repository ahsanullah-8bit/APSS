import cv2
import numpy as np
from ultralytics import YOLO
import os
import glob
from tqdm import tqdm

def process_images_with_model(
    model_path,
    image_input_dir,
    output_annotated_images_dir,
    output_labels_dir,
    confidence_threshold=0.4,
    line_thickness=2,
    font_scale=0.6,
    font_thickness=1
):
    try:
        model = YOLO(model_path, task='detect')
        print(f"YOLO detection model loaded from: {model_path}")
    except Exception as e:
        print(f"❌ Error loading YOLO model from '{model_path}': {e}")
        print("Please ensure the model path is correct and the model file is valid.")
        return

    os.makedirs(output_annotated_images_dir, exist_ok=True)
    os.makedirs(output_labels_dir, exist_ok=True)

    image_files = []
    for ext in ['jpg', 'jpeg', 'png', 'bmp', 'tiff']:
        image_files.extend(glob.glob(os.path.join(image_input_dir, f'*.{ext}')))

    print(f"Found {len(image_files)} image files in '{image_input_dir}'.")

    if not image_files:
        print(f"🚫 No image files found in directory: {image_input_dir}. Please check your 'IMAGE_INPUT_DIR' setting and ensure images are directly in it.")
        return

    class_names = model.names
    if not class_names:
        print("⚠️ Warning: Could not retrieve class names from the model. Labels will show class IDs.")

    for img_path in tqdm(image_files, desc="🔃 Processing Images"):
        img = cv2.imread(img_path)
        if img is None:
            tqdm.write(f"⚠️ Warning: Could not read image {img_path}. Skipping.")
            continue

        img_height, img_width, _ = img.shape

        results = model.predict(img, conf=confidence_threshold, verbose=False, batch=1)

        # Modified: base_name is just the filename without extension, as no subdirs are expected
        base_name = os.path.splitext(os.path.basename(img_path))[0]

        label_filepath = os.path.join(output_labels_dir, base_name + '.txt')
        # No need for os.makedirs(os.path.dirname(label_filepath), ...) as output_labels_dir is the final parent
        # os.makedirs(output_labels_dir, exist_ok=True) is already called at the start

        all_labels = []

        if os.path.exists(label_filepath):
            with open(label_filepath, 'r') as f:
                existing_labels = f.readlines()
                all_labels.extend([line.strip() for line in existing_labels if line.strip()])

        for r in results:
            for box in r.boxes:
                x1, y1, x2, y2 = box.xyxy[0].cpu().numpy()
                class_id = int(box.cls)

                x_center = ((x1 + x2) / 2.0) / img_width
                y_center = ((y1 + y2) / 2.0) / img_height
                box_width = (x2 - x1) / img_width
                box_height = (y2 - y1) / img_height

                yolo_label = f"{class_id} {x_center:.6f} {y_center:.6f} {box_width:.6f} {box_height:.6f}"
                all_labels.append(yolo_label)

        with open(label_filepath, 'w') as f:
            for label_line in all_labels:
                f.write(label_line + '\n')

        annotated_img = img.copy()
        for r in results:
            for box in r.boxes:
                x1, y1, x2, y2 = map(int, box.xyxy[0].cpu().numpy())
                conf = float(box.conf)
                class_id = int(box.cls)

                label = class_names[class_id] if class_names and class_id < len(class_names) else f"Class {class_id}"
                display_text = f"{label} {conf:.2f}"

                color = (0, 255, 0)

                cv2.rectangle(annotated_img, (x1, y1), (x2, y2), color, line_thickness)

                (text_width, text_height), baseline = cv2.getTextSize(display_text, cv2.FONT_HERSHEY_SIMPLEX, font_scale, font_thickness)
                cv2.rectangle(annotated_img, (x1, y1 - text_height - baseline), (x1 + text_width, y1), color, -1)
                cv2.putText(annotated_img, display_text, (x1, y1 - baseline), cv2.FONT_HERSHEY_SIMPLEX, font_scale, (0, 0, 0), font_thickness, cv2.LINE_AA)

        # Modified: output_img_filepath is directly in output_annotated_images_dir
        output_img_filepath = os.path.join(output_annotated_images_dir, os.path.basename(img_path))
        # os.makedirs(output_annotated_images_dir, exist_ok=True) is already called at the start
        cv2.imwrite(output_img_filepath, annotated_img)

    tqdm.write(f"\n✅ Processing complete.")
    tqdm.write(f"Annotated images saved to: {output_annotated_images_dir}")
    tqdm.write(f"YOLO labels saved/updated in: {output_labels_dir}")


if __name__ == "__main__":
    YOLO_MODEL_PATH = 'yolo11m_int8_openvino_model'

    DATASET_ROOT = 'License_Plates_bb'

    IMAGE_INPUT_DIR = os.path.join(DATASET_ROOT, 'images')

    OUTPUT_ANNOTATED_DIR = os.path.join(DATASET_ROOT, 'annotated_images_for_review')

    OUTPUT_LABELS_DIR = os.path.join(DATASET_ROOT, 'labels')

    DETECTION_CONF_THRESHOLD = 0.5

    process_images_with_model(
        model_path=YOLO_MODEL_PATH,
        image_input_dir=IMAGE_INPUT_DIR,
        output_annotated_images_dir=OUTPUT_ANNOTATED_DIR,
        output_labels_dir=OUTPUT_LABELS_DIR,
        confidence_threshold=DETECTION_CONF_THRESHOLD
    )
