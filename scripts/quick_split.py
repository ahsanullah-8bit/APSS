# Disclaimer: You know it, ChatGPT generated with some customizations
# Usage:
#   Set the source and destination directory and split ratios in the split(...) call at the bottom

# This script expects the simple YOLO directory structure. It may create some more during the process and delete thems.
# <src>
# │   data.yaml
# ├───images
# └───labels
#
# Creates another with structure
# <dst>
# │   data.yaml
# │
# ├───test
# │   ├───images
# │   └───labels
# ├───train
# │   ├───images
# │   └───labels
# └───val
#     ├───images
#     └───labels


from ultralytics.data.split import autosplit
from ultralytics.hub import check_dataset
import os
import shutil
from tqdm import tqdm
import yaml
import zipfile

def split(src, dst, train=0.80, val=0.10, test=0.10):
    # Copy the folder over to the destination
    try:
        os.mkdir(dst)
        print(f"🔃 Copying {src} to {dst}")
        shutil.copytree(src, dst, dirs_exist_ok=True)
        print(f"✅ Folder '{src}' copied successfully to '{dst}'")
    except FileNotFoundError:
        print(f"Error: Folder '{src}' not found.")
    except Exception as e:
        print(f"An error occurred: {e}")

    # Do the split
    src_images = os.path.join(dst, 'images')
    src_labels = os.path.join(dst, 'labels')

    autosplit(
        path=src_images,
        weights=(train, val, test),
        annotated_only=False,
    )

    # These come from the autosplit results
    split_files = {
        'train': os.path.join(dst, 'autosplit_train.txt'),
        'val': os.path.join(dst, 'autosplit_val.txt'),
        'test': os.path.join(dst, 'autosplit_test.txt'),
    }
    has_tests = os.path.exists(split_files['test'])

    for split, file in split_files.items():
        if not os.path.exists(file):
            continue
            
        print(f"🔃 Writing {split}")
        with open(file, 'r') as f:
            lines = f.read().splitlines()
            
        split_dir = os.path.join(dst, split)
        dst_img_dir = os.path.join(split_dir, 'images')
        dst_lbl_dir = os.path.join(split_dir, 'labels')
        os.makedirs(dst_img_dir, exist_ok=True)
        os.makedirs(dst_lbl_dir, exist_ok=True)

        for line in tqdm(lines):
            img_path = line.replace('./', '')  # remove leading ./
            img_name = os.path.basename(img_path)
            lbl_name = os.path.splitext(img_name)[0] + '.txt'

            # Copy image
            shutil.move(os.path.join(src_images, img_name), os.path.join(dst_img_dir, img_name))

            # Copy label if exists
            src_lbl_path = os.path.join(src_labels, lbl_name)
            if os.path.exists(src_lbl_path):
                shutil.copy(src_lbl_path, os.path.join(dst_lbl_dir, lbl_name))
        
        os.remove(file)
        print(f'✅ Removed {file}')

    # Create the data.yaml
    # Load the original data.yaml
    # Note: This might mis-order/reorder your yaml file
    src_data_yaml = os.path.join(dst, 'data.yaml')
    with open(src_data_yaml, 'r') as f:
        original_yaml = yaml.safe_load(f)

    original_yaml['train'] = 'train/images'
    original_yaml['val'] = 'val/images'
    if has_tests:
        original_yaml['test'] = 'test/images'

    # Write the data.yaml
    with open(os.path.join(dst, 'data.yaml'), 'w') as f:
        yaml.dump(original_yaml, f, sort_keys=False)

    # Delete the images and labels directories from before.
    shutil.rmtree(os.path.join(dst, 'images'))
    shutil.rmtree(os.path.join(dst, 'labels'))
    print('✅ Removed images and labels directories \n\n')


def check_dataset_files(dataset_dir):
    """
    Verifies that each image file in the specified directories has a corresponding .txt label file
    in the respective labels directory and the data.yaml file is correct.

    Args:
        dataset_dir (str): Path to the dataset directory.

    Returns:
        bool: True if all images have corresponding labels, False otherwise.  Also prints details
              of any missing files.
    """
    all_files_exist = True
    
    sub_dirs = {
        'train': os.path.join(dataset_dir, 'train'),
        'val': os.path.join(dataset_dir, 'val'),
        'test': os.path.join(dataset_dir, 'test')
    }

    for key, dir in sub_dirs.items():
        if not os.path.exists(dir):
            print(f'⚠️ No {key} directory found, skipping')
            continue
        
        labels_dir = os.path.join(dir, 'labels')
        images_dir = os.path.join(dir, 'images')
        image_files = [f for f in os.listdir(images_dir) if os.path.isfile(os.path.join(images_dir, f))]
        for image_file in tqdm(image_files, desc=f"Check {key}"):
            # Construct the expected label file name
            label_file_name = os.path.splitext(image_file)[0] + ".txt"
            label_file_path = os.path.join(labels_dir, label_file_name)
            if not os.path.exists(label_file_path):
                print(f"❌ Missing label file for image: {image_file} (Expected: {label_file_path})")
                all_files_exist = False

    return all_files_exist

def zip_folder(folder_path, output_zip_path=''):
    """
    Zips the contents of a folder (including subfolders and files) into a zip file.

    Args:
        folder_path (str): The path to the folder you want to zip.
        output_zip_path (str): The path to the output zip file (e.g., 'my_folder.zip').
    """
    
    if output_zip_path == '':
        output_zip_path = f'{folder_path}.zip'
    
    # Ensure that the folder path exists
    if not os.path.exists(folder_path):
        print(f"Error: Folder not found at {folder_path}")
        return

    # Create a new zip file
    with zipfile.ZipFile(output_zip_path, 'w', zipfile.ZIP_DEFLATED) as zipf:
        # Walk through the folder and its subfolders
        for foldername, subfolders, filenames in os.walk(folder_path):
            # Add the current folder to the zip file
            arcname = os.path.relpath(foldername, folder_path)  # Get relative path for archive
            if arcname:
                zipf.write(foldername, arcname)

            for filename in filenames:
                # Create complete file path
                file_path = os.path.join(foldername, filename)
                # Add the file to the zip file
                arcname = os.path.join(os.path.relpath(foldername, folder_path), filename) # Get relative path for archive
                zipf.write(file_path, arcname)
    print(f"✅ Successfully zipped folder '{folder_path}' to '{output_zip_path}'")


################################
##      MAIN
################################

if __name__ == "__main__":
    src = 'License_Plates_keypoints'
    dst = 'License_Plates'
    split(src, dst)
    all_exist = check_dataset_files(dst)
    if all_exist:
        print('✅ Dataset is intact')
    else:
        print('❌ Dataset has missing files')
    