#!/usr/bin/env python3

import json
import argparse
import matplotlib.pyplot as plt
import re
import os
from collections import defaultdict


def parse_benchmark_name(name):
    """
    Parses the Google Benchmark name string to extract implementation details.

    Args:
        name (str): The full benchmark name (e.g.,
                    "SetBenchmarkFixture<CoarseLockSet>/BM_CoarseLockOps/threads:2_mean",
                    "BM_CustomFineLockPQ/threads:4_stddev").

    Returns:
        tuple: (implementation_name, threads, aggregate_type) or (None, None, None) if parsing fails.
               aggregate_type can be 'mean', 'stddev', etc.
    """
    # Regex to capture implementation (e.g., CoarseLockSet, CustomFineLockPQ) and threads
    # Handles both SetBenchmarkFixture<...> style and direct BM_... style names
    # Looks for '/threads:(\d+)' followed by an optional aggregate suffix like '_mean'
    match = re.search(
        r"(?:SetBenchmarkFixture<(\w+)>|BM_(\w+))/.*?/threads:(\d+)(?:_(\w+))?$", name
    )

    if match:
        # Implementation name is either group 1 or group 2
        implementation = match.group(1) if match.group(1) else match.group(2)
        # Remap implementation names for clarity in the legend if needed
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
        aggregate = (
            match.group(4) if match.group(4) else None
        )  # Aggregate might be missing for raw runs
        return implementation, threads, aggregate
    else:
        # Fallback or alternative patterns if needed, otherwise return None
        # Simple pattern for names ending in /threads:N
        match_simple = re.search(
            r"(?:SetBenchmarkFixture<(\w+)>|BM_(\w+))/.*?/threads:(\d+)$", name
        )
        if match_simple:
            implementation = (
                match_simple.group(1)
                if match_simple.group(1)
                else match_simple.group(2)
            )
            # Remap implementation names
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

            threads = int(match_simple.group(3))
            return implementation, threads, None  # No aggregate type specified in name
        else:
            print(f"Warning: Could not parse benchmark name format: {name}")
            return None, None, None


def process_benchmark_data(json_data):
    """
    Processes the loaded benchmark JSON data to extract real_time metrics.

    Args:
        json_data (dict): The loaded JSON data from Google Benchmark.

    Returns:
        dict: A nested dictionary: {impl_name: {threads: {'mean': value, 'stddev': value}}}
    """
    metric = "real_time"  # Hardcode to real_time
    processed_data = defaultdict(lambda: defaultdict(dict))

    for benchmark in json_data.get("benchmarks", []):
        name = benchmark.get("name")
        run_type = benchmark.get("run_type")

        # We only care about aggregate results (mean, stddev)
        if run_type != "aggregate":
            continue

        impl_name, threads, aggregate_type = parse_benchmark_name(name)

        if not impl_name or not threads or aggregate_type not in ["mean", "stddev"]:
            continue  # Skip if parsing failed or not the aggregates we need

        # Extract the real_time value
        value = benchmark.get(metric)
        if value is None:
            print(f"Warning: Metric '{metric}' not found for benchmark: {name}")
            continue

        # Store the mean and stddev
        processed_data[impl_name][threads][aggregate_type] = value

    # Clean up entries that don't have both mean and stddev (optional, but good practice)
    cleaned_data = defaultdict(lambda: defaultdict(dict))
    for impl, thread_data in processed_data.items():
        for thread_count, metrics in thread_data.items():
            if "mean" in metrics and "stddev" in metrics:
                cleaned_data[impl][thread_count] = metrics
            # Handle cases where stddev might be zero or missing for single-thread runs
            elif "mean" in metrics and "stddev" not in metrics:
                # If stddev is missing (e.g., single rep), set it to 0 for plotting
                cleaned_data[impl][thread_count] = {
                    "mean": metrics["mean"],
                    "stddev": 0.0,
                }

    return cleaned_data


def plot_benchmarks(data, title, output_file, output_format):
    """
    Generates and saves the benchmark plot for execution time.

    Args:
        data (dict): Processed benchmark data from process_benchmark_data.
        title (str): The title for the plot.
        output_file (str): Path to save the plot image.
        output_format (str): Format for the output image ('png', 'svg').
    """
    fig, ax = plt.subplots(figsize=(12, 7))

    # Define markers and colors for consistency if needed, otherwise use defaults
    markers = ["o", "s", "^", "D", "v", "p", "*", "X"]
    marker_idx = 0

    # Sort implementations for consistent legend order
    # Custom sort order: Put baselines last
    def sort_key(impl_name):
        if impl_name.startswith("std::"):
            return 2
        if impl_name == "Sequential":
            return 1
        return 0  # Concurrent implementations first

    sorted_implementations = sorted(data.keys(), key=sort_key)

    all_thread_counts = set()
    for impl_name in sorted_implementations:
        thread_data = data[impl_name]
        if not thread_data:
            continue

        # Sort points by thread count for line plotting
        sorted_threads = sorted(thread_data.keys())
        all_thread_counts.update(sorted_threads)  # Collect all thread counts used

        means = [thread_data[t]["mean"] for t in sorted_threads]
        stddevs = [thread_data[t]["stddev"] for t in sorted_threads]

        # Use errorbar to plot line, mean points, and standard deviation
        ax.errorbar(
            sorted_threads,
            means,
            yerr=stddevs,
            label=impl_name,
            marker=markers[marker_idx % len(markers)],
            capsize=5,  # Size of the error bar caps
            linestyle="-",  # Line style
            linewidth=2,  # Line width
            markersize=8,
        )  # Marker size
        marker_idx += 1

    # --- Plot Configuration ---
    ax.set_xlabel("Number of Threads")
    # Ensure all used thread counts appear as ticks
    ax.set_xticks(sorted(list(all_thread_counts)))

    ax.set_ylabel("Execution Time (ms)")
    # Consider log scale if times vary drastically - uncomment if needed
    # ax.set_yscale('log')
    # Optional: Format y-axis ticks if needed (e.g., for log scale)
    # ax.yaxis.set_major_formatter(mticker.ScalarFormatter())

    ax.set_title(title if title else "Benchmark Comparison (Execution Time)")
    ax.legend(loc="best", fontsize="small")  # Adjust legend location and size
    ax.grid(True, which="both", linestyle="--", linewidth=0.5)  # Add grid lines

    plt.tight_layout()  # Adjust layout to prevent labels overlapping

    # --- Save Plot ---
    try:
        # Ensure output directory exists
        output_dir = os.path.dirname(output_file)
        if output_dir and not os.path.exists(output_dir):
            os.makedirs(output_dir)
            print(f"Created output directory: {output_dir}")

        plt.savefig(output_file, format=output_format, dpi=150, bbox_inches="tight")
        print(f"Plot saved to {output_file} (format: {output_format})")
    except Exception as e:
        print(f"Error saving plot to {output_file}: {e}")

    plt.close(fig)  # Close the figure to free memory


def main():
    parser = argparse.ArgumentParser(
        description="Generate execution time plots from Google Benchmark JSON results."
    )
    parser.add_argument(
        "--json-file", required=True, help="Path to the input benchmark JSON file."
    )
    parser.add_argument(
        "--output-file", required=True, help="Path to save the output plot image."
    )
    parser.add_argument(
        "--output-format",
        choices=["png", "svg"],
        default="png",
        help="Output image format (png or svg).",
    )
    # Removed the --metric argument
    parser.add_argument("--title", help="Optional title for the plot.")

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
    # process_benchmark_data now implicitly uses 'real_time'
    processed_data = process_benchmark_data(benchmark_data)
    if not processed_data:
        print("No suitable benchmark data found or processed.")
        return

    # --- Generate Plot ---
    # plot_benchmarks no longer needs the metric argument
    plot_benchmarks(processed_data, args.title, args.output_file, args.output_format)


if __name__ == "__main__":
    main()
