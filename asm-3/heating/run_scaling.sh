#!/bin/bash
#
# 159.735 Assignment 3 -- scaling measurements for the report.
#
#   ./run_scaling.sh
#
# Writes $OUTDIR/scaling.csv, one row per run, plus $OUTDIR/machine.txt.
#
# Portability
# -----------
# The same script drives the 159.735 HPC node (Ubuntu, 32 cores) and a macOS
# laptop. Nothing about the machine is hard coded: the core count is probed
# at start up and the thread sweep, the plate sizes and the iteration budgets
# are all derived from it. On a 10 core laptop it produces exactly the sweep
# it always did; on the 32 core node it sweeps out to 32 threads.
#
# Knobs (all optional, all environment variables):
#
#   OUTDIR=results-hpc   where to write, so a second machine's numbers do not
#                        overwrite the first's. Pass the same directory to
#                        ./analyse.py afterwards.
#   REPS=5               repeats per configuration; the analysis takes the
#                        fastest.
#   BUDGET=4             multiplies the strong scaling iteration counts.
#   MAXTHREADS=16        cap the sweep below the core count, eg if you are
#                        sharing the node with somebody else.
#   STUDIES="strong weak"  run only some of: converge strong weak sync.
#   OMP_WAIT_POLICY=active  keep idle threads spinning at the barrier instead
#                        of sleeping. Faster, but it burns a whole core per
#                        idle thread, so do not use it on a shared node.
#
# Measurement notes
# -----------------
# Single runs are noisy, on a laptop because other applications are running
# and on a cluster node because somebody else may be on it. Two things are
# done about that:
#
#   * every configuration is measured REPS times and the analysis takes the
#     fastest, which is the reading least polluted by other processes;
#   * repeats are interleaved (rep is the outer loop) so a burst of activity
#     from something else spreads itself over many configurations instead of
#     ruining one of them.
#
# The load average is recorded with every row so that contaminated readings
# can be spotted afterwards.
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

OS=$(uname -s)

# --------------------------------------------------------------------------
# Keep a laptop awake for the duration; a system sleep in the middle of a run
# makes nonsense of the timings. Cluster nodes do not sleep, so this is a
# no-op there.
# --------------------------------------------------------------------------
if [ "$OS" = Darwin ] && [ -z "${HEAT_AWAKE:-}" ] && command -v caffeinate >/dev/null 2>&1; then
  HEAT_AWAKE=1 exec caffeinate -dimsu "$0" "$@"
fi

# --------------------------------------------------------------------------
# Probe the machine. Everything below is derived from these.
# --------------------------------------------------------------------------
cpu_model() {
  local m=""
  case $OS in
    Linux)
      m=$(lscpu 2>/dev/null | sed -n 's/^Model name:[[:space:]]*//p' | head -1)
      [ -z "$m" ] && m=$(sed -n 's/^model name[[:space:]]*:[[:space:]]*//p' /proc/cpuinfo 2>/dev/null | head -1)
      ;;
    Darwin) m=$(sysctl -n machdep.cpu.brand_string 2>/dev/null) ;;
  esac
  echo "${m:-unknown CPU}"
}

# Logical CPUs. On Linux nproc honours the affinity mask and any cgroup quota,
# so under taskset or a batch scheduler it reports what we may actually use
# rather than what the box physically has.
n_logical() {
  case $OS in
    Linux)  nproc 2>/dev/null || getconf _NPROCESSORS_ONLN 2>/dev/null || echo 1 ;;
    Darwin) sysctl -n hw.ncpu 2>/dev/null || echo 1 ;;
  esac
}

# Physical cores, ie ignoring SMT/hyperthreading siblings. Worth knowing:
# above this thread count two threads share one core's arithmetic units and
# the efficiency curve bends, which is a feature of the machine rather than
# of the program.
n_physical() {
  local np=""
  case $OS in
    Linux)
      # One line per logical CPU as "core,socket"; distinct pairs are the
      # physical cores. Needs lscpu; if it is missing we just report the
      # logical count and no SMT marker is drawn.
      np=$(lscpu -p=Core,Socket 2>/dev/null | grep -v '^#' | sort -u | wc -l | tr -d ' ')
      ;;
    Darwin) np=$(sysctl -n hw.physicalcpu 2>/dev/null) ;;
  esac
  if [ -n "$np" ] && [ "$np" -gt 0 ] 2>/dev/null; then echo "$np"; else n_logical; fi
}

mem_gb() {
  case $OS in
    Linux)  awk '/^MemTotal:/ {printf "%d", $2/1048576}' /proc/meminfo 2>/dev/null ;;
    Darwin) echo $(( $(sysctl -n hw.memsize 2>/dev/null || echo 0) / 1073741824 )) ;;
  esac
}

# A probe that comes back empty or non-numeric must not take the arithmetic
# below with it; fall back to the given default instead.
num_or() {
  local v=$1 dflt=$2
  case $v in
    ''|*[!0-9]*) echo "$dflt" ;;
    *)           echo "$v" ;;
  esac
}

os_name() {
  case $OS in
    Linux)
      local pretty
      pretty=$(sed -n 's/^PRETTY_NAME="\(.*\)"$/\1/p' /etc/os-release 2>/dev/null | head -1)
      echo "${pretty:-Linux} (kernel $(uname -r), $(uname -m))"
      ;;
    Darwin) echo "macOS $(sw_vers -productVersion) ($(uname -m))" ;;
  esac
}

# One minute load average, recorded against every measurement.
loadavg1() {
  case $OS in
    Linux)  awk '{print $1}' /proc/loadavg 2>/dev/null || echo "?" ;;
    Darwin) sysctl -n vm.loadavg 2>/dev/null | awk '{print $2}' || echo "?" ;;
  esac
}

loadavg_all() {
  case $OS in
    Linux)  awk '{print "{ "$1" "$2" "$3" }"}' /proc/loadavg 2>/dev/null ;;
    Darwin) sysctl -n vm.loadavg 2>/dev/null ;;
  esac
}

NLOGICAL=$(num_or "$(n_logical)" 1)
NPHYSICAL=$(num_or "$(n_physical)" "$NLOGICAL")
MEMGB=$(num_or "$(mem_gb)" 0)

# The sweep goes up to every logical CPU unless told otherwise.
NCORE=$(num_or "${MAXTHREADS:-$NLOGICAL}" "$NLOGICAL")
[ "$NCORE" -lt 1 ] && NCORE=1

# --------------------------------------------------------------------------
# Thread affinity. Pinning each thread to its own core stops the OS migrating
# threads between cores mid-run, which otherwise shows up as scatter in the
# timings and as a wrongly pessimistic load balance figure. macOS does not let
# a process set affinity at all, so libomp there would only warn about it.
# --------------------------------------------------------------------------
if [ "$OS" = Linux ]; then
  export OMP_PROC_BIND=${OMP_PROC_BIND:-close}
  export OMP_PLACES=${OMP_PLACES:-cores}
fi

# --------------------------------------------------------------------------
# Thread sweeps, built from the core count.
#
# full   fine grained at the low end, where the curve bends, then evenly
#        spaced up to the core count. On a 10 core machine this is 1..10,
#        which is the sweep the laptop measurements used.
# coarse powers of two plus the core count, for the studies where a full
#        sweep would cost more time than it is worth.
# --------------------------------------------------------------------------
full_threads() {
  local nc=$1 step p
  step=$(( nc / 8 )); [ "$step" -lt 1 ] && step=1
  {
    for p in 1 2 3 4 6 8; do [ "$p" -le "$nc" ] && echo "$p"; done
    p=$step
    while [ "$p" -le "$nc" ]; do echo "$p"; p=$(( p + step )); done
    echo "$nc"
  } | sort -nu
}

coarse_threads() {
  local nc=$1 p=1
  { while [ "$p" -le "$nc" ]; do echo "$p"; p=$(( p * 2 )); done; echo "$nc"; } | sort -nu
}

THREADS_FULL=$(full_threads "$NCORE" | tr '\n' ' ')
THREADS_COARSE=$(coarse_threads "$NCORE" | tr '\n' ' ')

# --------------------------------------------------------------------------
# Iteration budgets for the strong scaling study.
#
# The budgets are set so that a *sequential* run takes about a second on a
# fast core, which keeps each measurement short enough to dodge most of the
# interference from other processes. On a many core machine that is not
# enough: divided over 32 threads a one second run finishes in 30 ms, which
# is short enough for the thread pool start up and the first touch page
# faults to be a large fraction of what is being timed. So the budget is
# scaled up with the core count, keeping the shortest parallel runs above
# roughly a tenth of a second.
#
# This changes nothing about the speedups. The work per iteration is the same
# for every thread count, so multiplying every run's iteration count by the
# same factor cancels out of T1/Tp.
# --------------------------------------------------------------------------
default_budget=$(( NCORE / 8 ))
[ "$default_budget" -lt 1 ] && default_budget=1
[ "$default_budget" -gt 4 ] && default_budget=4
BUDGET=${BUDGET:-$default_budget}

# 10000 iterations at 400x400, scaled to hold the work per run constant.
strong_iters() {
  awk -v n="$1" -v b="$BUDGET" \
      'BEGIN { r = 10000 * b / ((n/400)^2); printf "%d", (r < 1 ? 1 : r) }'
}

# Plate sizes for the strong study. A 400x400 plate cut over 32 threads gives
# each thread a dozen rows, so the largest sizes are the interesting ones on a
# big machine; 6400x6400 is added there. It needs about 550 MB at peak.
STRONG_SIZES="400 800 1600 3200"
if [ "$NCORE" -ge 16 ] && [ "$MEMGB" -ge 4 ]; then
  STRONG_SIZES="$STRONG_SIZES 6400"
fi

# Weak scaling: npix = 400*sqrt(p) keeps the pixels per thread constant.
weak_npix() { awk -v p="$1" 'BEGIN { printf "%d", int(400 * sqrt(p) + 0.5) }'; }
WEAK_ITERS=${WEAK_ITERS:-1500}

# A cluster node to yourself is far quieter than a laptop running a browser,
# so fewer repeats are needed to get a clean minimum.
if [ "$OS" = Darwin ]; then REPS=${REPS:-5}; else REPS=${REPS:-3}; fi

STUDIES=${STUDIES:-"converge strong weak sync"}
wanted() { case " $STUDIES " in *" $1 "*) return 0 ;; *) return 1 ;; esac; }

OUTDIR=${OUTDIR:-results}
OUT=$OUTDIR/scaling.csv
mkdir -p "$OUTDIR"

# --------------------------------------------------------------------------
# Sanity checks before spending half an hour on this.
# --------------------------------------------------------------------------
for prog in ./heat ./heat_omp; do
  if [ ! -x "$prog" ]; then
    echo "error: $prog not built. Run 'make' first." >&2
    exit 1
  fi
done

echo "study,version,part,npix,threads,iters,converged,time,busymax,busymin,busyavg,hash,rep,load" > "$OUT"

# Run one configuration and append its RESULT line to the CSV as one row.
#
# NOTE: the awk variable holding the load average is called ldavg, not load.
# "load" is a reserved builtin name in gawk (Ubuntu's awk), which rejects
# -v load=... with a fatal error, while the BSD awk on macOS accepts it. The
# CSV column is still called "load".
record() {
  local study=$1 rep=$2; shift 2
  local ldavg
  ldavg=$(loadavg1)
  "$@" 2>/dev/null | grep '^RESULT' | awk -v study="$study" -v rep="$rep" -v ldavg="$ldavg" '
    {
      for (i = 2; i <= NF; ++i) { split($i, a, "="); m[a[1]] = a[2] }
      printf "%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\n",
        study, m["version"], m["part"], m["npix"], m["threads"], m["iters"],
        m["converged"], m["time"], m["busymax"], m["busymin"], m["busyavg"],
        m["hash"], rep, ldavg
    }' >> "$OUT"
  printf '.'
}

# Prove the whole pipeline actually produces a CSV row before spending half an
# hour discovering that it does not. A broken awk, a binary that will not
# start or a RESULT line that has changed shape all show up here instead of as
# an empty CSV at the end.
preflight() {
  local before after
  before=$(wc -l < "$OUT")
  record preflight 0 ./heat 64 -o none -i 5 >/dev/null
  after=$(wc -l < "$OUT")
  if [ "$after" -le "$before" ]; then
    echo >&2
    echo "error: the measurement pipeline produced no CSV row." >&2
    echo "       Check that ./heat runs and that awk accepts -v:" >&2
    echo "         ./heat 64 -o none -i 5 | grep '^RESULT'" >&2
    echo "         $(awk --version 2>/dev/null | head -1 || awk -W version 2>&1 | head -1)" >&2
    exit 1
  fi
  # Drop the probe row again; it is not a measurement.
  grep -v '^preflight,' "$OUT" > "$OUT.tmp" && mv "$OUT.tmp" "$OUT"
}

# --------------------------------------------------------------------------
# Machine description. analyse.py reads this back: the "marker" line tells it
# where to draw the vertical line on the scaling figures, ie the thread count
# beyond which the machine stops offering another full core.
# --------------------------------------------------------------------------
{
  echo "machine : $(cpu_model), $NLOGICAL logical CPUs ($NPHYSICAL physical cores)"
  echo "memory  : ${MEMGB} GB"
  echo "os      : $(os_name)"
  echo "compiler: $(${CPP:-g++} --version 2>/dev/null | head -1)"
  echo "openmp  : OMP_PROC_BIND=${OMP_PROC_BIND:-unset} OMP_PLACES=${OMP_PLACES:-unset} OMP_WAIT_POLICY=${OMP_WAIT_POLICY:-unset}"
  echo "sweep   : up to $NCORE threads, budget x$BUDGET, $REPS repeats (analysis uses the fastest)"
  if [ "$OS" = Darwin ] && [ -n "$(sysctl -n hw.perflevel0.physicalcpu 2>/dev/null)" ]; then
    echo "cores   : $(sysctl -n hw.perflevel0.physicalcpu) performance + $(sysctl -n hw.perflevel1.physicalcpu) efficiency"
    echo "marker  : $(sysctl -n hw.perflevel0.physicalcpu) performance cores"
  elif [ "$NPHYSICAL" -lt "$NLOGICAL" ]; then
    echo "marker  : $NPHYSICAL physical cores (SMT beyond this)"
  fi
  echo "started : $(date)"
  echo "load at start: $(loadavg_all)"
} | tee "$OUTDIR/machine.txt"

echo
echo "thread sweep (full)   : $THREADS_FULL"
echo "thread sweep (coarse) : $THREADS_COARSE"
echo "strong plate sizes    : $STRONG_SIZES"
echo "studies               : $STUDIES"

printf "pipeline check        : "
preflight
echo "ok"

T0=$(date +%s)

# --------------------------------------------------------------------------
# 1. Runs to convergence: is the parallel answer the same?
# --------------------------------------------------------------------------
if wanted converge; then
echo
echo "[converge] full runs to convergence"
for n in 100 200 400; do
  for rep in 1 2; do
    printf "  n=%-5s rep %s " "$n" "$rep"
    record converge $rep ./heat "$n" -o none
    for p in $THREADS_COARSE; do
      record converge $rep ./heat_omp "$n" -p "$p" -o none
      record converge $rep ./heat_omp "$n" -p "$p" -d -o none
    done
    echo
  done
done
# One big one, for the correctness claim at a size worth calling large.
printf "  n=800  (single pass) "
record converge 1 ./heat 800 -o none
record converge 1 ./heat_omp 800 -p "$NCORE" -o none
record converge 1 ./heat_omp 800 -p "$NCORE" -d -o none
echo
fi

# --------------------------------------------------------------------------
# 2. Strong scaling.
# --------------------------------------------------------------------------
if wanted strong; then
echo
echo "[strong] fixed problem, fixed iteration budget, varying threads"
for rep in $(seq 1 $REPS); do
  for n in $STRONG_SIZES; do
    it=$(strong_iters "$n")
    printf "  rep %s  n=%-5s (%s iters) " "$rep" "$n" "$it"
    record strong $rep ./heat "$n" -o none -i "$it"
    for p in $THREADS_FULL; do
      record strong $rep ./heat_omp "$n" -p "$p" -o none -i "$it"
      record strong $rep ./heat_omp "$n" -p "$p" -d -o none -i "$it"
    done
    echo
  done
done
fi

# --------------------------------------------------------------------------
# 3. Weak scaling: npix = 400*sqrt(p), so pixels per thread stay constant.
# --------------------------------------------------------------------------
if wanted weak; then
echo
echo "[weak] plate grows as sqrt(p), work per thread held constant"
for rep in $(seq 1 $REPS); do
  printf "  rep %s " "$rep"
  for p in $THREADS_COARSE; do
    n=$(weak_npix "$p")
    record weak $rep ./heat "$n" -o none -i $WEAK_ITERS
    record weak $rep ./heat_omp "$n" -p "$p" -o none -i $WEAK_ITERS
    record weak $rep ./heat_omp "$n" -p "$p" -d -o none -i $WEAK_ITERS
  done
  echo
done
fi

# --------------------------------------------------------------------------
# 4. Synchronisation cost on its own.
# --------------------------------------------------------------------------
if wanted sync; then
echo
echo "[sync] per-iteration synchronisation cost"
for rep in $(seq 1 $REPS); do
  printf "  rep %s " "$rep"
  record sync $rep ./heat 64 -o none -i 20000
  for p in $THREADS_FULL; do
    record sync $rep ./heat_omp 64 -p "$p" -o none -i 20000
  done
  echo
done
fi

T1=$(date +%s)
echo
echo "load at end: $(loadavg_all)"
echo "elapsed    : $(( (T1 - T0) / 60 )) min $(( (T1 - T0) % 60 )) s"
echo "wrote $OUT ($(( $(wc -l < "$OUT") - 1 )) rows)"
echo
echo "next: ./analyse.py $OUTDIR"
