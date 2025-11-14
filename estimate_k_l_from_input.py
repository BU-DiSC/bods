import numpy as np
import argparse
import os
from typing import List, Dict, Any


def parse_args() -> argparse.Namespace:
    """Parse command line arguments."""
    parser = argparse.ArgumentParser(
        description='This script takes in the workload file (one integer per line) and prints a small workload description.'
    )
    parser.add_argument('-f', '--file_name', required=True, help='The relative path for the workload file')
    return parser.parse_args()


def read_values(file_path: str) -> List[int]:
    """Read newline-separated integers from file and return as a list.

    Raises:
        ValueError: if the file contains a non-integer line or is empty.
    """
    vals: List[int] = []
    with open(file_path, 'r') as f:
        for row in f:
            s = row.strip()
            if s == '':
                continue
            try:
                vals.append(int(s))
            except ValueError:
                raise ValueError(f"Invalid integer in file: {row!r}")

    if not vals:
        raise ValueError("Input file contains no integers")

    return vals


def analyze_values(vals: List[int]) -> Dict[str, Any]:
    """Analyze the list of values and return metrics.

    Returns a dict with keys: vals, N, I, K, L, fixed_window, max_window
    """
    N = len(vals)
    sorted_vals = np.sort(vals)
    I = sorted_vals[0]
    fixed_window = True
    max_window = 0

    if N > 1:
        prev_window = int(sorted_vals[1] - sorted_vals[0])
        for i in range(1, N - 1):
            current_window = int(sorted_vals[i + 1] - sorted_vals[i])
            if current_window != prev_window:
                fixed_window = False
            max_window = max(max_window, current_window)
            prev_window = current_window

    arr_L: List[int] = []
    for i in range(N):
        # Note: this follows the original behavior using vals.index(...)
        # which returns the first occurrence if duplicates exist.
        arr_L.append(int(abs(i - vals.index(int(sorted_vals[i])))))

    L = max(arr_L) if arr_L else 0
    K = sum(1 for a in arr_L if a != 0)

    return {
        'vals': vals,
        'N': N,
        'I': int(I),
        'K': int(K),
        'L': int(L),
        'fixed_window': bool(fixed_window),
        'max_window': int(max_window),
    }


def print_results(metrics: Dict[str, Any]) -> None:
    vals = metrics['vals']
    print(vals, end="\n\n")
    print(f"N = {metrics['N']}")
    print(f"Start Index = {metrics['I']}")
    print(f"K = {metrics['K']}")
    print(f"L = {metrics['L']}")
    print(f"Fixed Window: {metrics['fixed_window']}")
    if metrics['fixed_window']:
        print(f"Window Size: {metrics['max_window']}")
    else:
        print(f"Max Window Size: {metrics['max_window']}")


def main() -> None:
    args = parse_args()
    try:
        vals = read_values(args.file_name)
    except Exception as e:
        print(f"Error: {e}")
        print()
        print("Use --help for usage information.")
        raise SystemExit(1)

    metrics = analyze_values(vals)
    print_results(metrics)


if __name__ == '__main__':
    main()