#!/usr/bin/env python3
"""Analyze sorted runs and difference distributions from an input file.

Usage:
    # Text input (one integer per line)
    python3 analyze_runs_from_input.py -f workloads/createdata.txt [--allow-equal] [--top-k 10]

    # Binary input (unsigned ints; autodetect u32 vs u64; assumes little-endian)
    python3 analyze_runs_from_input.py -f workloads/data.bin --bin [--width {32,64}] [--allow-equal] [--top-k 10]

Features:
- Keeps the first comma-separated value on lines containing multiple entries.
- Reports number of monotonic runs and distribution of their lengths.
- Reports distribution of differences between consecutive values.
- Use `--top-k N` to show only the top-N most frequent run lengths and diffs.
"""
from __future__ import annotations

import argparse
import struct
import os, struct, csv
from collections import Counter
from statistics import mean, median
from typing import List, Tuple
from itertools import islice


def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(description="Analyze runs and diffs from a file")
    p.add_argument("-f", "--file_name", required=True, help="Path to workload file")
    p.add_argument(
        "--allow-equal",
        action="store_true",
        help="Treat equal consecutive values as part of a run (non-decreasing).",
    )
    p.add_argument(
        "--top-k",
        type=int,
        default=0,
        help="Show only the top-K most frequent run lengths and diffs (0 = show all)",
    )
    p.add_argument(
        "--bin",
        action="store_true",
        help="Treat input as binary of unsigned ints (u32 or u64). If width is not given, autodetect.",
    )
    p.add_argument(
        "--width",
        choices=["32", "64"],
        default=None,
        help="Force integer width for --bin input (u32 or u64). If omitted, autodetect.",
    )
    return p.parse_args()


def read_values(file_path: str) -> Tuple[List[int], int, int]:
    """Read integers from file. Keep the first value on comma-separated lines.

    Returns: (values, truncated_count, invalid_count)
    """
    vals: List[int] = []
    truncated = 0
    invalid = 0

    with open(file_path, "r") as f:
        for row in f:
            s = row.strip()
            if s == "":
                continue
            parts = [p.strip() for p in s.split(",") if p.strip() != ""]
            if not parts:
                continue
            first = parts[0]
            if len(parts) > 1:
                truncated += 1
            try:
                vals.append(int(first))
            except ValueError:
                invalid += 1
                continue

    return vals, truncated, invalid


def _iter_uints_le(file_path: str, width_bits: int, chunk_bytes: int = 1 << 20):
    """Stream unsigned little-endian ints (32 or 64 bit) from a binary file."""
    fmt = "<I" if width_bits == 32 else "<Q"
    size = 4 if width_bits == 32 else 8
    with open(file_path, "rb") as f:
        while True:
            buf = f.read((chunk_bytes // size) * size)
            if not buf:
                break
            for (x,) in struct.iter_unpack(fmt, buf):
                yield x


def read_values_bin_with_width(file_path: str, width_bits: int):
    """Read unsigned little-endian integers of given width from a binary file."""
    return list(_iter_uints_le(file_path, width_bits)), 0, 0  # truncated, invalid not applicable


def _sample_first(iterable, n):
    from itertools import islice
    return list(islice(iterable, n))


def _score_sequence(vals, allow_equal: bool):
    """Score sequence 'near-sortedness' via mean run length and negative diff fraction."""
    if len(vals) < 3:
        return {"mean_run": 1.0, "neg_frac": 1.0, "score": 0.0}
    diffs = [vals[i+1] - vals[i] for i in range(len(vals)-1)]
    runs = []
    rl = 1
    for d in diffs:
        ok = (d >= 0) if allow_equal else (d > 0)
        if ok:
            rl += 1
        else:
            runs.append(rl)
            rl = 1
    runs.append(rl)
    mean_run = sum(runs)/len(runs)
    neg_frac = sum(1 for d in diffs if d < 0) / len(diffs)
    score = mean_run - 5.0 * neg_frac  # prefer longer runs, fewer negatives
    return {"mean_run": mean_run, "neg_frac": neg_frac, "score": score}


def autodetect_width(file_path: str, allow_equal: bool) -> int:
    """Autodetect u32 vs u64 by file-size divisibility and a small sample score."""
    size = os.path.getsize(file_path)
    can32 = (size % 4 == 0)
    can64 = (size % 8 == 0)
    if can32 and not can64:
        return 32
    if can64 and not can32:
        return 64
    if not can32 and not can64:
        raise ValueError(f"File size {size} not divisible by 4 or 8; not a clean u32/u64 stream.")

    # Ambiguous: try both on samples (assumes little-endian).
    SAMPLE_N = 200_000
    s32_vals = _sample_first(_iter_uints_le(file_path, 32), SAMPLE_N)
    s64_vals = _sample_first(_iter_uints_le(file_path, 64), SAMPLE_N // 2)
    s32 = _score_sequence(s32_vals, allow_equal)
    s64 = _score_sequence(s64_vals, allow_equal)
    if s64["score"] > s32["score"]:
        return 64
    if s32["score"] > s64["score"]:
        return 32
    return 64 if s64["mean_run"] >= s32["mean_run"] else 32


def write_counter_csv(counter: Counter, out_path: str, header=("value","count")):
    os.makedirs(os.path.dirname(out_path), exist_ok=True)
    with open(out_path, "w", newline="") as fh:
        w = csv.writer(fh)
        w.writerow(header)
        # stable order: most frequent first, then by value
        for k, v in sorted(counter.items(), key=lambda kv: (-kv[1], kv[0])):
            w.writerow([k, v])


def compute_runs(vals: List[int], allow_equal: bool = False) -> List[int]:
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


def print_distribution_counter(name: str, counter: Counter, top_k: int = 0) -> None:
    if not counter:
        print(f"{name}: (empty)")
        return
    print(f"{name} (value:count)")
    if top_k and top_k > 0:
        for k, v in counter.most_common(top_k):
            print(f"  {k}: {v}")
        return
    for k in sorted(counter.keys()):
        print(f"  {k}: {counter[k]}")


def main() -> None:
    args = parse_args()
    is_bin_input = args.bin or os.path.splitext(args.file_name)[1].lower() == ".bin"
    if is_bin_input:
        if args.width is None:
            width = autodetect_width(args.file_name, args.allow_equal)
            print(f"[auto] Detected binary width: u{width}")
        else:
            width = int(args.width)
            print(f"[forced] Using binary width: u{width}")
        vals, truncated, invalid = read_values_bin_with_width(args.file_name, width)
    else:
        vals, truncated, invalid = read_values(args.file_name)

    print(f"Read {len(vals)} values from '{args.file_name}'")
    if truncated:
        print(f"Truncated {truncated} line(s) containing multiple comma-separated values (kept first value)")
    if invalid:
        print(f"Skipped {invalid} invalid (non-integer) line(s)")
    print()

    if not vals:
        print("No valid values to analyze.")
        raise SystemExit(1)

    runs = compute_runs(vals, allow_equal=args.allow_equal)
    diffs = compute_diffs(vals)

    print(f"Total runs: {len(runs)}")
    if runs:
        print(
            f"Run lengths: min={min(runs)}, max={max(runs)}, mean={mean(runs):.2f}, median={median(runs):.2f}"
        )
        print_distribution_counter("Run length distribution", Counter(runs), top_k=args.top_k)

    print()
    print(f"Total diffs computed: {len(diffs)}")
    if diffs:
        print(f"Diffs: min={min(diffs)}, max={max(diffs)}, mean={mean(diffs):.2f}, median={median(diffs):.2f}")
        print_distribution_counter("Diff distribution", Counter(diffs), top_k=args.top_k)


if __name__ == "__main__":
    main()
