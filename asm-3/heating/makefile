# Makefile for 159.735 Assignment 3
#
# To build everything:
#
#     make
#
# Nothing in here is machine specific: the same makefile builds on the
# 159.735 HPC node (Ubuntu/GCC) and on a macOS laptop (Apple clang plus
# Homebrew's libomp). The two things that differ between them, where CFITSIO
# lives and how OpenMP is switched on, are worked out below.
#
# If a build fails, run
#
#     make env
#
# which reports the compiler, whether it can do OpenMP, and where CFITSIO was
# found.
#
# Useful overrides:
#
#     make CPP=g++-12               use a particular compiler
#     make CFITSIO=$(HOME)/cfitsio  CFITSIO in your own directory
#     make EXTRA=-march=native      tune for the machine you are building on
#
# -march=native is worth trying on the cluster: this is a memory bound
# stencil and the wider vector units help. It is off by default because it
# makes the binaries unportable between nodes. If you use it, check that
# heat and heat_omp still print the same "hash=" value -- that is the proof
# that the extra optimisation has not changed the arithmetic.

CPP = g++

CFLAGS = -w -O3 $(EXTRA)

# Locally compiled modules
OBJS = fitsfile.o

# ---------------------------------------------------------------------------
# Link to CFITSIO libraries
#
# Normally nothing needs changing here: pkg-config locates the library for you.
# That covers Homebrew on macOS (brew install cfitsio) as well as a distro
# package on Linux (Debian/Ubuntu: sudo apt install libcfitsio-dev).
#
# On a cluster you usually have no root, so if pkg-config comes up empty try,
# in order:
#
#   1. a module, if the site provides one:   module load cfitsio
#   2. an install in your own home directory:
#
#        make CFITSIO=$(HOME)/cfitsio
#
#      having built it there once with
#
#        ./configure --prefix=$(HOME)/cfitsio && make && make install
#
# ---------------------------------------------------------------------------
PKG_INC := $(shell pkg-config --cflags cfitsio 2>/dev/null)
PKG_LIB := $(shell pkg-config --libs   cfitsio 2>/dev/null)

ifneq ($(CFITSIO),)
  # Explicit location given on the command line
  INCP = -I$(CFITSIO) -I$(CFITSIO)/include
  LIBP = -L$(CFITSIO) -L$(CFITSIO)/lib
  LIBS = -lcfitsio -lm
else ifneq ($(strip $(PKG_LIB)),)
  # Found by pkg-config
  INCP = $(PKG_INC)
  LIBP =
  LIBS = $(PKG_LIB) -lm
else
  # Last resort: standard system locations
  INCP = -I/usr/include/cfitsio
  LIBP =
  LIBS = -lcfitsio -lm
endif

# ---------------------------------------------------------------------------
# OpenMP flags, for the parallel version.
#
# Apple's "g++" is really clang, and it rejects a bare -fopenmp. On macOS it
# needs Homebrew's libomp (brew install libomp) wired in by hand; on Linux
# plain -fopenmp is both the compile and the link flag.
# ---------------------------------------------------------------------------
UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S),Darwin)
  LOMP     := $(shell brew --prefix libomp 2>/dev/null)
  OMPFLAGS = -Xpreprocessor -fopenmp -I$(LOMP)/include
  OMPLIBS  = -L$(LOMP)/lib -lomp
else
  OMPFLAGS = -fopenmp
  OMPLIBS  = -fopenmp
endif

# NOTE: the object files must come BEFORE the libraries they depend on. The GNU
# linker resolves symbols in the order it sees them, so "-lcfitsio fitsfile.o"
# leaves fitsfile.o's references to ffopen/ffclos/... unresolved.
MODS = $(INCP) $(OBJS) $(LIBP) $(LIBS)

BINS = heat_demo heat heat_omp

.PHONY : all clean env

all : $(BINS)

clean :
	rm -f $(BINS)
	rm -f *.o

# Report what the build is about to use. Run this first on a machine you have
# not built on before; it turns three confusing link errors into one clear
# answer about what is missing.
env :
	@echo "compiler : $(CPP)"
	@$(CPP) --version | head -1
	@echo "system   : $(UNAME_S) ($(shell uname -m))"
	@echo "cores    : $(shell nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo '?')"
	@echo "CFLAGS   : $(CFLAGS)"
	@echo "omp flags: $(OMPFLAGS) | $(OMPLIBS)"
	@echo "cfitsio  : $(if $(CFITSIO),from CFITSIO=$(CFITSIO),$(if $(strip $(PKG_LIB)),via pkg-config,not found by pkg-config -- falling back to /usr/include/cfitsio))"
	@echo "  inc    : $(INCP)"
	@echo "  lib    : $(LIBP) $(LIBS)"
	@printf 'int main(){return 0;}\n' > .omptest.c
	@$(CPP) $(OMPFLAGS) .omptest.c $(OMPLIBS) -o .omptest 2>/dev/null \
	  && echo "openmp   : OK" || echo "openmp   : FAILED -- $(CPP) cannot compile with $(OMPFLAGS)"
	@rm -f .omptest .omptest.c

# Demo program. Add more programs by making entries similar to this
heat_demo : heat_demo.cpp draw.hxx array.hxx $(OBJS)
	${CPP} $(CFLAGS) -o heat_demo heat_demo.cpp $(MODS)

# Sequential solution.
heat : heat.cpp heatutil.hxx draw.hxx array.hxx $(OBJS)
	${CPP} $(CFLAGS) -o heat heat.cpp $(MODS)

# Parallel solution. The OpenMP flags are needed on BOTH the compile and the
# link side.
heat_omp : heat_omp.cpp heatutil.hxx draw.hxx array.hxx $(OBJS)
	${CPP} $(CFLAGS) $(OMPFLAGS) -o heat_omp heat_omp.cpp $(MODS) $(OMPLIBS)

# Modules compiled and linked separately
fitsfile.o : fitsfile.cpp fitsfile.h
	${CPP} $(CFLAGS) $(INCP) -c fitsfile.cpp
