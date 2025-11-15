#!/usr/bin/env python3
"""Analyze sorted runs and difference distributions from an input file.

This script reads a workload file with one entry per line. If a line contains
more than one comma-separated value, the line is discarded (as requested).

It reports:
- total number of monotonic runs (default: strictly increasing)
- distribution of run lengths
- distribution of differences between consecutive values

Usage: `python3 analyze_runs_from_input.py -f <file>`
"""
from __future__ import annotations

import argparse
from collections import Counter
from typing import List, Tuple


def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(description="Analyze runs and diffs from a file")
    p.add_argument("-f", "--file_name", required=True, help="Path to workload file")
    p.add_argument(
        "--allow-equal",
        action="store_true",
        help="Treat equal consecutive values as part of a run (non-decreasing).",
    )
    return p.parse_args()


def read_values(file_path: str) -> Tuple[List[int], int, int]:
    """Read integers from file. If a line contains multiple comma-separated
    values, keep the first value and discard the rest of the line.

    Returns: (values, truncated_multi_value_lines, invalid_lines)
    """
    vals: List[int] = []
    truncated = 0
    invalid = 0

    with open(file_path, "r") as f:
        for lineno, row in enumerate(f, start=1):
            s = row.strip()
            if s == "":
                continue
            parts = [p.strip() for p in s.split(",") if p.strip() != ""]
            if not parts:
                continue
            # If multiple values on a line, keep the first and count it as truncated
            first = parts[0]
            if len(parts) > 1:
                truncated += 1
            try:
                vals.append(int(first))
            except ValueError:
                invalid += 1
                continue

    return vals, truncated, invalid


def compute_runs(vals: List[int], allow_equal: bool = False) -> List[int]:
    """Return list of run lengths. A run is a sequence of consecutive values
    that are strictly increasing (or non-decreasing if allow_equal=True).
    """
    if not vals:
        return []

    runs: List[int] = []
    run_len = 1
    for a, b in zip(vals, vals[1:]):
        if (allow_equal and b >= a) or (not allow_equal and b > a):
            run_len += 1
        else:
            runs.append(run_len)
            run_len = 1
    runs.append(run_len)
    return runs


def compute_diffs(vals: List[int]) -> List[int]:
    return [b - a for a, b in zip(vals, vals[1:])]


def print_distribution_counter(name: str, counter: Counter) -> None:
    if not counter:
        print(f"{name}: (empty)")
        return
    print(f"{name} (value:count)")
    for k in sorted(counter.keys()):
        print(f"  {k}: {counter[k]}")


def main() -> None:
    args = parse_args()
    vals, truncated, invalid = read_values(args.file_name)

    print(f"Read {len(vals)} values from '{args.file_name}'")
    if truncated:
        print(
            f"Truncated {truncated} line(s) containing multiple comma-separated values (kept first value)"
        )
    if invalid:
        print(f"Skipped {invalid} invalid (non-integer) line(s)")
    print()

    if not vals:
        print("No valid values to analyze.")
        raise SystemExit(1)

    runs = compute_runs(vals, allow_equal=args.allow_equal)
    diffs = compute_diffs(vals)

    from statistics import mean, median

    print(f"Total runs: {len(runs)}")
    if runs:
        print(
            f"Run lengths: min={min(runs)}, max={max(runs)}, mean={mean(runs):.2f}, median={median(runs):.2f}"
        )
        print_distribution_counter("Run length distribution", Counter(runs))

    print()
    print(f"Total diffs computed: {len(diffs)}")
    if diffs:
        print(
            f"Diffs: min={min(diffs)}, max={max(diffs)}, mean={mean(diffs):.2f}, median={median(diffs):.2f}"
        )
        print_distribution_counter("Diff distribution", Counter(diffs))


if __name__ == "__main__":
    main()
