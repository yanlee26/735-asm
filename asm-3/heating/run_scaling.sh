#!/bin/bash
#
# 159.735 Assignment 3 -- scaling measurements for the report.
#
#   ./run_scaling.sh
#
# Writes results/scaling.csv, one row per run.
#
# Measurement notes
# -----------------
# This is a laptop with other applications running on it, so single runs are
# noisy. Two things are done about that:
#
#   * every configuration is measured REPS times and the analysis takes the
#     fastest, which is the reading least polluted by other processes;
#   * repeats are interleaved (rep is the outer loop) so a burst of activity
#     from something else spreads itself over many configurations instead of
#     ruining one of them.
#
# The script also re-runs itself under caffeinate. Without that the machine
# sleeps mid-benchmark, and a run that straddles a system sleep reports a
# meaningless time.
#
# Studies:
#
#   converge  the real program, run to convergence. Confirms the sequential
#             and parallel versions need the same number of iterations and
#             produce the same image.
#
#   strong    fixed problem, fixed iteration budget, varying thread count.
#             The data for Amdahl's law. The budget is only there to keep
#             each run short; the work per iteration is identical for every
#             thread count, so the speedups are unaffected by it.
#
#   weak      the plate grows as sqrt(p) so the pixels per thread stay
#             constant. The data for Gustafson's law.
#
#   sync      a plate small enough that the arithmetic is nearly free, so
#             what is left is the per-iteration synchronisation cost.
#
set -u

# Keep the machine awake for the duration; a system sleep in the middle of a
# run makes nonsense of the timings.
if [ -z "${HEAT_AWAKE:-}" ] && command -v caffeinate >/dev/null 2>&1; then
  HEAT_AWAKE=1 exec caffeinate -dimsu "$0" "$@"
fi

REPS=${REPS:-5}
OUT=results/scaling.csv
mkdir -p results

echo "study,version,part,npix,threads,iters,converged,time,busymax,busymin,busyavg,hash,rep,load" > "$OUT"

# Run one configuration and append its RESULT line to the CSV as one row.
# The 1 minute load average is recorded with each row so that obviously
# contaminated measurements can be spotted afterwards.
record() {
  local study=$1 rep=$2; shift 2
  local load
  load=$(sysctl -n vm.loadavg | awk '{print $2}')
  "$@" 2>/dev/null | grep '^RESULT' | awk -v study="$study" -v rep="$rep" -v load="$load" '
    {
      for (i = 2; i <= NF; ++i) { split($i, a, "="); m[a[1]] = a[2] }
      printf "%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\n",
        study, m["version"], m["part"], m["npix"], m["threads"], m["iters"],
        m["converged"], m["time"], m["busymax"], m["busymin"], m["busyavg"],
        m["hash"], rep, load
    }' >> "$OUT"
  printf '.'
}

{
  echo "machine : $(sysctl -n machdep.cpu.brand_string), $(sysctl -n hw.ncpu) cores ($(sysctl -n hw.perflevel0.physicalcpu) performance + $(sysctl -n hw.perflevel1.physicalcpu) efficiency)"
  echo "memory  : $(( $(sysctl -n hw.memsize) / 1073741824 )) GB"
  echo "os      : macOS $(sw_vers -productVersion) ($(uname -m))"
  echo "compiler: $(g++ --version | head -1)"
  echo "repeats : $REPS (analysis uses the fastest)"
  echo "load at start: $(sysctl -n vm.loadavg)"
} | tee results/machine.txt

# --------------------------------------------------------------------------
# 1. Runs to convergence: is the parallel answer the same?
# --------------------------------------------------------------------------
echo
echo "[converge] full runs to convergence"
for n in 100 200 400; do
  for rep in 1 2; do
    printf "  n=%-5s rep %s " "$n" "$rep"
    record converge $rep ./heat "$n" -o none
    for p in 1 2 3 4 6 8 10; do
      record converge $rep ./heat_omp "$n" -p "$p" -o none
      record converge $rep ./heat_omp "$n" -p "$p" -d -o none
    done
    echo
  done
done
# One big one, for the correctness claim at a size worth calling large.
printf "  n=800  (single pass) "
record converge 1 ./heat 800 -o none
record converge 1 ./heat_omp 800 -p 10 -o none
record converge 1 ./heat_omp 800 -p 10 -d -o none
echo

# --------------------------------------------------------------------------
# 2. Strong scaling. Iteration budgets chosen so a sequential run takes about
#    a second, which keeps each measurement short enough to dodge most of the
#    interference from other processes.
# --------------------------------------------------------------------------
echo
echo "[strong] fixed problem, fixed iteration budget, varying threads"
strong_iters() {
  case $1 in
    400)  echo 10000 ;;
    800)  echo  2500 ;;
    1600) echo   620 ;;
    3200) echo   155 ;;
  esac
}
for rep in $(seq 1 $REPS); do
  for n in 400 800 1600 3200; do
    it=$(strong_iters "$n")
    printf "  rep %s  n=%-5s (%s iters) " "$rep" "$n" "$it"
    record strong $rep ./heat "$n" -o none -i "$it"
    for p in 1 2 3 4 5 6 7 8 9 10; do
      record strong $rep ./heat_omp "$n" -p "$p" -o none -i "$it"
      record strong $rep ./heat_omp "$n" -p "$p" -d -o none -i "$it"
    done
    echo
  done
done

# --------------------------------------------------------------------------
# 3. Weak scaling: npix = 400*sqrt(p), so pixels per thread stay constant.
# --------------------------------------------------------------------------
echo
echo "[weak] plate grows as sqrt(p), work per thread held constant"
WEAK_ITERS=1500
for rep in $(seq 1 $REPS); do
  printf "  rep %s " "$rep"
  for pn in "1 400" "2 566" "3 693" "4 800" "6 980" "8 1131" "10 1265"; do
    set -- $pn; p=$1; n=$2
    record weak $rep ./heat "$n" -o none -i $WEAK_ITERS
    record weak $rep ./heat_omp "$n" -p "$p" -o none -i $WEAK_ITERS
    record weak $rep ./heat_omp "$n" -p "$p" -d -o none -i $WEAK_ITERS
  done
  echo
done

# --------------------------------------------------------------------------
# 4. Synchronisation cost on its own.
# --------------------------------------------------------------------------
echo
echo "[sync] per-iteration synchronisation cost"
for rep in $(seq 1 $REPS); do
  printf "  rep %s " "$rep"
  record sync $rep ./heat 64 -o none -i 20000
  for p in 1 2 3 4 5 6 7 8 9 10; do
    record sync $rep ./heat_omp 64 -p "$p" -o none -i 20000
  done
  echo
done

echo
echo "load at end: $(sysctl -n vm.loadavg)"
echo "wrote $OUT ($(( $(wc -l < "$OUT") - 1 )) rows)"
