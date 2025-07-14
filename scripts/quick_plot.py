# Disclaimer: Credit goes to ChatGPT, though some customizations are made.
# I don't REALLY have time to write everything myself.

# A Quick script to plot the downloaded dataset, just to see if the values are still intact.
# Usage:
#   Set the dataset_dir
#   Call the function related to your dataset in the loop below

import os
from PIL import Image, ImageDraw
from tqdm import tqdm
import yaml
import math


def draw_polygons(image_path, label_path):
    with Image.open(image_path) as img:
        draw = ImageDraw.Draw(img)

        w, h = img.size
        with open(label_path, "r") as f:
            for line in f:
                parts = line.strip().split()
                cls_idx = int(parts[0])
                coords = list(map(float, parts[1:]))

                if len(coords) % 2 != 0:
                    print(f"⚠️  Skipping malformed label in {label_path}")
                    continue

                polygon = [(coords[i] * w, coords[i+1] * h) for i in range(0, len(coords), 2)]
                draw.polygon(polygon, outline="red", width=2)
                draw.text(polygon[0], class_names[cls_idx], fill="red")

        return img

def draw_obbs(image_path, label_path, class_names=None, box_color="blue", text_color="blue", box_width=2):
    """
    Draws oriented bounding boxes (OBBs) on an image based on labels in a text file.
    Credit: ChatGPT

    Args:
        image_path (str): Path to the image file.
        label_path (str): Path to the label file. Each line should contain:
                           class_id x_center y_center width height angle (normalized).
                           Angle is typically in radians.
        class_names (list, optional): List of class names corresponding to the class IDs.
                                      Defaults to None (no class names displayed).
        box_color (str): Color of the bounding boxes. Defaults to "blue".
        text_color (str): Color of the class name text. Defaults to "blue".
        box_width (int): Width of the bounding box lines. Defaults to 2.

    Returns:
        PIL.Image.Image: The image with oriented bounding boxes drawn on it.
    """
    try:
        with Image.open(image_path) as img:
            draw = ImageDraw.Draw(img)
            width, height = img.size

            with open(label_path, "r") as f:
                for line in f:
                    parts = line.strip().split()
                    if len(parts) != 6:
                        print(f"⚠️ Skipping malformed OBB label in {label_path}: {line}")
                        continue

                    try:
                        cls_idx = int(parts[0])
                        xc_norm, yc_norm, bw_norm, bh_norm, angle_rad = map(float, parts[1:])

                        # Convert normalized coordinates to absolute
                        xc = xc_norm * width
                        yc = yc_norm * height
                        bw = bw_norm * width
                        bh = bh_norm * height

                        # Calculate the 4 corners of the rotated rectangle
                        cos_a = math.cos(angle_rad)
                        sin_a = math.sin(angle_rad)

                        dx = bw / 2
                        dy = bh / 2

                        corners = [
                            (xc - dx * cos_a + dy * sin_a, yc - dx * sin_a - dy * cos_a),
                            (xc + dx * cos_a + dy * sin_a, yc + dx * sin_a - dy * cos_a),
                            (xc + dx * cos_a - dy * sin_a, yc + dx * sin_a + dy * cos_a),
                            (xc - dx * cos_a - dy * sin_a, yc - dx * sin_a + dy * cos_a)
                        ]

                        draw.polygon(corners, outline=box_color, width=box_width)

                        if class_names and 0 <= cls_idx < len(class_names):
                            text = class_names[cls_idx]
                            # Adjust text position slightly above the first corner
                            text_x = int(corners[0][0])
                            text_y = int(corners[0][1]) - 10
                            draw.text((text_x, text_y), text, fill=text_color)

                    except ValueError:
                        print(f"⚠️ Skipping line with invalid numeric values in {label_path}: {line}")
                        continue
                    except IndexError:
                        print(f"⚠️ Class ID out of range in {label_path}: {cls_idx}")
                        continue

            return img

    except FileNotFoundError:
        print(f"Error: Image or label file not found at the specified paths.")
        return None
    except Exception as e:
        print(f"An error occurred: {e}")
        return None

def draw_bounding_boxes(image_path, label_path, class_names=None, box_color="blue", text_color="blue", box_width=4):
    """
    Draws bounding boxes on an image based on labels in a text file.
    Credit: Gemini

    Args:
        image_path (str): Path to the image file.
        label_path (str): Path to the label file. Each line should contain:
                           class_id x_center y_center width height (normalized).
        class_names (list, optional): List of class names corresponding to the class IDs.
                                      Defaults to None (no class names displayed).
        box_color (str): Color of the bounding boxes. Defaults to "blue".
        text_color (str): Color of the class name text. Defaults to "blue".
        box_width (int): Width of the bounding box lines. Defaults to 2.

    Returns:
        PIL.Image.Image: The image with bounding boxes drawn on it.
    """
    try:
        with Image.open(image_path) as img:
            draw = ImageDraw.Draw(img)
            width, height = img.size

            with open(label_path, "r") as f:
                for line in f:
                    parts = line.strip().split()
                    if len(parts) != 5:
                        print(f"⚠️ Skipping malformed bounding box label in {label_path}: {line}")
                        continue

                    try:
                        class_id = int(parts[0])
                        x_center_norm, y_center_norm, w_norm, h_norm = map(float, parts[1:])

                        x_center = x_center_norm * width
                        y_center = y_center_norm * height
                        box_width_px = w_norm * width
                        box_height_px = h_norm * height

                        x1 = int(x_center - box_width_px / 2)
                        y1 = int(y_center - box_height_px / 2)
                        x2 = int(x_center + box_width_px / 2)
                        y2 = int(y_center + box_height_px / 2)

                        draw.rectangle([(x1, y1), (x2, y2)], outline=box_color, width=box_width)

                        if class_names and 0 <= class_id < len(class_names):
                            text = class_names[class_id]
                            # Adjust text position slightly above the top-left corner of the box
                            text_x = x1
                            text_y = y1 - 10
                            draw.text((text_x, text_y), text, fill=text_color)

                    except ValueError:
                        print(f"⚠️ Skipping line with invalid numeric values in {label_path}: {line}")
                        continue
                    except IndexError:
                        print(f"⚠️ Class ID out of range in {label_path}: {class_id}")
                        continue

            return img

    except FileNotFoundError:
        print(f"Error: Image or label file not found at the specified paths.")
        return None
    except Exception as e:
        print(f"An error occurred: {e}")
        return None

def draw_bounding_boxes_and_keypoints(image_path, label_path, class_names=None, box_color="blue", text_color="blue", keypoint_color="red", box_width=4, keypoint_size=10):
    """
    Draws bounding boxes and keypoints on an image based on labels in a text file.
    Credit: Gemini

    Args:
        image_path (str): Path to the image file.
        label_path (str): Path to the label file.  Each line should contain:
                         class_id x_center y_center width height kpx1 kpy1 vis1 kpx2 kpy2 vis2 ...
                         where x_center, y_center, width, height are normalized bounding box coordinates,
                         and kpx, kpy are absolute keypoint coordinates, vis is visibility (0, 1, or 2).
        class_names (list, optional): List of class names corresponding to the class IDs.
                                      Defaults to None (no class names displayed).
        box_color (str): Color of the bounding boxes. Defaults to "blue".
        text_color (str): Color of the class name text. Defaults to "blue".
        keypoint_color (str): Color of the keypoints. Defaults to "red".
        box_width (int): Width of the bounding box lines. Defaults to 2.
        keypoint_size (int): Diameter of the keypoint circles. Defaults to 5.

    Returns:
        PIL.Image.Image: The image with bounding boxes and keypoints drawn on it.
    """
    try:
        with Image.open(image_path) as img:
            draw = ImageDraw.Draw(img)
            width, height = img.size

            with open(label_path, "r") as f:
                for line in f:
                    parts = line.strip().split()
                    if len(parts) < 5:
                        print(f"⚠️ Skipping malformed label (insufficient data) in {label_path}: {line}")
                        continue

                    try:
                        class_id = int(parts[0])
                        x_center_norm, y_center_norm, w_norm, h_norm = map(float, parts[1:5])

                        x_center = x_center_norm * width
                        y_center = y_center_norm * height
                        box_width_px = w_norm * width
                        box_height_px = h_norm * height

                        x1 = int(x_center - box_width_px / 2)
                        y1 = int(y_center - box_height_px / 2)
                        x2 = int(x_center + box_width_px / 2)
                        y2 = int(y_center + box_height_px / 2)

                        draw.rectangle([(x1, y1), (x2, y2)], outline=box_color, width=box_width)

                        if class_names and 0 <= class_id < len(class_names):
                            text = class_names[class_id]
                            text_x = x1
                            text_y = y1 - 10
                            draw.text((text_x, text_y), text, fill=text_color)

                        # Draw keypoints if present
                        if len(parts) > 5:
                            keypoint_data = parts[5:]
                            if len(keypoint_data) % 3 != 0:
                                print(f"⚠️ Skipping malformed keypoint data in {label_path}: {line}")
                            else:
                                num_keypoints = len(keypoint_data) // 3
                                for i in range(num_keypoints):
                                    kpx = float(keypoint_data[i * 3])
                                    kpy = float(keypoint_data[i * 3 + 1])
                                    vis = int(keypoint_data[i * 3 + 2])

                                    # Only draw if visible
                                    if vis > 0:  # 0: invisible, 1: visible, 2:occluded.
                                        x = int(kpx * width)
                                        y = int(kpy * height)
                                        draw.ellipse((x - keypoint_size // 2, y - keypoint_size // 2, x + keypoint_size // 2, y + keypoint_size // 2),
                                                     fill=keypoint_color)
                                        # You could add the keypoint index here as well, if needed
                                        # draw.text((x,y), str(i), fill="white")

                    except ValueError:
                        print(f"⚠️ Skipping line with invalid numeric values in {label_path}: {line}")
                        continue
                    except IndexError:
                        print(f"⚠️ Class ID or keypoint index out of range in {label_path}: {line}")
                        continue

            return img

    except FileNotFoundError:
        print(f"Error: Image or label file not found at the specified paths.")
        return None
    except Exception as e:
        print(f"An error occurred: {e}")
        return None


######################################
####            MAIN
######################################

if __name__ == '__main__':
    from argparse import ArgumentParser
    parser = ArgumentParser(prog='APSS Quick-Plot', allow_abbrev=True, description='A Script to plot annotations on images for testing.')
    
    parser.add_argument('--dataset', type=str, required=True, help='Provide path to the dataset.')
    parser.add_argument('--task', type=str, required=True, choices=['detect', 'obb', 'pose', 'segment_wm', 'segment_wp', 'classify'], help='Provide task name. Tasks include Detect, OBB, Pose, Segment With Masks, Segment With Polygons and Classify. See help.')
    parser.add_argument('--class_names', type=dict[int, str], help='List of class names')
    parser.add_argument('--yaml_file', type=str, help='Path to the data.yaml file. Necessary if you dont provide class_names')
    
    args = parser.parse_args()
    
    task_type: str = args.task
    
    dataset_dir: str = args.dataset
    images_dir = os.path.join(dataset_dir, "images")
    labels_dir = os.path.join(dataset_dir, "labels")
    output_dir = os.path.join(dataset_dir, "annotated")
    os.makedirs(output_dir, exist_ok=True)

    if args.class_names:
        class_names: dict[int, str] = args.class_names
    else:
        # Load class names from data.yaml
        if args.yaml_file:
            yaml_file: str = args.yaml_file
        elif os.path.exists(os.path.join(dataset_dir, f"data.yml")):
            yaml_file = os.path.join(dataset_dir, 'data.yml')
        elif os.path.exists(os.path.join(dataset_dir, "data.yaml")):
            yaml_file = os.path.join(dataset_dir, 'data.yaml')
        else:
            print('❌ Error: No data.yml was found in the dataset directory. Please provide either class_names or yaml_file!')
            exit(-1)
        
        if not os.path.exists(yaml_file):
            print(f'❌ Error: {yaml_file} doesn''t exist. Please provide one!')
            exit(-1)
            
        with open(yaml_file, "r") as f:
            data_yaml = yaml.safe_load(f)
        class_names: dict[int, str] = data_yaml["names"]
        
    if class_names.__len__ == 0:
        print(f'❌ Invalid list of class_names: {class_names}')
        exit(-1)
        
    # Loop through all labels and draw
    img_exts = ['png', 'jpg', 'jpeg', 'webp']
    for label_file in tqdm(os.listdir(labels_dir), desc="Drawing annotations"):
        if not label_file.endswith(".txt"):
            print(f'⚠️ Warning: Label file with invalid extension: {label_file}')
            continue
        
        label_path = os.path.join(labels_dir, label_file)
        
        img_file = ''
        for ext in img_exts:
            img_file = label_file.replace('.txt', f'.{ext}')
            if os.path.exists(os.path.join(images_dir, img_file)):
                break
        
        img_path = os.path.join(images_dir, img_file)
        if img_file.__len__() > 0 and os.path.exists(img_path):
            img_out_path = os.path.join(output_dir, img_file)
            if os.path.exists(img_out_path):
                continue
            
            result_img = None
            if (task_type == 'detect'):
                result_img = draw_bounding_boxes(img_path, label_path)
            elif (task_type == 'pose'):
                result_img = draw_bounding_boxes_and_keypoints(img_path, label_path)
            elif (task_type == 'obb'):
                result_img = draw_obbs(img_path, label_path)
            elif (task_type == 'segment_wp'):
                result_img = draw_polygons(img_path, label_path)
            else:
                print('❌ Error: Task not implemented yet!')
                exit(-1)
                
            if not result_img:
                print(f'❌ Results is None for {img_path}')
                continue
            
            result_img.save(fp=img_out_path)
        else:
            print(f"❌ Image file not found for label: {label_file}")
            
    print(f"✅ Saved annotations at {output_dir}")
