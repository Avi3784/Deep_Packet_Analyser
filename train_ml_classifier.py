#!/usr/bin/env python3
"""
Train and export a label-only decision tree model for DPI traffic classification.

Input CSV columns:
- protocol
- dst_port
- payload_length
- has_sni
- has_http_host
- sni_len
- is_dns_port
- is_https_port
- is_http_port
- label

Output format matches models/traffic_dt_model.txt.
"""

import argparse
import csv
from pathlib import Path

from sklearn.tree import DecisionTreeClassifier


FEATURES = [
    "protocol",
    "dst_port",
    "payload_length",
    "has_sni",
    "has_http_host",
    "sni_len",
    "is_dns_port",
    "is_https_port",
    "is_http_port",
]


def load_dataset(csv_path: Path):
    x = []
    y = []

    with csv_path.open("r", newline="", encoding="utf-8") as f:
        reader = csv.DictReader(f)
        missing = [c for c in FEATURES + ["label"] if c not in reader.fieldnames]
        if missing:
            raise ValueError(f"Missing required CSV columns: {missing}")

        for row in reader:
            x.append([float(row[c]) for c in FEATURES])
            y.append(row["label"].strip().upper())

    if not x:
        raise ValueError("Dataset is empty")

    return x, y


def export_model(tree: DecisionTreeClassifier, output_file: Path):
    t = tree.tree_

    with output_file.open("w", encoding="utf-8") as f:
        f.write("DPI_ML_TREE_V1\n")
        f.write(f"{len(FEATURES)}\n")
        f.write(f"0 {t.node_count}\n")

        for i in range(t.node_count):
            is_leaf = int(t.children_left[i] == t.children_right[i])
            feature_index = int(t.feature[i])
            threshold = float(t.threshold[i]) if not is_leaf else 0.0
            left = int(t.children_left[i])
            right = int(t.children_right[i])

            if is_leaf:
                class_idx = int(t.value[i][0].argmax())
                label = tree.classes_[class_idx]
            else:
                label = "UNKNOWN"

            f.write(
                f"{i} {is_leaf} {feature_index} {threshold:.8f} {left} {right} {label}\n"
            )


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Train DPI decision tree classifier")
    parser.add_argument("--csv", required=True, help="Training CSV path")
    parser.add_argument(
        "--out",
        default="models/traffic_dt_model.txt",
        help="Output model path",
    )
    parser.add_argument("--max-depth", type=int, default=8, help="Tree max depth")
    parser.add_argument("--min-samples-leaf", type=int, default=5, help="Min samples per leaf")
    args = parser.parse_args()

    x, y = load_dataset(Path(args.csv))

    clf = DecisionTreeClassifier(
        criterion="gini",
        max_depth=args.max_depth,
        min_samples_leaf=args.min_samples_leaf,
        random_state=42,
    )
    clf.fit(x, y)

    out = Path(args.out)
    out.parent.mkdir(parents=True, exist_ok=True)
    export_model(clf, out)
    print(f"Saved model to {out}")
