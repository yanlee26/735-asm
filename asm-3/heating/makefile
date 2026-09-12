# Makefile for 159.735 Assignment 3
#
# To build just type:
#
# make heat_demo

CPP = g++

CFLAGS = -w -O3

# Locally compiled modules
OBJS = fitsfile.o

# ---------------------------------------------------------------------------
# Link to CFITSIO libraries
#
# Normally nothing needs changing here: pkg-config locates the library for you.
# That covers Homebrew on macOS (brew install cfitsio) as well as a distro
# package on Linux (Debian/Ubuntu: sudo apt install libcfitsio-dev).
#
# If you built CFITSIO from source into your own directory, point at it with:
#
#     make heat_demo CFITSIO=$(HOME)/cfitsio
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

BINS = heat_demo

all : $(BINS)

clean :
	rm -f $(BINS)
	rm -f *.o

# Demo program. Add more programs by making entries similar to this
heat_demo : heat_demo.cpp draw.hxx array.hxx $(OBJS)
	${CPP} $(CFLAGS) -o heat_demo heat_demo.cpp $(MODS)

# Sequential solution. Uncomment once you have written heat.cpp, and add it
# to BINS above so that a plain "make" builds it too.
#heat : heat.cpp draw.hxx array.hxx $(OBJS)
#	${CPP} $(CFLAGS) -o heat heat.cpp $(MODS)

# Parallel solution. Uncomment once you have written heat_omp.cpp. The OpenMP
# flags are needed on BOTH the compile and the link side.
#heat_omp : heat_omp.cpp draw.hxx array.hxx $(OBJS)
#	${CPP} $(CFLAGS) $(OMPFLAGS) -o heat_omp heat_omp.cpp $(MODS) $(OMPLIBS)

# Modules compiled and linked separately
fitsfile.o : fitsfile.cpp fitsfile.h
	${CPP} $(CFLAGS) $(INCP) -c fitsfile.cpp
