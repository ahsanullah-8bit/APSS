import cv2
import numpy as np
import os
import glob
from tqdm import tqdm

def order_points(pts):
    """
    Orders a list of 4 points to be top-left, top-right, bottom-right, bottom-left.
    This is crucial for perspective transformation.
    """
    rect = np.zeros((4, 2), dtype="float32")
    s = pts.sum(axis=1)
    rect[0] = pts[np.argmin(s)] # Top-left
    rect[2] = pts[np.argmax(s)] # Bottom-right
    diff = np.diff(pts, axis=1)
    rect[1] = pts[np.argmin(diff)] # Top-right
    rect[3] = pts[np.argmax(diff)] # Bottom-left
    return rect

def four_point_transform(image, pts, max_width, max_height, gain):
    """
    Applies a four-point perspective transform to crop and unwarp an image, without exceeding the
    max_width and max_height. The width and height are extracted from the detection points.
    Args:
        image (np.array): The input image.
        pts (np.array): A 4x2 numpy array of the (x, y) coordinates of the
                        four corners of the region to transform, in the order
                        top-left, top-right, bottom-right, bottom-left.
        max_width (int): The max width of the output unwarped image.
        max_height (int): The max height of the output unwarped image.
        gain (float): Extra enlargement needed for the image. -1 for dynamic or achieving max gain.
    Returns:
        np.array: The unwarped and cropped image.
    """
    rect = order_points(pts)
    (tl, tr, br, bl) = rect
    
    hl = bl[1] - tl[1] # ybl - ytl
    hr = br[1] - tr[1] # ybr - ytr
    wt = tr[0] - tl[0] # xtr - xtl
    wb = br[0] - bl[0] # xbr - xbl
    fh = max(hl, hr)
    fw = max(wt, wb)

    if (gain == -1):
        gain_w = fw / max_width if max_width > 0 else 0
        gain_h = fh / max_height if max_height > 0 else 0
        
        if gain_w == 0 and gain_h == 0:
            gain = 1.0
        elif gain_w == 0:
            gain = gain_h
        elif gain_h == 0:
            gain = gain_w
        else:
            gain = max(gain_w, gain_h)

    output_height = int(fh / gain)
    output_width = int(fw / gain)
    
    output_width = max(1, output_width)
    output_height = max(1, output_height)

    dst = np.array([
        [0, 0],
        [output_width - 1, 0],
        [output_width - 1, output_height - 1],
        [0, output_height - 1]], dtype="float32")

    trans_mat = cv2.getPerspectiveTransform(rect, dst)
    warped = cv2.warpPerspective(image, trans_mat, dsize=(output_width, output_height))

    return warped

def process_yolo_polygon_dataset(
    images_dir,
    labels_dir,
    output_dir,
    target_class_id, # The class ID of the objects you want to crop (e.g., license plates)
    num_polygon_points=4, # Number of (x,y) pairs expected for the polygon (e.g., 4 for a quadrilateral)
    output_plate_width=-1,
    output_plate_height=-1,
    size_gain=0.2
):
    """
    Reads a YOLO-formatted dataset (images and polygon labels), crops objects
    based on their polygon points, and saves the cropped images.

    Args:
        images_dir (str): Path to the directory containing images.
        labels_dir (str): Path to the directory containing YOLO .txt label files.
        output_dir (str): Directory to save the cropped images.
        target_class_id (int): The class ID of the objects you want to crop.
        num_polygon_points (int): The number of (x,y) points defining the polygon in the label file.
                                  For a 4-sided polygon, this should be 4.
        output_plate_width (int): Desired width of the output unwarped image. -1 for dynamic.
        output_plate_height (int): Desired height of the output unwarped image. -1 for dynamic.
        size_gain (float): Extra enlargement needed for the image, used if output_plate_width/height are -1.
                           -1 for dynamic gain calculation to fit max_width/height.
    """
    os.makedirs(output_dir, exist_ok=True)

    image_files = []
    for ext in ['jpg', 'jpeg', 'png', 'bmp', 'tiff']:
        image_files.extend(glob.glob(os.path.join(images_dir, '**', f'*.{ext}'), recursive=True))
    
    if not image_files:
        print(f"🚫 No image files found in directory: {images_dir} or its subdirectories.")
        return

    for img_path in tqdm(image_files, desc="🔃 Processing Dataset"):
        img_basename = os.path.basename(img_path)
        img_name_without_ext = os.path.splitext(img_basename)[0]
        
        # Construct the corresponding label file path
        # Assuming labels are in a parallel structure (e.g., images -> labels)
        relative_path_to_img = os.path.relpath(img_path, images_dir)
        label_file_path = os.path.join(labels_dir, os.path.dirname(relative_path_to_img), f"{img_name_without_ext}.txt")

        if not os.path.exists(label_file_path):
            tqdm.write(f"⚠️ Warning: No label file found for {img_path}. Skipping. (Expected: {label_file_path})")
            continue

        img = cv2.imread(img_path)
        if img is None:
            tqdm.write(f"⚠️ Warning: Could not read image {img_path}. Skipping.")
            continue

        h, w, _ = img.shape
        
        plate_count = 0
        with open(label_file_path, 'r') as f:
            for line_idx, line in enumerate(f):
                parts = line.strip().split()
                
                # Expected format: class_id poly_x1 poly_y1 poly_x2 poly_y2 ...
                expected_parts_count = 1 + (num_polygon_points * 2) # class_id + N*2 coords
                
                if len(parts) != expected_parts_count:
                    tqdm.write(f"⚠️ Warning: Malformed label line in {label_file_path} (line {line_idx+1}). Expected {expected_parts_count} parts, got {len(parts)}. Skipping this annotation.")
                    continue
                
                try:
                    class_id = int(parts[0])
                    
                    if class_id == target_class_id:
                        # Extract polygon coordinates (normalized)
                        polygon_coords_normalized = np.array(list(map(float, parts[1:]))).reshape(-1, 2)

                        if polygon_coords_normalized.shape[0] != num_polygon_points:
                            tqdm.write(f"⚠️ Warning: Polygon in {label_file_path} (line {line_idx+1}) does not have {num_polygon_points} points. Skipping.")
                            continue

                        # Denormalize keypoints to pixel coordinates
                        polygon_coords_abs = polygon_coords_normalized * np.array([w, h])

                        # Perform perspective transform
                        try:
                            warped_object = four_point_transform(
                                img, 
                                polygon_coords_abs, 
                                max_width=output_plate_width, 
                                max_height=output_plate_height, 
                                gain=size_gain
                            )
                            
                            # Construct output path, mirroring input structure
                            relative_output_path = os.path.join(os.path.dirname(relative_path_to_img), f"{img_name_without_ext}_obj{plate_count}.png")
                            output_filepath = os.path.join(output_dir, relative_output_path)
                            os.makedirs(os.path.dirname(output_filepath), exist_ok=True) # Ensure output subdirs exist
                            
                            cv2.imwrite(output_filepath, warped_object)
                            # tqdm.write(f"  ✅ Saved cropped object: {output_filepath}")
                            plate_count += 1
                        except Exception as e:
                            tqdm.write(f"❌ Error during perspective transform for object in {img_path} (line {line_idx+1}), coords {polygon_coords_abs}: {e}")
                except ValueError:
                    tqdm.write(f"⚠️ Warning: Invalid numeric values in label line {line_idx+1} of {label_file_path}. Skipping this annotation.")
                    continue
        
        if plate_count == 0:
            tqdm.write(f"🗑️ No target objects (class ID {target_class_id}) found or processed in {img_path}.")

if __name__ == "__main__":
    # --- Configuration ---
    DATASET_BASE_DIR = 'License_Plates'
    IMAGES_SUBDIR = 'train/images'
    LABELS_SUBDIR = 'train/labels'
    OUTPUT_CROPS_DIR = 'cropped_objects_from_dataset'

    TARGET_OBJECT_CLASS_ID = 0
    NUM_POLYGON_POINTS = 4
    DESIRED_CROP_WIDTH = 640
    DESIRED_CROP_HEIGHT = 640
    CROP_SIZE_GAIN = -1

    full_images_dir = os.path.join(DATASET_BASE_DIR, IMAGES_SUBDIR)
    full_labels_dir = os.path.join(DATASET_BASE_DIR, LABELS_SUBDIR)
    full_output_dir = os.path.join(DATASET_BASE_DIR, OUTPUT_CROPS_DIR)


    # --- Run the processing ---
    print(f"Starting processing for images in: {full_images_dir}")
    print(f"Reading labels from: {full_labels_dir}")
    print(f"Saving crops to: {full_output_dir}")

    process_yolo_polygon_dataset(
        images_dir=full_images_dir,
        labels_dir=full_labels_dir,
        output_dir=full_output_dir,
        target_class_id=TARGET_OBJECT_CLASS_ID,
        num_polygon_points=NUM_POLYGON_POINTS,
        output_plate_width=DESIRED_CROP_WIDTH,
        output_plate_height=DESIRED_CROP_HEIGHT,
        size_gain=CROP_SIZE_GAIN
    )

    print(f"\n✅ Processing complete. Cropped objects saved to: {full_output_dir}")
