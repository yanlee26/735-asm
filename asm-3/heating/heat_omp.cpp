/******************************************************************************

  159.735 Assignment 3 -- OpenMP solution of the heat distribution problem

  Same Jacobi solver as heat.cpp, run across a pool of OpenMP threads. Both
  programs call the same jacobi_sweep() out of heatutil.hxx, so a pixel gets
  exactly the same arithmetic whichever program computes it.

  Parallelisation strategy
  ------------------------
  The plate is cut into horizontal strips of whole rows. A new pixel value
  depends only on the *previous* image, so no two threads ever disagree about
  a pixel and the answer does not depend on how the rows were shared out.
  Rows are the unit of work because a row is contiguous in memory: a thread
  reads only two rows it does not own, the halo row just above its strip and
  the halo row just below it.

  Two ways of sharing the rows out are provided:

    static (default)  Each thread is given one contiguous block of rows once,
                      at the start, with the remainder spread one row at a
                      time so no two threads differ by more than a row. No
                      bookkeeping during the run and the best cache locality,
                      because a thread revisits its own rows every iteration.

    dynamic (-d)      The rows are cut into chunks and threads help themselves
                      to the next chunk from a shared counter. Costs one
                      atomic increment per chunk and loses some locality, but
                      it is the better choice when the cores are not equally
                      fast -- see the report for the measured difference on
                      this machine's mix of performance and efficiency cores.

  The thread pool is created once, outside the iteration loop, and the whole
  do-while loop runs inside the parallel region, so threads are forked once
  per run rather than once per iteration.

  Synchronisation
  ---------------
  This is a local synchronisation problem: a thread must not run ahead of the
  neighbours it shares halo rows with. Two team-wide barriers per iteration
  are enough, and are simpler than pairwise neighbour signalling:

    (1) an explicit barrier once every thread has finished writing its rows
        of g. Without it a thread could start the next sweep and overwrite a
        row of h that a neighbour is still reading as a halo row. It also
        makes every per-thread convergence count visible.

    (2) the implicit barrier at the end of the "single" block, in which one
        thread adds up the per-thread counts, decides whether to stop, and
        swaps the two image buffers over for the whole team.

  Convergence and termination
  ---------------------------
  The number of iterations needed is not known in advance, so each thread
  counts how many of its own pixels moved by less than tol and writes that
  into its own padded slot of a shared array. After barrier (1) one thread
  adds the slots up: the plate has converged when every interior pixel has
  settled. That thread writes the verdict into a single shared flag, and
  every thread tests that same flag after barrier (2). Because they all read
  one flag after a barrier, the whole team leaves the loop on the same
  iteration -- no thread is ever left waiting at a barrier the others have
  already walked away from, and none exits early leaving work undone.

  To build:

     make heat_omp

  Usage: ./heat_omp <npix> [options]

     npix     plate is npix x npix pixels
     -p N     size of the thread pool  (default: OMP_NUM_THREADS/all cores)
     -t TOL   convergence tolerance in Kelvin  (default 1e-5)
     -o FILE  FITS file for the answer         (default plate1_omp.fit;
              "none" skips all file output, for timing runs)
     -i N     stop after N iterations even if the plate has not converged.
              Purely a benchmarking aid. Leave it out for a real run.
     -d       use the dynamic partition instead of static blocks

******************************************************************************/
#include <algorithm>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include <omp.h>

#include "arrayff.hxx"
#include "draw.hxx"
#include "heatutil.hxx"

// Shared counters are given a cache line each. Packed together they would
// share a line and the threads would spend their time bouncing that line
// between cores instead of working (false sharing). 128 bytes is the line
// size on Apple silicon and a safe over-estimate of the 64 byte x86 line.
const int CACHE_LINE = 128;

struct PaddedCount {
  int n;
  char pad[CACHE_LINE - sizeof(int)];
};

// Seconds each thread spent inside jacobi_sweep. Against the wall clock this
// says how much of the run a thread spent working rather than waiting at a
// barrier, which is the clearest measure of how well the partition balanced.
struct PaddedTime {
  double t;
  char pad[CACHE_LINE - sizeof(double)];
};

int main(int argc, char* argv[])
{
  const HeatOptions opt = parse_options(argc, argv, "plate1_omp.fit");
  if (!opt.ok) {
    std::cerr << "Usage: " << argv[0]
              << " <npix> [-p nthreads] [-t tol] [-o outfile|none]"
              << " [-i maxiter] [-d]\n";
    return 1;
  }

  // X and Y dimensions. Force it to be a square.
  const int npixx  = opt.npix;
  const int npixy  = opt.npix;
  const int ntotal = npixx * npixy;

  if (opt.nthreads > 0) omp_set_num_threads(opt.nthreads);

  // h is the current image, g is the updated image.
  Array<float, 2> h(npixy, npixx), g(npixy, npixx);
  fix_boundaries2<float>(h);
  fix_boundaries2<float>(g);

  std::vector<unsigned char> fixed;
  build_fixed_mask(npixy, npixx, fixed);
  const unsigned char* fixedp = &fixed[0];

  if (opt.writefits()) dump_array<float, 2>(h, "plate0.fit");

  const int nrequired = (npixx - 2) * (npixy - 2);

  // Sized before the parallel region so no thread can race the vectors' own
  // bookkeeping.
  const int maxth = omp_get_max_threads();
  std::vector<PaddedCount> counts(maxth);
  std::vector<PaddedTime>  busy(maxth);
  for (int t = 0; t < maxth; ++t) busy[t].t = 0.0;

  // For the dynamic partition: aim at about four chunks per thread. Fewer
  // and a straggler cannot be compensated for; many more and the atomic
  // counter starts to cost something.
  const int nrows  = npixy - 2;
  int chunk = nrows / (4 * (maxth > 0 ? maxth : 1));
  if (chunk < 1) chunk = 1;
  const int nchunks = (nrows + chunk - 1) / chunk;
  int nextchunk = 0;

  int  iter = 0;
  int  nconverged = 0;
  int  nthreads_used = 1;
  bool done = false;

  const double tstart = wall_seconds();

  #pragma omp parallel default(shared)
  {
    const int tid = omp_get_thread_num();
    const int nth = omp_get_num_threads();

    // This thread's strip of the plate, for the static partition.
    int y0, y1;
    row_range(npixy, nth, tid, y0, y1);

    if (tid == 0) nthreads_used = nth;

    double mybusy = 0.0;

    for (;;) {

      // Update my rows of g from h, and count how many of my pixels settled.
      const double tw0 = wall_seconds();

      if (!opt.dynamic) {

        counts[tid].n = jacobi_sweep(h.buffer, g.buffer, fixedp,
                                     npixx, y0, y1, opt.tol);

      } else {

        int myconv = 0;
        for (;;) {
          int mine;
          #pragma omp atomic capture
          mine = nextchunk++;
          if (mine >= nchunks) break;

          const int a = 1 + mine * chunk;
          int b = a + chunk;
          if (b > npixy - 1) b = npixy - 1;

          myconv += jacobi_sweep(h.buffer, g.buffer, fixedp,
                                 npixx, a, b, opt.tol);
        }
        counts[tid].n = myconv;

      }

      mybusy += wall_seconds() - tw0;

      // (1) Everyone has finished writing g and publishing their count.
      #pragma omp barrier

      #pragma omp single
      {
        int total = 0;
        for (int t = 0; t < nth; ++t) total += counts[t].n;
        nconverged = total;

        ++iter;
        done = (total >= nrequired) || (iter >= opt.itmax);

        // The new image becomes the current one for the whole team.
        std::swap(h.buffer, g.buffer);
        nextchunk = 0;
      }
      // (2) Implicit barrier at the end of "single": every thread now sees
      //     the swapped buffers, the refilled chunk counter and the same
      //     value of done, so the team leaves together.

      if (done) break;
    }

    busy[tid].t = mybusy;
  }

  const double secs = wall_seconds() - tstart;

  const bool converged = (nconverged >= nrequired);
  if (!converged)
    std::cerr << "NOTE: stopped at the iteration limit, not yet converged\n";

  if (opt.writefits()) dump_array<float, 2>(h, opt.outfile);

  // How evenly did the work land? A thread busy for the whole run is the one
  // holding everybody else up at the barriers.
  double busymin = busy[0].t, busymax = busy[0].t, busysum = 0.0;
  for (int t = 0; t < nthreads_used; ++t) {
    if (busy[t].t < busymin) busymin = busy[t].t;
    if (busy[t].t > busymax) busymax = busy[t].t;
    busysum += busy[t].t;
  }
  const double busyavg = busysum / nthreads_used;

  std::cout << "Required " << iter << " iterations on " << nthreads_used
            << " threads (" << (opt.dynamic ? "dynamic" : "static")
            << " partition)" << std::endl;
  std::cout << std::fixed << std::setprecision(4)
            << "Elapsed " << secs << " s"
            << "  (" << 1.0e3 * secs / iter << " ms/iteration)" << std::endl;
  std::cout << std::fixed << std::setprecision(1)
            << "Load balance: busiest thread " << 100.0 * busymax / secs
            << "% of wall time, idlest " << 100.0 * busymin / secs
            << "%, mean " << 100.0 * busyavg / secs << "%" << std::endl;

  std::cout << "RESULT version=omp npix=" << opt.npix
            << " threads=" << nthreads_used
            << " part=" << (opt.dynamic ? "dynamic" : "static")
            << " tol=" << std::scientific << std::setprecision(2) << opt.tol
            << " iters=" << iter
            << " converged=" << (converged ? 1 : 0)
            << std::fixed << std::setprecision(6)
            << " time=" << secs
            << " mean=" << array_mean(h.buffer, ntotal)
            << " hash=" << std::hex << array_hash(h.buffer, ntotal) << std::dec
            << std::setprecision(4)
            << " busymax=" << busymax << " busymin=" << busymin
            << " busyavg=" << busyavg
            << std::endl;

  return 0;
}
