# THIS ONE IS NOT TESTED.
# Script for converting a Polygon annotated dataset to Oriented Bounding Box dataset.
# Usage: Change the source directory and give it a go. It expects the dataset to only have `images` and `labels` directory
#           containing the images and labels. No fancy things.
# WARNING: This script is only designed to convert 4-point polygons (if more, ignored). It seta visibility to 1 for all.
#           It is specific to our License Plate dataset.

import os
import numpy as np
from tqdm import tqdm
import cv2
import shutil

SOURCE_DIR = 'License_Plates'
DATASET_DIR = 'License_Plates_obb'
LABELS_DIR = f"{DATASET_DIR}/labels"
OUTPUT_LABELS_DIR = f'{DATASET_DIR}/labels' # f"{DATASET_DIR}/obb_labels"

try:
    # Copy the source dataset
    os.mkdir(DATASET_DIR)
    shutil.copytree(SOURCE_DIR, DATASET_DIR, dirs_exist_ok=True)
    # Rename the source labels directory
    os.rename(LABELS_DIR, f'{LABELS_DIR}_pre')
    LABELS_DIR = f'{LABELS_DIR}_pre'
    # Create destination labels directory
    os.makedirs(OUTPUT_LABELS_DIR, exist_ok=True)
    print(f"Folder '{SOURCE_DIR}' copied successfully to '{DATASET_DIR}'")
except FileNotFoundError:
    print(f"Error: Folder '{SOURCE_DIR}' not found.")
except Exception as e:
    print(f"An error occurred: {e}")
    

def convert_polygon_to_obb_cv(polygon):
    # polygon shape: (4, 2)
    polygon = np.array(polygon, dtype=np.float32)
    rect = cv2.minAreaRect(polygon)  # ((x_center, y_center), (w, h), angle)
    (x_center, y_center), (w, h), angle = rect

    # Normalize angle: OpenCV can return angle in [-90, 0), we normalize it to radians
    angle_rad = np.deg2rad(angle)

    return x_center, y_center, w, h, angle_rad

def convert_yolo_poly_to_obb(label_path, output_path):
    with open(label_path, 'r') as f:
        lines = f.readlines()

    new_lines = []

    for line in lines:
        parts = line.strip().split()
        if len(parts) != 9:
            print(f"Skipping malformed line in {label_path}: {line}")
            continue

        class_id = parts[0]
        coords = [float(p) for p in parts[1:]]
        coords = np.array(coords, dtype=np.float32).reshape(4, 2)  # (x, y)

        x_c, y_c, w, h, theta = convert_polygon_to_obb_cv(coords)

        obb_line = f"{class_id} {x_c:.6f} {y_c:.6f} {w:.6f} {h:.6f} {theta:.6f}"
        new_lines.append(obb_line)

    with open(output_path, 'w') as f:
        f.write("\n".join(new_lines))

def batch_convert_labels():
    label_files = [f for f in os.listdir(LABELS_DIR) if f.endswith('.txt')]

    for file in tqdm(label_files, desc="Converting polygon labels to OBB"):
        input_path = os.path.join(LABELS_DIR, file)
        output_path = os.path.join(OUTPUT_LABELS_DIR, file)
        convert_yolo_poly_to_obb(input_path, output_path)


######################################
####            MAIN
######################################


if __name__ == "__main__":
    batch_convert_labels()
    print(f"\n✅ Done. Converted labels saved to: {OUTPUT_LABELS_DIR}")
