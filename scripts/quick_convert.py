# A quick script to convert our License Plate labelbox project to YOLO format.
# If you want to use a local .ndjson exported from labelbox. You have to load it, set export_json to that.
# Usage: 
#   Pass your LABELBOX_API_KEY to the Client(...) or set it as env variable
#   Set project ID
#   Set Project name
#   Set Ontology mapping values: what is the name in labelbox vs what you want in data.yaml

import os
import time
from concurrent.futures import ThreadPoolExecutor, as_completed
import urllib.request
import urllib.error
from tqdm import tqdm
from PIL import Image
import labelbox as lb
import yaml

def lb_polygon_to_yolo(data_row, project_id, ontology_mapping):
    yolo_format = {
        "masks": {"xy": []},
        "cls": []
    }

    class_list = ontology_mapping.keys()
    labels = data_row["projects"][project_id]["labels"]

    for label in labels:
        for obj in label["annotations"]["objects"]:
            if obj["annotation_kind"] == "ImagePolygon" and obj["name"] in class_list:
                polygon = obj["polygon"]

                if len(polygon) < 3:
                    raise ValueError(f"Expected at least 3 points, got {len(polygon)}: {polygon}")
                if polygon[0] == polygon[-1]:
                    polygon = polygon[:-1]

                coordinates = [{"x": pt["x"], "y": pt["y"]} for pt in polygon]
                yolo_format["masks"]["xy"].append(coordinates)
                yolo_format["cls"].append(ontology_mapping[obj["name"]])

    return yolo_format


def download_with_retries(url, destination, retries=3, backoff=2):
    for attempt in range(retries):
        try:
            urllib.request.urlretrieve(url, destination)
            return  # Success
        except urllib.error.URLError as e:
            print(f"⚠️  Attempt {attempt+1} failed for {url} — {e}")
            if attempt < retries - 1:
                time.sleep(backoff ** attempt)
            else:
                raise  # Final failure


def process_data_row(data_row, project_id, ontology_mapping, class_to_index, images_dir, labels_dir):
    try:
        img_name = data_row['data_row']['external_id']
        img_url = data_row['data_row']['row_data']
        img_base = os.path.splitext(img_name)[0]

        # Save image
        img_path = os.path.join(images_dir, img_name)
        download_with_retries(img_url, img_path)

        # Convert Labelbox annotation
        polygon_data = lb_polygon_to_yolo(data_row, project_id, ontology_mapping)

        # Get image size for normalization
        with Image.open(img_path) as img:
            w, h = img.size

        # Save annotation file in YOLO format
        label_path = os.path.join(labels_dir, f"{img_base}.txt")
        with open(label_path, "w") as f:
            for cls_name, polygon in zip(polygon_data["cls"], polygon_data["masks"]["xy"]):
                cls_idx = class_to_index[cls_name]
                xy_normalized = []
                for pt in polygon:
                    x = round(pt["x"] / w, 6)
                    y = round(pt["y"] / h, 6)
                    xy_normalized.append(f"{x} {y}")
                line = f"{cls_idx} " + " ".join(xy_normalized)
                f.write(line + "\n")
    except Exception as e:
        print(f"Error processing {data_row['data_row']['external_id']}: {e}")




######################################
####            MAIN
######################################

if __name__ == "__main__":
    ################################################################
    # Initialize Labelbox client
    # NOTE: This call requires Labelbox's API-key as argument or you should set the LABELBOX_API
    client = lb.Client(api_key='') # My bad, forgot to remove the API Key. It was an expired one.
    PROJECT_ID = "cm7xlwbmk053507wwcvfz1nuw"
    project_name = "License_Plates"
    # What the names are in LabelBox VS What you want it to be in data.yaml
    ontology_mapping_polygon = {"license_plate": "license_plate"}

    # The rest #####################################################

    project = client.get_project(PROJECT_ID)

    print("🔄 Exporting project annotations...")
    export_task = project.export()
    export_task.wait_till_done()
    # export_task = lb.ExportTask.get_task(client, "")
    export_json = [data_row.json for data_row in export_task.get_buffered_stream()]

    # Project setup
    classes = sorted(set(ontology_mapping_polygon.values()))
    class_to_index = {cls_name: idx for idx, cls_name in enumerate(classes)}

    # Create directories
    os.makedirs(project_name, exist_ok=True)
    images_dir = os.path.join(project_name, "images")
    labels_dir = os.path.join(project_name, "labels")
    os.makedirs(images_dir, exist_ok=True)
    os.makedirs(labels_dir, exist_ok=True)

    # Write data.yaml
    data_yaml = {
        "path": ".",
        "train": "images",
        "val": "images",
        "nc": len(classes),
        "names": {i: cls for i, cls in enumerate(classes)}
    }
    with open(os.path.join(project_name, "data.yaml"), "w") as f:
        yaml.dump(data_yaml, f, sort_keys=False)

    # Process data rows with multithreading
    max_workers = 8  # Adjust based on your system's capabilities
    with ThreadPoolExecutor(max_workers=max_workers) as executor:
        futures = [
            executor.submit(
                process_data_row,
                data_row,
                PROJECT_ID,
                ontology_mapping_polygon,
                class_to_index,
                images_dir,
                labels_dir
            )
            for data_row in export_json
        ]
        for future in tqdm(as_completed(futures), total=len(futures), desc="Exporting"):
            future.result()

    print(f"\n✅ YOLO project created: {project_name}")
