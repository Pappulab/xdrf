# Makefile for xdrf test suite.
# Run from the test/ directory, or from the parent with 'make -C test'.

PARENT   = ..
LIBXDRF  = $(PARENT)/libxdrf.a

# Inherit compiler settings from parent
CC       = gcc
CXX      = g++
F77      = gfortran

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
ARCH     = darwin
else
ARCH     = linux
endif

CFLAGS   = -O -D$(ARCH) -I$(PARENT)
CXXFLAGS = -O -std=c++17 -D$(ARCH) -I$(PARENT)
FFLAGS   = -O3
LIBS     = -lm

TESTS    = test_xdrf test_fortran test_xdrf_cpp

.PHONY: all clean test

all: test

# Ensure library is built first
$(LIBXDRF):
	$(MAKE) -C $(PARENT) libxdrf.a

test_xdrf: test_xdrf.c $(LIBXDRF)
	$(CC) -o $@ $(CFLAGS) $< $(LIBXDRF) $(LIBS)

test_fortran: test_fortran.f $(LIBXDRF)
	$(F77) -o $@ $(FFLAGS) $< $(LIBXDRF) $(LIBS)

test_xdrf_cpp: test_xdrf_cpp.cpp $(LIBXDRF)
	$(CXX) -o $@ $(CXXFLAGS) $< $(LIBXDRF) $(LIBS)

test: $(TESTS)
	@echo ""
	@echo "==============================="
	@echo "  Running C tests"
	@echo "==============================="
	./test_xdrf
	@echo ""
	@echo "==============================="
	@echo "  Running Fortran tests"
	@echo "==============================="
	./test_fortran
	@echo ""
	@echo "==============================="
	@echo "  Running C++ tests"
	@echo "==============================="
	./test_xdrf_cpp
	@echo ""
	@echo "==============================="
	@echo "  Cleaning up temp files"
	@echo "==============================="
	rm -f tmp_*.xdr
	@echo "All tests complete."

clean:
	rm -f $(TESTS) tmp_*.xdr
