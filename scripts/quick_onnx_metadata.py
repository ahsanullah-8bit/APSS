# A Quick Script to print the metadata of an ONNX model

import onnxruntime as ort
from onnxruntime import ModelMetadata
import numpy as np
import onnx
from pprint import pprint
import os
from argparse import ArgumentParser

def inspect_onnx(model_path):
    if not os.path.exists(model_path):
        raise FileNotFoundError(f"Model not found: {model_path}")

    # Load model with ONNX Runtime
    session = ort.InferenceSession(model_path)
    print(f"\n🔍 Model: {os.path.basename(model_path)}")

    # ONNX Metadata
    model = onnx.load(model_path)
    print("\n📌 ONNX Metadata:")
    print("  Producer:", model.producer_name)
    print("  Domain:", model.domain)
    print("  Opset Version:", model.opset_import[0].version)
    print("  Doc String:", model.doc_string)
    
    # ONNXRuntime Metadata
    meta = session.get_modelmeta()
    print("\n📌 ONNXRuntime Metadata:")
    print(f"  Graph Name       : {meta.graph_name}")
    print(f"  Description      : {meta.description}")
    print(f"  Domain           : {meta.domain}")
    print(f"  Version          : {meta.version}")
    print(f"  Producer Name    : {meta.producer_name}")
    custom_metadata = meta.custom_metadata_map
    if custom_metadata:
        print("\n🔖 Custom Metadata:")
        for key, value in custom_metadata.items():
            print(f"  {key}: {value}")
    else:
        print("\n🔖 No custom metadata embedded.")

    # Inputs
    print("\n📥 Inputs:")
    for inp in session.get_inputs():
        print(f"  Name: {inp.name}")
        print(f"  Type: {inp.type}")
        print(f"  Shape: {inp.shape}")

    # Outputs
    print("\n📤 Outputs:")
    for out in session.get_outputs():
        print(f"  Name: {out.name}")
        print(f"  Type: {out.type}")
        print(f"  Shape: {out.shape}")

    # Node summary
    print("\n🔧 Ops Summary:")
    op_counts = {}
    for node in model.graph.node:
        op = node.op_type
        op_counts[op] = op_counts.get(op, 0) + 1
    for op, count in sorted(op_counts.items(), key=lambda x: -x[1]):
        print(f"  {op}: {count}")

    # Optional: test dummy inference
    try:
        print("\n🧪 Dummy Inference:")
        dummy_input = {}
        for inp in session.get_inputs():
            shape = [dim if isinstance(dim, int) else 1 for dim in inp.shape]
            dummy_input[inp.name] = np.random.rand(*shape).astype(np.float32)
        results = session.run(None, dummy_input)
        for i, res in enumerate(results):
            print(f"  Output[{i}] shape: {res.shape}")
    except Exception as e:
        print("  ⚠️ Dummy run failed:", str(e))


if __name__ == "__main__":
    parser = ArgumentParser(prog='APSS Quick ONNX Metadata', description='Prints metadata of an onnx model')
    parser.add_argument('--model', required=True, help='Path to the .onnx model file')
    
    args = parser.parse_args()
    
    if args.model:
        inspect_onnx(args.model)
