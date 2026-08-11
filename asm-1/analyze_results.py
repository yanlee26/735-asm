#!/usr/bin/env python3
"""
159.735 Assignment 1: Analyze benchmark_results.csv

Reads the CSV produced by run_benchmark.sh, averages repeated runs per
numproc, computes the measured speedup S(p) = T_1 / T_p, estimates the
serial fraction f from the p=1 and largest-p data points, and prints a
comparison table against the theoretical Amdahl's Law prediction:

    S(p) = 1 / (f + (1-f)/p)

If matplotlib is available, also saves a plot (speedup_plot.png)
comparing measured vs theoretical speedup curves.

Usage:
    python3 analyze_results.py [benchmark_results.csv]
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


def estimate_f(avg_times):
    """
    Estimate the serial fraction f using the two extreme data points
    (p=1 and the largest p tested), by solving:
        S(p) = T_1/T_p = 1 / (f + (1-f)/p)
    for f.
    """
    p_values = sorted(avg_times.keys())
    p1 = p_values[0]
    p_max = p_values[-1]

    if p1 != 1:
        print(f"Warning: no p=1 baseline found; using p={p1} as baseline instead.")

    T1 = avg_times[p1]
    Tp = avg_times[p_max]
    S = T1 / Tp

    # Solve S = 1 / (f + (1-f)/p)  =>  f = (p - S) / (S * (p - 1))
    if p_max == 1:
        return None
    f_est = (p_max - S) / (S * (p_max - 1))
    return f_est


def amdahl_speedup(f, p):
    return 1.0 / (f + (1.0 - f) / p)


def main():
    filename = sys.argv[1] if len(sys.argv) > 1 else "benchmark_results.csv"

    avg_times = load_and_average(filename)
    p_values = sorted(avg_times.keys())
    baseline_p = p_values[0]
    T_baseline = avg_times[baseline_p]

    f_est = estimate_f(avg_times)

    print(f"{'numproc':>8} {'avg_total_time(s)':>18} {'measured_S(p)':>15} " f"{'amdahl_S(p)':>13} {'efficiency':>11}")

    for p in p_values:
        measured_S = T_baseline / avg_times[p]
        amdahl_S = amdahl_speedup(f_est, p) if f_est is not None else float("nan")
        efficiency = measured_S / p
        print(f"{p:>8} {avg_times[p]:>18.6f} {measured_S:>15.3f} " f"{amdahl_S:>13.3f} {efficiency:>11.3f}")

    if f_est is not None:
        print(f"\nEstimated serial fraction f (from p={baseline_p} and "
              f"p={p_values[-1]} data points): f ≈ {f_est:.5f}")
        print(f"Theoretical max speedup (p→∞): S_inf = 1/f ≈ {1/f_est:.2f}")

    # Optional plot
    try:
        import matplotlib.pyplot as plt

        measured = [T_baseline / avg_times[p] for p in p_values]
        theoretical = [amdahl_speedup(f_est, p) for p in p_values] if f_est else None
        ideal = [p for p in p_values]

        plt.figure(figsize=(7, 5))
        plt.plot(p_values, measured, "o-", label="Measured speedup")
        if theoretical:
            plt.plot(p_values, theoretical, "s--", label=f"Amdahl's Law (f≈{f_est:.4f})")
        plt.plot(p_values, ideal, "k:", label="Ideal linear speedup")
        plt.xlabel("Number of processes (p)")
        plt.ylabel("Speedup S(p)")
        plt.title("Measured vs Theoretical Speedup (Amdahl's Law)")
        plt.legend()
        plt.grid(True, alpha=0.3)
        plt.tight_layout()
        plt.savefig("speedup_plot.png", dpi=150)
        print("\nPlot saved to speedup_plot.png")
    except ImportError:
        print("\n(matplotlib not available -- skipping plot; "
              "table above can be pasted into Excel/report directly)")


if __name__ == "__main__":
    main()
