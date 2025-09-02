import os
import sys
import json
import csv
from argparse import ArgumentParser
from hub_sdk import HUBClient

def export_metrics(model_id: str) -> list[dict[str, any]] :
	client = HUBClient({"api_key": api_key})
	if not client.authenticated:
		print("ERROR: Authentication failed. Check your API key.", file=sys.stderr)
		sys.exit(1)

	model = client.model(model_id)

	try:
		metrics = model.get_metrics()
		return metrics
	except Exception as e:
		print(f"ERROR: Failed to fetch metrics: {e}", file=sys.stderr)
		sys.exit(1)

def write_to_json(metrics: list[dict[str, any]], out_filename: str):
	out_path = f"{out_filename}.json"
 
	try:
		with open(out_path, "w") as f:
			json.dump(metrics, f, indent=2)
	except IOError as e:
		print(f"ERROR: Could not write to {out_path}: {e}", file=sys.stderr)
		sys.exit(1)

	print(f"✅ Metrics saved to {out_path}")

def write_to_csv(metrics: list[dict[str, any]], out_filename: str):
	out_path = f"{out_filename}.csv"

	try:
		rows = {}
		for entry in metrics:
			key = entry["meta"]["name"]
			data = entry["data"]
			for epoch, value in data.items():
				epoch = int(epoch)
				if epoch not in rows:
					rows[epoch] = {"epoch": epoch}
				rows[epoch][key] = value

		with open(out_path, "w", newline="") as f:
			fieldnames = ["epoch"] + sorted({entry["meta"]["name"] for entry in metrics})
			writer = csv.DictWriter(f, fieldnames=fieldnames)
			writer.writeheader()
			for row in sorted(rows.values(), key=lambda r: r["epoch"]):
				writer.writerow(row)
	except IOError as e:
		print(f"ERROR: Could not write to {out_path}: {e}", file=sys.stderr)
		sys.exit(1)

	print(f"✅ Metrics saved to {out_path}")

# main
if __name__ == "__main__":
	parser = ArgumentParser(
        prog="fetch_hub_metrics",
        description="Fetch YOLO model training metrics from Ultralytics HUB",
    )
	parser.add_argument(
        "--api_key", help="Ultralytics HUB API key (or set ULTRALYTICS_API_KEY)"
    )
	parser.add_argument(
        "--model_id",
        required=True,
        help="Model ID (e.g., Q4pgGzd78aPbtmdrvdwB)",
    )
	parser.add_argument(
        "--out",
        default=None,
        help="Output JSON file (defaults to <model_name>_metrics.json)",
    )
	args = parser.parse_args()

    ############
	api_key = args.api_key or os.getenv("ULTRALYTICS_API_KEY")
	if not api_key:
		print(
            "ERROR: No API key provided. Use --api_key or set ULTRALYTICS_API_KEY.",
            file=sys.stderr,
        )
		sys.exit(1)

	filename = args.out or f"{args.model_id}_metrics"
	metrics = export_metrics(args.model_id)
	write_to_json(metrics, filename)
	write_to_csv(metrics, filename)
