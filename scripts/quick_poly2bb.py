# Script for converting a Polygon annotated dataset to Bounding Box dataset.
# Usage: Change the source directory and give it a go. It expects the dataset to only have `images` and `labels` directory
#           containing the images and labels. No fancy things.
# WARNING: This script is only designed to convert 4-point polygons (if more, ignored). It is specific to our License Plate dataset.

import os
import numpy as np
from tqdm import tqdm
import cv2
import shutil

# Directory setup
SOURCE_DIR = 'License_Plates'
DATASET_DIR = 'License_Plates_bb'
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


def convert_polygon_to_bb_cv(coords):
    """Converts a 4-sided polygon (numpy array of shape (4, 2)) to a bounding box (x_center, y_center, width, height)."""
    x_coords = coords[:, 0]
    y_coords = coords[:, 1]

    x_min = np.min(x_coords)
    y_min = np.min(y_coords)
    x_max = np.max(x_coords)
    y_max = np.max(y_coords)

    width = x_max - x_min
    height = y_max - y_min
    x_center = (x_min + x_max) / 2.0
    y_center = (y_min + y_max) / 2.0

    return x_center, y_center, width, height

def convert_yolo_poly_to_bb(label_path, output_path):
    with open(label_path, 'r') as f:
        lines = f.readlines()

    new_lines = []

    for line in lines:
        parts = line.strip().split()
        if len(parts) > 9:
            print(f"⚠️ Malformed line found in {label_path}: {line}")
            parts = parts[:9]
            print(f'Converted malformed line to {parts}')
        elif len(parts) < 9:
            print(f"⚠️ Skipping malformed line in {label_path}: {line}")

        class_id = parts[0]
        coords = [float(p) for p in parts[1:]]
        coords = np.array(coords, dtype=np.float32).reshape(4, 2)  # (x, y)

        x_c, y_c, w, h = convert_polygon_to_bb_cv(coords)

        bb_line = f"{class_id} {x_c:.6f} {y_c:.6f} {w:.6f} {h:.6f}"
        new_lines.append(bb_line)

    with open(output_path, 'w') as f:
        f.write("\n".join(new_lines))

def batch_convert_labels():
    label_files = [f for f in os.listdir(LABELS_DIR) if f.endswith('.txt')]

    for file in tqdm(label_files, desc="Converting polygon labels to Bounding Boxes"):
        input_path = os.path.join(LABELS_DIR, file)
        output_path = os.path.join(OUTPUT_LABELS_DIR, file)
        convert_yolo_poly_to_bb(input_path, output_path)



######################################
####            MAIN
######################################

if __name__ == "__main__":
    batch_convert_labels()
    # shutil.rmtree(LABELS_DIR)
    print(f'Removed the source labels directory {LABELS_DIR}')
    print(f"\n✅ Done. Converted labels saved to: {OUTPUT_LABELS_DIR}")
