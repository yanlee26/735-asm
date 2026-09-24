#ifndef HEATUTIL_HXX
#define HEATUTIL_HXX

/*
  159.735 Assignment 3 -- shared helpers for the heat distribution solver.

  Everything that the sequential program (heat.cpp) and the OpenMP program
  (heat_omp.cpp) have in common lives here, so that both versions are
  guaranteed to run *exactly* the same arithmetic in the same order on any
  given pixel. That is what makes the two programs produce bit-identical
  images and stop after the same number of iterations.

  Nothing in here needs OpenMP; heatutil.hxx compiles fine without it.
*/

#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <string>
#include <vector>

#include "arrayff.hxx"
#include "draw.hxx"

// Convergence tolerance and iteration cap used by both programs unless
// overridden on the command line.
const float HEAT_TOL = 0.00001f;
const int HEAT_ITMAX = 1000000;

/*
  Seconds on a monotonic clock, used for every timing in both programs.

  Deliberately not omp_get_wtime(): the LLVM runtime implements that with
  gettimeofday(), which is wall clock time and so keeps counting while the
  machine is asleep -- a laptop that dozes off part way through a long run
  then reports hours for a job that took seconds. steady_clock does not tick
  across a system sleep, and using one clock in both programs keeps the
  sequential and parallel times directly comparable.
*/
inline double wall_seconds()
{
  return std::chrono::duration<double>(
             std::chrono::steady_clock::now().time_since_epoch())
      .count();
}

/*
  Which pixels are held at a fixed temperature?

  fix_boundaries2() paints the cold plate edge, the cold finger and the
  printed circuit components. Those pixels are Dirichlet boundary conditions:
  they must keep their value for the whole run. Rather than re-drawing the
  circuit on every iteration (which is a serial O(n^2) section, and hurts the
  parallel version for no good reason) we work out once which pixels the
  drawing routines touch, and afterwards simply carry those values through.

  The trick is to run the supplied drawing code on a scratch array that has
  been filled with a sentinel value: any pixel that is no longer the sentinel
  afterwards is one the circuit was drawn on.
*/
inline void build_fixed_mask(int npixy, int npixx,
                             std::vector<unsigned char> &fixed)
{
  const float sentinel = -1.0e30f;

  Array<float, 2> probe(npixy, npixx);
  probe.reset(sentinel);
  fix_boundaries2<float>(probe);

  fixed.assign(static_cast<size_t>(npixy) * npixx, 0);
  for (int y = 0; y < npixy; ++y)
    for (int x = 0; x < npixx; ++x)
      if (probe(y, x) != sentinel)
        fixed[static_cast<size_t>(y) * npixx + x] = 1;
}

/*
  Partition the interior rows [1, npixy-1) into nth contiguous blocks and
  return the half open range [y0, y1) belonging to thread tid.

  Contiguous blocks of whole rows are used rather than, say, interleaved rows
  because each thread then reads mostly its own rows: only the single row
  above y0 and the single row below y1-1 are owned by a neighbour. The
  remainder is spread one row at a time over the first few threads so that no
  two threads ever differ by more than one row of work.

  With nth == 1 this hands back every interior row, which is precisely what
  the sequential program wants.
*/
inline void row_range(int npixy, int nth, int tid, int &y0, int &y1)
{
  const int nrows = npixy - 2; // interior rows only
  if (nrows <= 0 || tid >= nth)
  {
    y0 = 1;
    y1 = 1;
    return;
  }

  const int base = nrows / nth;
  const int extra = nrows % nth;
  const int start = tid * base + (tid < extra ? tid : extra);
  const int count = base + (tid < extra ? 1 : 0);

  y0 = 1 + start;
  y1 = y0 + count;
}

/*
  One Jacobi sweep over the rows [y0, y1) of the plate.

    g(y,x) = ( h(y,x-1) + h(y,x+1) + h(y-1,x) + h(y+1,x) ) / 4

  Pixels belonging to the circuit keep their old value. The number of pixels
  whose temperature moved by less than tol is returned, which is how both
  programs detect convergence.

  This is the *only* place the stencil is written down. The sequential
  program calls it once per iteration for the whole plate; each OpenMP thread
  calls it once per iteration for its own block of rows. Since the update of
  a pixel depends only on the previous image h, splitting the row range
  changes nothing about the value any pixel gets.
*/
inline int jacobi_sweep(const float *h, float *g, const unsigned char *fixed,
                        int npixx, int y0, int y1, float tol)
{
  int nconv = 0;

  for (int y = y0; y < y1; ++y)
  {

    const float *hup = h + static_cast<size_t>(y - 1) * npixx;
    const float *hmid = h + static_cast<size_t>(y) * npixx;
    const float *hdn = h + static_cast<size_t>(y + 1) * npixx;
    float *gmid = g + static_cast<size_t>(y) * npixx;
    const unsigned char *fx = fixed + static_cast<size_t>(y) * npixx;

    int rowconv = 0;
    for (int x = 1; x < npixx - 1; ++x)
    {
      const float old = hmid[x];
      const float upd = fx[x] ? old
                              : 0.25f * (hmid[x - 1] + hmid[x + 1] + hup[x] + hdn[x]);
      gmid[x] = upd;
      if (std::fabs(upd - old) < tol)
        ++rowconv;
    }
    nconv += rowconv;
  }

  return nconv;
}

/*
  Command line handling, shared so that the two programs take the same
  options. The plate size is positional; everything else is a flag:

     -p N     size of the OpenMP thread pool  (heat_omp only)
     -t TOL   convergence tolerance in Kelvin
     -o FILE  where to write the answer, or "none" for no file output
     -i N     stop after N iterations even if not converged (benchmark aid)
     -d       use the dynamic partition instead of static blocks (heat_omp)
*/
struct HeatOptions
{
  int npix;
  int nthreads; // 0: let OpenMP decide
  float tol;
  std::string outfile;
  int itmax;
  bool dynamic;
  bool ok;

  HeatOptions()
      : npix(0), nthreads(0), tol(HEAT_TOL), outfile("plate1.fit"),
        itmax(HEAT_ITMAX), dynamic(false), ok(false) {}

  bool writefits() const { return outfile != "none"; }
};

inline HeatOptions parse_options(int argc, char *argv[],
                                 const std::string &defout)
{
  HeatOptions o;
  o.outfile = defout;

  if (argc < 2)
    return o;
  o.npix = atoi(argv[1]);

  for (int i = 2; i < argc; ++i)
  {
    const std::string a = argv[i];
    const bool hasval = (i + 1 < argc);
    if (a == "-p" && hasval)
      o.nthreads = atoi(argv[++i]);
    else if (a == "-t" && hasval)
      o.tol = static_cast<float>(atof(argv[++i]));
    else if (a == "-o" && hasval)
      o.outfile = argv[++i];
    else if (a == "-i" && hasval)
      o.itmax = atoi(argv[++i]);
    else if (a == "-d")
      o.dynamic = true;
    else
      return o; // unrecognised: bail out
  }

  if (o.npix < 3 || o.tol <= 0.0f || o.itmax < 1)
    return o;

  o.ok = true;
  return o;
}

// FNV-1a over the raw bytes of the image. Printing this lets us prove that
// the sequential and parallel runs produced the identical image, without
// having to diff two FITS files (whose headers carry timestamps).
inline unsigned long long array_hash(const float *buf, size_t n)
{
  unsigned long long hsh = 1469598103934665603ULL;
  const unsigned char *p = reinterpret_cast<const unsigned char *>(buf);
  const size_t nbytes = n * sizeof(float);
  for (size_t i = 0; i < nbytes; ++i)
  {
    hsh ^= static_cast<unsigned long long>(p[i]);
    hsh *= 1099511628211ULL;
  }
  return hsh;
}

inline double array_mean(const float *buf, size_t n)
{
  double s = 0.0;
  for (size_t i = 0; i < n; ++i)
    s += buf[i];
  return s / static_cast<double>(n);
}

#endif
