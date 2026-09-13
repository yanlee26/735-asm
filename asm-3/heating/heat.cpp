/******************************************************************************

  159.735 Assignment 3 -- Sequential solution of the heat distribution problem

  Solves Laplace's equation on a printed circuit plate by Jacobi iteration.
  The plate edge, the cold finger and the printed circuit components are held
  at fixed temperatures; every other pixel relaxes to the average of its four
  neighbours,

     g(y,x) = ( h(y,x-1) + h(y,x+1) + h(y-1,x) + h(y+1,x) ) / 4

  until no pixel moves by more than the tolerance in a whole sweep.

  To build:

     make heat

  To run (100x100 plate, answer written to plate1.fit):

     ./heat 100

  Usage: ./heat <npix> [options]

     npix     plate is npix x npix pixels
     -t TOL   convergence tolerance in Kelvin  (default 1e-5)
     -o FILE  FITS file for the answer         (default plate1.fit;
              "none" skips all file output, for timing runs)
     -i N     stop after N iterations even if the plate has not converged.
              Purely a benchmarking aid: it lets the cost of an iteration be
              measured on plates far too large to relax fully. Leave it out
              for a real run.

******************************************************************************/
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "arrayff.hxx"
#include "draw.hxx"
#include "heatutil.hxx"

int main(int argc, char* argv[])
{
  const HeatOptions opt = parse_options(argc, argv, "plate1.fit");
  if (!opt.ok) {
    std::cerr << "Usage: " << argv[0]
              << " <npix> [-t tol] [-o outfile|none] [-i maxiter]\n";
    return 1;
  }

  // X and Y dimensions. Force it to be a square.
  const int npixx  = opt.npix;
  const int npixy  = opt.npix;
  const int ntotal = npixx * npixy;

  // h is the current image, g is the updated image. y is the first index, to
  // reflect row major order: h(y, x).
  Array<float, 2> h(npixy, npixx), g(npixy, npixx);

  // Draw the printed circuit components into both buffers. Both need them
  // because the two buffers take turns at being the current image, and the
  // outermost ring of pixels is never touched by a sweep.
  fix_boundaries2<float>(h);
  fix_boundaries2<float>(g);

  // Work out once which pixels the circuit occupies, rather than re-drawing
  // it on every iteration.
  std::vector<unsigned char> fixed;
  build_fixed_mask(npixy, npixx, fixed);

  // The initial value image, with boundaries and circuit components fixed.
  if (opt.writefits()) dump_array<float, 2>(h, "plate0.fit");

  // Every interior pixel has to settle before the plate counts as converged.
  // The pixels on the outer edge are fixed, so they are converged by
  // definition and are left out of the count.
  const int nrequired = (npixx - 2) * (npixy - 2);

  int iter = 0;
  int nconverged = 0;

  const double tstart = wall_seconds();

  do {

    // One Jacobi sweep over every interior row, counting settled pixels as
    // it goes. row_range() with a team of one hands back the whole plate:
    // the sequential program is the one thread case of the parallel one.
    int y0, y1;
    row_range(npixy, 1, 0, y0, y1);
    nconverged = jacobi_sweep(h.buffer, g.buffer, &fixed[0],
                              npixx, y0, y1, opt.tol);

    // The new image becomes the current one. Swapping the buffers costs two
    // pointer assignments instead of copying n^2 floats.
    std::swap(h.buffer, g.buffer);

    ++iter;

  } while (nconverged < nrequired && iter < opt.itmax);

  const double secs = wall_seconds() - tstart;

  const bool converged = (nconverged >= nrequired);
  if (!converged)
    std::cerr << "NOTE: stopped at the iteration limit, not yet converged\n";

  if (opt.writefits()) dump_array<float, 2>(h, opt.outfile);

  std::cout << "Required " << iter << " iterations" << std::endl;
  std::cout << std::fixed << std::setprecision(4)
            << "Elapsed " << secs << " s"
            << "  (" << 1.0e3 * secs / iter << " ms/iteration)" << std::endl;

  std::cout << "RESULT version=seq npix=" << opt.npix
            << " threads=1 part=seq"
            << " tol=" << std::scientific << std::setprecision(2) << opt.tol
            << " iters=" << iter
            << " converged=" << (converged ? 1 : 0)
            << std::fixed << std::setprecision(6)
            << " time=" << secs
            << " mean=" << array_mean(h.buffer, ntotal)
            << " hash=" << std::hex << array_hash(h.buffer, ntotal) << std::dec
            << std::endl;

  return 0;
}
