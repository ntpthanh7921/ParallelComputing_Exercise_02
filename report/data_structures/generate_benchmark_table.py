#!/usr/bin/env python3

import json
import argparse
import re
import os
from collections import defaultdict


def parse_benchmark_name_for_table(name):
    """
    Parses the Google Benchmark name string specifically for table generation.

    Args:
        name (str): The full benchmark name.

    Returns:
        tuple: (implementation_name, threads, aggregate_type) or (None, None, None).
    """
    # Regex to capture implementation and threads for aggregate results
    match = re.search(
        r"(?:SetBenchmarkFixture<(\w+)>|BM_(\w+))/.*?/threads:(\d+)(?:_(\w+))?$", name
    )

    if match:
        implementation = match.group(1) if match.group(1) else match.group(2)
        # Remap names for table clarity
        if implementation == "SequentialSet":
            implementation = "Sequential"
        if implementation == "CoarseLockSet":
            implementation = "Coarse Lock"
        if implementation == "FineLockSet":
            implementation = "Fine Lock"
        if implementation == "StdSet":
            implementation = "std::set"
        if implementation == "StdUnorderedSet":
            implementation = "std::unordered_set"
        if implementation == "CustomFineLockPQ":
            implementation = "Fine Lock PQ"
        if implementation == "StdPriorityQueue":
            implementation = "std::priority_queue"

        threads = int(match.group(3))
        aggregate = match.group(4) if match.group(4) else None
        return implementation, threads, aggregate
    else:
        # Check for simple pattern without SetBenchmarkFixture/BM prefix if needed
        match_simple = re.search(r"(\w+)/.*?/threads:(\d+)(?:_(\w+))?$", name)
        if match_simple:
            implementation = match_simple.group(1)
            # Remap names
            if implementation == "SequentialSet":
                implementation = "Sequential"
            if implementation == "CoarseLockSet":
                implementation = "Coarse Lock"
            if implementation == "FineLockSet":
                implementation = "Fine Lock"
            if implementation == "StdSet":
                implementation = "std::set"
            if implementation == "StdUnorderedSet":
                implementation = "std::unordered_set"
            if implementation == "CustomFineLockPQ":
                implementation = "Fine Lock PQ"
            if implementation == "StdPriorityQueue":
                implementation = "std::priority_queue"
            threads = int(match_simple.group(2))
            aggregate = match_simple.group(3) if match_simple.group(3) else None
            return implementation, threads, aggregate
        else:
            # print(f"Warning: Could not parse benchmark name format for table: {name}")
            return None, None, None


def process_data_for_table(json_data):
    """
    Processes benchmark JSON to extract real_time mean, stddev, and unit.

    Args:
        json_data (dict): Loaded JSON data.

    Returns:
        dict: {impl_name: {threads: {'mean': v, 'stddev': v, 'unit': u}}}
    """
    metric = "real_time"
    processed_data = defaultdict(lambda: defaultdict(dict))

    for benchmark in json_data.get("benchmarks", []):
        name = benchmark.get("name")
        run_type = benchmark.get("run_type")

        if run_type != "aggregate":
            continue

        impl_name, threads, aggregate_type = parse_benchmark_name_for_table(name)

        if not impl_name or not threads or aggregate_type not in ["mean", "stddev"]:
            continue

        value = benchmark.get(metric)
        unit = benchmark.get("time_unit")
        if value is None or unit is None:
            continue

        processed_data[impl_name][threads][aggregate_type] = value
        processed_data[impl_name][threads]["unit"] = unit

    # Clean up and ensure stddev exists
    cleaned_data = defaultdict(lambda: defaultdict(dict))
    for impl, thread_data in processed_data.items():
        for thread_count, metrics in thread_data.items():
            if "mean" in metrics and "unit" in metrics:
                stddev_val = metrics.get("stddev", 0.0)
                cleaned_data[impl][thread_count] = {
                    "mean": metrics["mean"],
                    "stddev": stddev_val,
                    "unit": metrics["unit"],
                }
    return cleaned_data


def generate_markdown_table(data, title):
    """
    Generates and prints a Markdown table summarizing the benchmark results.

    Args:
        data (dict): Processed benchmark data.
        title (str): The title for the table section.
    """
    if not data:
        print("No data available to generate table.")
        return

    # Determine all unique implementations and thread counts
    implementations = sorted(
        data.keys(),
        key=lambda name: (
            0
            if not name.startswith("std::") and name != "Sequential"
            else (1 if name == "Sequential" else 2),
            name,
        ),
    )
    all_thread_counts = sorted(list(set(tc for impl in data for tc in data[impl])))

    # --- Create Header ---
    header_cols = [f"{tc} Thread{'s' if tc > 1 else ''}" for tc in all_thread_counts]
    header = "| Implementation        | " + " | ".join(header_cols) + " |"
    # Fixed line: removed inner brackets from the generator expression
    separator = (
        "|-----------------------|-"
        + "-|".join("-" * len(col) for col in header_cols)
        + "-|"
    )

    # --- Print Title and Header ---
    if title:
        print(f"### {title}\n")
    print(header)
    print(separator)

    # --- Create Rows ---
    for impl_name in implementations:
        row = f"| {impl_name:<21} |"  # Pad implementation name
        for i, tc in enumerate(all_thread_counts):
            col_width = len(header_cols[i])  # Get width for padding
            if tc in data[impl_name]:
                metrics = data[impl_name][tc]
                mean_val = metrics["mean"]
                stddev_val = metrics["stddev"]
                unit = metrics["unit"]
                cell_content = f"{mean_val:.2f} ± {stddev_val:.2f} {unit}"
            else:
                cell_content = "N/A"
            row += f" {cell_content:<{col_width}} |"  # Pad cell content
        print(row)
    print("\n")  # Add a newline after the table


def main():
    parser = argparse.ArgumentParser(
        description="Generate Markdown tables from Google Benchmark JSON results."
    )
    parser.add_argument(
        "--json-file", required=True, help="Path to the input benchmark JSON file."
    )
    parser.add_argument("--title", help="Optional title for the table.")

    args = parser.parse_args()

    # --- Input Validation ---
    if not os.path.exists(args.json_file):
        print(f"Error: Input JSON file not found: {args.json_file}")
        return

    # --- Load Data ---
    try:
        with open(args.json_file, "r") as f:
            benchmark_data = json.load(f)
    except json.JSONDecodeError as e:
        print(f"Error decoding JSON file {args.json_file}: {e}")
        return
    except Exception as e:
        print(f"Error reading file {args.json_file}: {e}")
        return

    # --- Process Data ---
    processed_data = process_data_for_table(benchmark_data)
    if not processed_data:
        print("No suitable benchmark data found or processed.")
        return

    # --- Generate Table ---
    table_title = (
        args.title
        if args.title
        else f"Benchmark Execution Time Summary ({os.path.basename(args.json_file)})"
    )
    generate_markdown_table(processed_data, table_title)


if __name__ == "__main__":
    main()
