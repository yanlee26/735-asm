#!/usr/bin/env python3
"""
159.735 Assignment 2: Analyze gustafson_results.csv

Reads the CSV produced by run_benchmark_gustafson.sh, averages repeated
runs per numproc, and computes the "scaled speedup" used to test
Gustafson's Law:

    S_scaled(p) = p * T(1) / T(p)

where T(1) is the baseline time for the p=1 (single work-unit) run, and
T(p) is the time for the SCALED problem (N = p * work_per_proc) on p
processes. If the program scales perfectly (Gustafson's ideal), T(p)
should stay roughly constant as p grows, giving S_scaled(p) = p (linear).

Also estimates the "serial fraction observed in the scaled run" (alpha)
using the Gustafson-Barsis relation:

    S_scaled(p) = p - alpha*(p - 1)   =>   alpha = (p - S_scaled(p)) / (p - 1)

Usage:
    python3 analyze_gustafson.py [gustafson_results.csv]
"""

import csv
import sys
from collections import defaultdict


def load_and_average(filename):
    totals = defaultdict(list)
    with open(filename, newline="") as f:
        reader = csv.DictReader(f)
        for row in reader:
            p = int(row["numproc"])
            t = float(row["total_time"])
            totals[p].append(t)
    avg = {p: sum(times) / len(times) for p, times in totals.items()}
    return dict(sorted(avg.items()))


def main():
    filename = sys.argv[1] if len(sys.argv) > 1 else "gustafson_results.csv"
    avg_times = load_and_average(filename)
    p_values = sorted(avg_times.keys())

    if 1 not in avg_times:
        print("Warning: no p=1 baseline found in the data; "
              "scaled speedup cannot be computed relative to a single-process run.")
        baseline_p = p_values[0]
    else:
        baseline_p = 1

    T1 = avg_times[baseline_p]

    print(f"{'numproc':>8} {'N (scaled)':>12} {'avg_total_time(s)':>18} "
          f"{'scaled_speedup':>15} {'alpha(serial%)':>15}")

    for p in p_values:
        Tp = avg_times[p]
        # Scaled speedup: how many times more work got done in roughly
        # the same time, relative to the p=1 baseline
        S_scaled = p * T1 / Tp
        if p == 1:
            alpha = 0.0
        else:
            alpha = (p - S_scaled) / (p - 1)
        print(f"{p:>8} {'N/A':>12} {Tp:>18.6f} {S_scaled:>15.3f} {alpha*100:>14.2f}%")

    print(f"\nBaseline T({baseline_p}) = {T1:.6f} s")
    print("\nInterpretation:")
    print("  - If total_time stays roughly constant as numproc increases,")
    print("    scaled_speedup should be close to numproc (linear) -- this")
    print("    is the ideal Gustafson's Law behaviour.")
    print("  - A rising alpha (serial %) with p indicates a growing")
    print("    overhead (e.g. Alltoallv or Gatherv cost) that Gustafson's")
    print("    simple fixed-alpha model does not fully capture.")

    # Optional plot
    try:
        import matplotlib.pyplot as plt

        scaled_speedups = [p * T1 / avg_times[p] for p in p_values]
        ideal = list(p_values)

        plt.figure(figsize=(7, 5))
        plt.plot(p_values, scaled_speedups, "o-", label="Measured scaled speedup")
        plt.plot(p_values, ideal, "k:", label="Ideal linear (Gustafson)")
        plt.xlabel("Number of processes (p)")
        plt.ylabel("Scaled speedup")
        plt.title("Gustafson's Law: Scaled Speedup vs Ideal")
        plt.legend()
        plt.grid(True, alpha=0.3)
        plt.tight_layout()
        plt.savefig("gustafson_plot.png", dpi=150)
        print("\nPlot saved to gustafson_plot.png")
    except ImportError:
        print("\n(matplotlib not available -- skipping plot; "
              "table above can be pasted into Excel/report directly)")


if __name__ == "__main__":
    main()
