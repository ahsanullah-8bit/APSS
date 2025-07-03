from ultralytics import YOLO
import numpy as np
import math
import glob
import cv2
import os
from tqdm import tqdm

def order_points(pts):
	"""
	Orders a list of 4 points to be top-left, top-right, bottom-right, bottom-left.
	This is crucial for perspective transformation.
	"""
	# Initialize a list of coordinates that will be ordered
	# such that the first entry in the list is the top-left,
	# the second entry is the top-right, the third is the
	# bottom-right, and the fourth is the bottom-left
	rect = np.zeros((4, 2), dtype="float32")

	# The top-left point will have the smallest sum,
	# whereas the bottom-right point will have the largest sum
	s = pts.sum(axis=1)
	rect[0] = pts[np.argmin(s)] # Top-left
	rect[2] = pts[np.argmax(s)] # Bottom-right

	# Now, compute the difference between the points, the
	# top-right point will have the smallest difference,
	# whereas the bottom-left will have the largest difference
	diff = np.diff(pts, axis=1)
	rect[1] = pts[np.argmin(diff)] # Top-right
	rect[3] = pts[np.argmax(diff)] # Bottom-left

	return rect

def four_point_transform(image, pts, output_width=300, output_height=100):
	"""
	Applies a four-point perspective transform to crop and unwarp an image.	
	Args:
		image (np.array): The input image.
		pts (np.array): A 4x2 numpy array of the (x, y) coordinates of the
						four corners of the region to transform, in the order
						top-left, top-right, bottom-right, bottom-left.
		output_width (int): The desired width of the output unwarped image. -1 for dynamic
		output_height (int): The desired height of the output unwarped image. -1 for dynamic	
	Returns:
		np.array: The unwarped and cropped image.
	"""
	# Obtain a consistent order of the points and unpack them individually
	rect = order_points(pts)
	(tl, tr, br, bl) = rect

	# Define the destination points for the unwarped image
	dst = np.array([
		[0, 0],
		[output_width - 1, 0],
		[output_width - 1, output_height - 1],
		[0, output_height - 1]], dtype="float32")

	# Compute the perspective transform matrix and then apply it
	trans_mat = cv2.getPerspectiveTransform(rect, dst)
	warped = cv2.warpPerspective(image, trans_mat, dsize=(output_width, output_height))

	return warped

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
		gain (float): Extra enlargement needed for the image. -1 for dynamic or acheiving max gain.
	Returns:
		np.array: The unwarped and cropped image.
	"""
	# Obtain a consistent order of the points and unpack them individually
	rect = order_points(pts)
	(tl, tr, br, bl) = rect
 
	hl = bl[1] - tl[1] # ybl - ytl
	hr = br[1] - tr[1] # ybr - ytr
	wt = tr[0] - tl[0] # xtr - xtl
	wb = br[0] - bl[0] # xbr - xbl
	fh = max(hl, hr)
	fw = max(wt, wb)

	# We determine the gain needed without exeeding either of the width and height
	if (gain == -1):
		gain_w = fw / max_width
		gain_h = fh / max_height
		gain = max(gain_w, gain_h)	# This might seem misleading. But this gives the largest gain division, avoiding the exceeding w h.  

	# Predict the dynamic height and width
	# print(f'Size before gain, {fw} x {fh}, expected gain {gain}')
	output_height = int(fh / gain)
	output_width = int(fw / gain)
	# print(f'Size after gain, {output_width} x {output_height}')

	# Define the destination points for the unwarped image
	dst = np.array([
		[0, 0],
		[output_width - 1, 0],
		[output_width - 1, output_height - 1],
		[0, output_height - 1]], dtype="float32")

	# Compute the perspective transform matrix and then apply it
	trans_mat = cv2.getPerspectiveTransform(rect, dst)
	warped = cv2.warpPerspective(image, trans_mat, dsize=(output_width, output_height))

	return warped

def process_license_plates(
	model_path,
	image_input_path,
	output_dir,
	license_plate_class_id,
	output_plate_width=300,
	output_plate_height=100,
	size_gain=0.5
):
	"""
	Loads a YOLO pose model, detects license plates, and crops them using keypoints.

	Args:
		model_path (str): Path to your custom-trained YOLO pose model (.pt file).
		image_input_path (str): Path to a single image file or a directory of images.
		output_dir (str): Directory to save the cropped license plate images.
		license_plate_class_id (int): The class ID for license plates in your model.
		output_plate_width (int): Desired width of the cropped license plate. -1 for dynamic
		output_plate_height (int): Desired height of the cropped license plate. -1 for dynamic
	"""
	# Load the YOLO model
	try:
		model = YOLO(model_path)
		print(f"YOLO model loaded from: {model_path}")
	except Exception as e:
		print(f"Error loading YOLO model: {e}")
		return

	# Ensure output directory exists
	os.makedirs(output_dir, exist_ok=True)

	# Get list of image files
	image_files = []
	if os.path.isfile(image_input_path):
		image_files.append(image_input_path)
	elif os.path.isdir(image_input_path):
		# Supported image extensions
		for ext in ['jpg', 'jpeg', 'png', 'bmp', 'tiff']:
			image_files.extend(glob.glob(os.path.join(image_input_path, '**' , f'*.{ext}'), recursive=True))
		if not image_files:
			print(f"🚫 No image files found in directory: {image_input_path}")
			return
	else:
		print(f"❌ Invalid image_input_path: {image_input_path}. Must be a file or directory.")
		return

	for img_path in tqdm(image_files, desc="🔃 Processing Images"):
		img = cv2.imread(img_path)
		if img is None:
			tqdm.write(f"⚠️ Warning: Could not read image {img_path}. Skipping.")
			continue

		# h, w, _ = img.shape

		# Perform inference
		results = model(img, verbose=False) # verbose=False to suppress detailed output per image

		plate_count = 0
		for r in results:
			# Iterate through detections
			for i, box in enumerate(r.boxes):
				class_id = int(box.cls)
				conf = float(box.conf)

				if class_id == license_plate_class_id:
					# Get keypoints if available
					if r.keypoints is not None and len(r.keypoints.xy) > i:
						keypoints_normalized = r.keypoints.xy[i].cpu().numpy() # Get keypoints for current detection
						
						# Ensure we have exactly 4 keypoints for perspective transform
						if keypoints_normalized.shape[0] == 4:
							# Perform perspective transform
							try:
								warped_plate = four_point_transform(img, keypoints_normalized, output_plate_width, output_plate_height, -1)
								output_filename = f"{os.path.basename(img_path).split('.')[0]}_p{plate_count}.png"
								output_filepath = os.path.join(output_dir, output_filename)
								cv2.imwrite(output_filepath, warped_plate)
								# print(f"  - Saved cropped plate: {output_filepath} (Confidence: {conf:.2f})")
								plate_count += 1
							except Exception as e:
								tqdm.write(f"❌ Error during perspective transform for a plate in {img_path}, coords {keypoints_normalized}: {e}")
        
							# Draw on the image
							for kp in keypoints_normalized:
								cv2.circle(img, (int(kp[0]), int(kp[1])), 5, (0, 255, 0), -1)
						else:
							tqdm.write(f"⛷️ Skipping plate in {img_path}: Expected 4 keypoints, got {keypoints_normalized.shape[0]}.")
					else:
						tqdm.write(f"⛷️ Skipping plate in {img_path}: No keypoints found for detection.")

		if plate_count == 0:
			print(f"🗑️ No license plates detected in {img_path}.")
		
		# Optionally, save the image with keypoints drawn (for debugging/visualization)
		cv2.imwrite(os.path.join(output_dir, f"{os.path.basename(img_path)}"), img)


if __name__ == "__main__":
	# --- Configuration ---
	# Path to your custom-trained YOLO pose model (.pt file)
	YOLO_MODEL_PATH = 'yolo11n-pose-1700_openvino_model' # <--- !!! CHANGE THIS !!!

	# Path to the input images (can be a single image file or a directory)
	IMAGE_INPUT_PATH = 'D:/Projects/FYP/Yolo/License Plates' # <--- !!! CHANGE THIS !!!

	# Directory where cropped license plates will be saved
	OUTPUT_CROPS_DIR = 'cropped_license_plates'

	# The class ID that corresponds to 'license plate' in your YOLO model's classes.
	# You'll need to know this from your model's training configuration (data.yaml).
	LICENSE_PLATE_CLASS_ID = 0  # <--- !!! CHANGE THIS to your actual class ID !!!

	# Desired dimensions for the output cropped license plate images
	DESIRED_PLATE_WIDTH = 320 # 300
	DESIRED_PLATE_HEIGHT = 320 # 100

	# --- Run the processing ---
	process_license_plates(
		YOLO_MODEL_PATH,
		IMAGE_INPUT_PATH,
		OUTPUT_CROPS_DIR,
		LICENSE_PLATE_CLASS_ID,
		DESIRED_PLATE_WIDTH,
		DESIRED_PLATE_HEIGHT
	)

	print(f"\nProcessing complete. Cropped plates saved to: {OUTPUT_CROPS_DIR}")
