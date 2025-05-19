# This Python file uses the following encoding: utf-8

from ultralytics import YOLO
import os

def download_yolo_models(model_list):
    for model_name in model_list:
        print(f'\n\nDownloading {model_name}')
        model = YOLO(model_name)
        model.export(format='onnx')

if __name__ == "__main__":
    download_yolo_models(["yolo11n.pt", "yolo11n-seg.pt", "yolo11n-pose"])
