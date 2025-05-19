# Script for converting a Polygon annotated dataset to Pose/Keypoints dataset.
# Usage: Change the source directory and give it a go. It expects the dataset to only have `images` and `labels` directory
#           containing the images and labels. No fancy things.
# WARNING: This script is only designed to convert 4-point polygons (if more, ignored). It sets visibility to 1 for all.
#           It is specific to our License Plate dataset.

import os
import numpy as np
from tqdm import tqdm
import cv2
import shutil
import yaml

# Directory setup
SOURCE_DIR = 'License_Plates'
DATASET_DIR = 'License_Plates_keypoints'
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
    """Converts a 4-sided polygon (numpy array of shape (4, 2)) to bounding box (x_center, y_center, width, height)."""
    
    # Bounding Box
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

def convert_yolo_poly_to_keypoints(label_path, output_path):
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

        # Keypoints
        # Just straight up coords
        # Since we're going from polygons to keypoints. All the points are obviously visible.
        # Maybe specific to us, but we only labeled plates that are clear and visible
        keypoint_parts = [f'{coords[i][0]} {coords[i][1]} 1' for i in range(len(coords))]
        keypoint_line = f"{class_id} {x_c:.6f} {y_c:.6f} {w:.6f} {h:.6f} {' '.join(keypoint_parts)}"
        new_lines.append(keypoint_line)

    with open(output_path, 'w') as f:
        f.write("\n".join(new_lines))

def batch_convert_labels():
    label_files = [f for f in os.listdir(LABELS_DIR) if f.endswith('.txt')]

    for file in tqdm(label_files, desc="Converting polygon labels to Keypoints"):
        input_path = os.path.join(LABELS_DIR, file)
        output_path = os.path.join(OUTPUT_LABELS_DIR, file)
        convert_yolo_poly_to_keypoints(input_path, output_path)


######################################
####            MAIN
######################################

if __name__ == "__main__":
    batch_convert_labels()
    shutil.rmtree(LABELS_DIR)
    print(f'✅ Removed the source labels directory {LABELS_DIR}')
    print(f"\n✅ Done. Converted labels saved to: {OUTPUT_LABELS_DIR}")
    
    # NOTE: This is the most important part
    # Load the original data.yaml
    # Note: This might mis-order/reorder your yaml file
    src_data_yaml = os.path.join(DATASET_DIR, 'data.yaml')
    with open(src_data_yaml, 'r') as f:
        original_yaml = yaml.safe_load(f)

    # This defines how the keypoint shape is organized. 4 points, 3 values (x, y, vis) in this case
    original_yaml['kpt_shape'] = [4, 3]
    
    # Write the data.yaml
    with open(os.path.join(DATASET_DIR, 'data.yaml'), 'w') as f:
        yaml.dump(original_yaml, f, sort_keys=False)
    print(f'Added kpt_shape {original_yaml['kpt_shape']} to .yaml')
