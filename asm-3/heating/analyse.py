#!/usr/bin/env python3
"""159.735 Assignment 3 -- turn results/scaling.csv into the tables and
figures used in the report.

    ./analyse.py

Reads  results/scaling.csv   (written by run_scaling.sh)
Writes results/tables.md     tables, ready to paste into the report
       results/*.png         figures

Every configuration was measured several times; the fastest is used
throughout, that being the reading least polluted by other processes on the
machine. The spread between the fastest and the median is reported so the
reader can judge how much to trust the numbers.
"""
import csv
import re
import statistics
import sys
from pathlib import Path

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np

# Which run to analyse. Defaults to results/, but run_scaling.sh can be told
# to write elsewhere (OUTDIR=results-hpc), so that a second machine's numbers
# sit beside the first's instead of overwriting them.
RES = Path(sys.argv[1]) if len(sys.argv) > 1 else Path("results")
if not (RES / "scaling.csv").exists():
    sys.exit(f"{RES/'scaling.csv'} not found -- run ./run_scaling.sh first "
             f"(or pass the results directory: ./analyse.py results-hpc)")

ROWS = []
with open(RES / "scaling.csv") as fh:
    for r in csv.DictReader(fh):
        for k in ("npix", "threads", "iters", "converged"):
            r[k] = int(r[k])
        r["time"] = float(r["time"])
        for k in ("busymax", "busymin", "busyavg"):
            r[k] = float(r[k]) if r[k] else float("nan")
        ROWS.append(r)


def runs(study, version, part, npix, threads):
    return [r for r in ROWS
            if r["study"] == study and r["version"] == version
            and r["part"] == part and r["npix"] == npix
            and r["threads"] == threads]


def best(study, version, part, npix, threads):
    rs = runs(study, version, part, npix, threads)
    return min(r["time"] for r in rs) if rs else None


def row_for(study, version, part, npix, threads):
    rs = runs(study, version, part, npix, threads)
    return min(rs, key=lambda r: r["time"]) if rs else None


def sizes(study):
    return sorted({r["npix"] for r in ROWS if r["study"] == study})


def threadlist(study, npix, part="static"):
    return sorted({r["threads"] for r in ROWS
                   if r["study"] == study and r["version"] == "omp"
                   and r["part"] == part and r["npix"] == npix})


def amdahl_fit(ps, ss):
    """Least squares serial fraction f in S(p) = 1 / (f + (1-f)/p)."""
    ps, ss = np.asarray(ps, float), np.asarray(ss, float)
    grid = np.linspace(1e-6, 1.0, 20001)[:, None]
    pred = 1.0 / (grid + (1.0 - grid) / ps[None, :])
    err = np.sum((pred - ss[None, :]) ** 2, axis=1)
    return float(grid[int(np.argmin(err)), 0])


def karp_flatt(p, s):
    """Experimentally determined serial fraction."""
    if p == 1 or s <= 0:
        return None
    return (1.0 / s - 1.0 / p) / (1.0 - 1.0 / p)


def fmt(x, nd=2, dash="-"):
    return dash if x is None else f"{x:.{nd}f}"


out = []
def emit(line=""):
    out.append(line)
    print(line)


machine = (RES / "machine.txt").read_text().strip() if (RES / "machine.txt").exists() else "?"

# run_scaling.sh writes a "marker : <p> <label>" line giving the thread count
# beyond which the machine stops handing out another full core -- the 4
# performance cores of the laptop, or the physical core count of an SMT
# machine. Nothing is drawn if the machine has no such boundary.
_m = re.search(r"^marker\s*:\s*(\d+)\s+(.*)$", machine, re.M)
if _m:
    CORE_MARK = (int(_m.group(1)), _m.group(2).strip())
else:
    # machine.txt from before the marker line existed: recover it from the
    # laptop's "4 performance + 6 efficiency" description.
    _m = re.search(r"(\d+)\s+performance", machine)
    CORE_MARK = (int(_m.group(1)), "performance cores") if _m else None


def mark_cores(ax, ytext):
    """Draw the core-boundary guide line, if this machine has one."""
    if not CORE_MARK:
        return
    p, label = CORE_MARK
    ax.axvline(p, color="grey", ls=":", lw=1)
    ax.text(p + 0.1, ytext, f"{p} {label}", fontsize=8, color="grey")

emit("# Scaling results\n")
emit("```")
emit(machine)
emit("```\n")

# measurement quality
spreads = []
seen = {(r["study"], r["version"], r["part"], r["npix"], r["threads"]) for r in ROWS}
for key in seen:
    ts = [r["time"] for r in runs(*key)]
    if len(ts) > 2:
        spreads.append((statistics.median(ts) - min(ts)) / min(ts))
if spreads:
    emit(f"Measurement spread across repeats (median over fastest): "
         f"median {100*statistics.median(spreads):.0f}%, "
         f"worst {100*max(spreads):.0f}%. The fastest run of each "
         f"configuration is used below.\n")

# ---------------------------------------------------------------------------
# 1. Correctness
# ---------------------------------------------------------------------------
emit("## 1. The parallel version gives the same answer\n")
emit("Every run below was taken all the way to convergence. `iterations` and "
     "`image hash` (FNV-1a over the raw pixel bytes) are compared across the "
     "sequential program and the OpenMP program at every thread count and "
     "both partitions.\n")
emit("| npix | iterations | distinct iteration counts | distinct image hashes | runs compared |")
emit("|---:|---:|---:|---:|---:|")
allgood = True
for n in sizes("converge"):
    rs = [r for r in ROWS if r["study"] == "converge" and r["npix"] == n]
    its = {r["iters"] for r in rs}
    hs = {r["hash"] for r in rs}
    allgood &= (len(its) == 1 and len(hs) == 1)
    emit(f"| {n} | {sorted(its)[0]:,} | {len(its)} | {len(hs)} | {len(rs)} |")
emit()
emit("**Every configuration needed the identical number of iterations and "
     "produced a bit-identical image.**" if allgood
     else "**MISMATCH -- see results/scaling.csv.**")
emit()

# ---------------------------------------------------------------------------
# 2. Strong scaling / Amdahl
# ---------------------------------------------------------------------------
emit("## 2. Strong scaling (Amdahl's law)\n")
emit("Fixed plate, fixed number of iterations, varying thread count. "
     "Speedup is against the *sequential program*, not against the parallel "
     "program on one thread.\n")

fits = {}
for n in sizes("strong"):
    r1 = row_for("strong", "seq", "seq", n, 1)
    t1 = best("strong", "seq", "seq", n, 1)
    emit(f"### {n}x{n}, {r1['iters']} iterations -- sequential {t1:.3f} s\n")
    emit("| threads | static t (s) | speedup | efficiency | Karp-Flatt e "
         "| busy% | dynamic t (s) | speedup |")
    emit("|---:|---:|---:|---:|---:|---:|---:|---:|")
    ps, ss = [], []
    for p in threadlist("strong", n):
        ts = best("strong", "omp", "static", n, p)
        td = best("strong", "omp", "dynamic", n, p)
        rs = row_for("strong", "omp", "static", n, p)
        s = t1 / ts
        ps.append(p); ss.append(s)
        busy = 100.0 * rs["busyavg"] / rs["time"]
        e = karp_flatt(p, s)
        emit(f"| {p} | {ts:.3f} | {s:.2f} | {s/p:.2f} | {fmt(e, 3)} "
             f"| {busy:.0f}% | {td:.3f} | {t1/td:.2f} |")
    emit()
    smax = max(ss); pbest = ps[ss.index(smax)]
    emit(f"Best static speedup **{smax:.2f}x at {pbest} threads**.")
    if smax > 1.05:
        f = amdahl_fit(ps, ss)
        fits[n] = (f, ps, ss)
        emit(f" Least squares Amdahl serial fraction **f = {f:.3f}**, "
             f"so the ceiling 1/f is about **{1.0/f:.1f}x**.")
    else:
        fits[n] = (None, ps, ss)
        emit(" No useful speedup at this size: the run is dominated by "
             "per-iteration overhead, which Amdahl's law cannot model as a "
             "serial *fraction* because the overhead is added work, not "
             "part of the sequential program.")
    emit()

# small sizes, from the convergence runs, to show the overhead-dominated end
emit("### The small end, from the runs to convergence\n")
emit("These are complete runs of the real program, so they show what a user "
     "would actually see.\n")
emit("| npix | iterations | sequential (s) | best parallel (s) | threads | speedup |")
emit("|---:|---:|---:|---:|---:|---:|")
conv_small = []
for n in sizes("converge"):
    t1 = best("converge", "seq", "seq", n, 1)
    r1 = row_for("converge", "seq", "seq", n, 1)
    cands = []
    for part in ("static", "dynamic"):
        for p in threadlist("converge", n, part):
            t = best("converge", "omp", part, n, p)
            if t:
                cands.append((t, p, part))
    tb, pb, partb = min(cands)
    conv_small.append((n, t1 / tb))
    emit(f"| {n} | {r1['iters']:,} | {t1:.3f} | {tb:.3f} ({partb}) | {pb} "
         f"| {t1/tb:.2f} |")
emit()

# figure: speedup + efficiency
fig, axes = plt.subplots(1, 2, figsize=(13, 5.2))
pmax = max(max(v[1]) for v in fits.values())
ideal = np.arange(1, pmax + 1)
ax = axes[0]
ax.plot(ideal, ideal, "k--", lw=1, label="ideal (linear)")
for n in sorted(fits):
    f, ps, ss = fits[n]
    line, = ax.plot(ps, ss, "o-", ms=4, label=f"{n}x{n}")
    if f:
        pp = np.linspace(1, pmax, 200)
        ax.plot(pp, 1.0 / (f + (1 - f) / pp), ":", lw=1.2,
                color=line.get_color())
ax.set_xlabel("threads $p$"); ax.set_ylabel("speedup  $T_1/T_p$")
ax.set_title("Strong scaling: measured (solid) vs Amdahl fit (dotted)")
ax.grid(alpha=.3); ax.legend(fontsize=9)

ax = axes[1]
ax.axhline(1.0, color="k", ls="--", lw=1, label="ideal")
mark_cores(ax, 0.05)
for n in sorted(fits):
    f, ps, ss = fits[n]
    ax.plot(ps, [s / p for p, s in zip(ps, ss)], "o-", ms=4, label=f"{n}x{n}")
ax.set_xlabel("threads $p$"); ax.set_ylabel("efficiency  $S_p/p$")
ax.set_title("Parallel efficiency"); ax.set_ylim(0, 1.15)
ax.grid(alpha=.3); ax.legend(fontsize=9)
fig.tight_layout(); fig.savefig(RES / "strong_scaling.png", dpi=130)
emit("![strong scaling](strong_scaling.png)\n")

# ---------------------------------------------------------------------------
# 3. Weak scaling / Gustafson
# ---------------------------------------------------------------------------
emit("## 3. Weak scaling (Gustafson's law)\n")
emit("The plate side grows as `400*sqrt(p)`, so the pixels per thread -- and "
     "so the arithmetic each thread does per iteration -- stay constant. "
     "Perfect scaling would keep the parallel time flat and give a scaled "
     "speedup of `p`.\n")
emit("| p | npix | pixels/thread | sequential (s) | static (s) | scaled speedup "
     "| dynamic (s) | scaled speedup |")
emit("|---:|---:|---:|---:|---:|---:|---:|---:|")
wp, wss, wsd, wtime = [], [], [], []
weak = sorted({(r["threads"], r["npix"]) for r in ROWS
               if r["study"] == "weak" and r["version"] == "omp"})
for p, n in weak:
    t1 = best("weak", "seq", "seq", n, 1)
    ts = best("weak", "omp", "static", n, p)
    td = best("weak", "omp", "dynamic", n, p)
    wp.append(p); wss.append(t1 / ts); wsd.append(t1 / td); wtime.append(ts)
    emit(f"| {p} | {n} | {n*n//p:,} | {t1:.3f} | {ts:.3f} | {t1/ts:.2f} "
         f"| {td:.3f} | {t1/td:.2f} |")
emit()
alpha = float(np.mean([(p - s) / (p - 1) for p, s in zip(wp, wss) if p > 1]))
alphad = float(np.mean([(p - s) / (p - 1) for p, s in zip(wp, wsd) if p > 1]))
emit(f"Fitting Gustafson's `S = p - a(p-1)`: **a = {alpha:.3f}** for the "
     f"static partition, **a = {alphad:.3f}** for the dynamic one.\n")

fig, axes = plt.subplots(1, 2, figsize=(13, 5.2))
ax = axes[0]
ax.plot(wp, wtime, "o-", ms=5, label="measured (static)")
ax.axhline(wtime[0], color="k", ls="--", lw=1, label="ideal (flat)")
ax.set_xlabel("threads $p$   (plate side $=400\\sqrt{p}$)")
ax.set_ylabel("time for the fixed iteration budget (s)")
ax.set_title("Weak scaling: constant work per thread")
ax.set_ylim(0, max(wtime) * 1.25); ax.grid(alpha=.3); ax.legend(fontsize=9)

ax = axes[1]
pp = np.array(wp, float)
ax.plot(pp, pp, "k--", lw=1, label="ideal")
ax.plot(wp, wss, "o-", ms=5, label="measured (static)")
ax.plot(wp, wsd, "s-", ms=5, label="measured (dynamic)")
ax.plot(pp, pp - alpha * (pp - 1), ":", lw=1.5,
        label=f"Gustafson, $a$={alpha:.3f}")
ax.set_xlabel("threads $p$"); ax.set_ylabel("scaled speedup")
ax.set_title("Gustafson scaled speedup"); ax.grid(alpha=.3); ax.legend(fontsize=9)
fig.tight_layout(); fig.savefig(RES / "weak_scaling.png", dpi=130)
emit("![weak scaling](weak_scaling.png)\n")

# ---------------------------------------------------------------------------
# 4. Overhead and load balance
# ---------------------------------------------------------------------------
emit("## 4. Where the parallel time goes\n")
n = sizes("sync")[0]
r1 = row_for("sync", "seq", "seq", n, 1)
it = r1["iters"]
emit(f"### Synchronisation cost\n")
emit(f"A {n}x{n} plate for {it:,} iterations -- small enough that the "
     "arithmetic is cheap and the run is dominated by the two barriers and "
     "the reduction.\n")
emit("The overhead below is what the parallel run costs *above perfectly "
     "divided work*, `t(p) - t_seq/p`. Subtracting `t_seq/p` rather than "
     "`t_seq` is what makes this portable between machines: where a core is "
     f"slow enough that even a {n}x{n} sweep is not free, splitting the "
     "arithmetic over `p` threads saves real time, and measuring against "
     "`t_seq` would net that saving off against the barrier cost and report "
     "a negative overhead.\n")
emit("| threads | time (s) | perfectly divided (s) | overhead (us/iteration) |")
emit("|---:|---:|---:|---:|")
sp, so = [], []
for p in threadlist("sync", n):
    t = best("sync", "omp", "static", n, p)
    ideal = r1["time"] / p
    ov = 1e6 * (t - ideal) / it
    sp.append(p); so.append(ov)
    emit(f"| {p} | {t:.4f} | {ideal:.4f} | {ov:.2f} |")
emit()

emit("### Load balance: static vs dynamic partition\n")
emit("The mean fraction of the wall clock that a thread spent doing "
     "arithmetic rather than waiting at a barrier. Low numbers mean cores "
     "are sitting idle.\n")
bign = sizes("strong")[-1]
emit(f"Plate {bign}x{bign}:\n")
emit("| threads | static busy% | static busiest/idlest | dynamic busy% "
     "| dynamic busiest/idlest |")
emit("|---:|---:|---:|---:|---:|")
bp, bs, bd = [], [], []
for p in threadlist("strong", bign):
    rs = row_for("strong", "omp", "static", bign, p)
    rd = row_for("strong", "omp", "dynamic", bign, p)
    a = 100 * rs["busyavg"] / rs["time"]; b = 100 * rd["busyavg"] / rd["time"]
    bp.append(p); bs.append(a); bd.append(b)
    emit(f"| {p} | {a:.0f}% "
         f"| {100*rs['busymax']/rs['time']:.0f}% / {100*rs['busymin']/rs['time']:.0f}% "
         f"| {b:.0f}% "
         f"| {100*rd['busymax']/rd['time']:.0f}% / {100*rd['busymin']/rd['time']:.0f}% |")
emit()

fig, axes = plt.subplots(1, 2, figsize=(13, 5.2))
ax = axes[0]
ax.plot(sp, so, "o-", ms=5)
ax.set_xlabel("threads $p$"); ax.set_ylabel("overhead per iteration (us)")
ax.set_title(f"Synchronisation cost ({n}x{n} plate)"); ax.grid(alpha=.3)
ax = axes[1]
ax.plot(bp, bs, "o-", ms=5, label="static partition")
ax.plot(bp, bd, "s-", ms=5, label="dynamic partition")
ax.axhline(100, color="k", ls="--", lw=1, label="perfect balance")
mark_cores(ax, 8)
ax.set_xlabel("threads $p$"); ax.set_ylabel("mean thread busy % of wall time")
ax.set_title(f"Load balance ({bign}x{bign} plate)")
ax.set_ylim(0, 115); ax.grid(alpha=.3); ax.legend(fontsize=9)
fig.tight_layout(); fig.savefig(RES / "overhead_balance.png", dpi=130)
emit("![overhead and balance](overhead_balance.png)\n")

(RES / "tables.md").write_text("\n".join(out) + "\n")
print(f"\nwrote {RES/'tables.md'} and the figures")
