# xdrf test suite

Unit tests for the xdrf library covering the C, C++ and Fortran APIs.

## Building and running

From the `test/` directory:

    make test

Or from the project root:

    make -C test

This will build the library (if needed), compile all three test programs, run them, and clean up temporary files.

Each program writes its scratch files as `tmp_*.xdr` in the working directory and removes them as it goes; `make test` sweeps up any left behind by a failing run.

## Test programs

### `test_xdrf.c` — C API tests

32 tests covering:

- **Open/Close**: write mode, read mode, nonexistent file handling
- **Primitive round-trips**: int, float, double, short, char, string, bool, mixed types
- **xdr3dfcoord**: small arrays, boundary cases (9/10 atoms), 100 and 1000 atoms, precision levels, negative/large/identical/scattered coordinates, water-like repeating patterns
- **Advanced**: append mode, multiple open files, reopen, setpos/getpos, multi-frame trajectories, metadata interleaved with coordinates, compression ratio verification
- **Robustness**: mixing caller-owned and library-owned `XDR` structs in either close order, rejection of a `size` that disagrees with the file, and rejection of corrupt frames (out-of-range compression index, inverted coordinate range, oversized compressed block, negative atom count)

### `test_xdrf_cpp.cpp` — C++ API tests

24 tests covering:

- **Construction / RAII**: open and close via scope, explicit close, nonexistent file, move construction and assignment, operations on a closed file
- **Primitive round-trips**: int, float, double, short, char, bool, string, mixed types
- **xdr3dfcoord**: `std::vector` and raw-pointer overloads, 100 and 1000 atoms, precision levels, rejection of a coordinate count that is not a multiple of 3
- **Advanced**: multi-frame trajectories, setpos/getpos, append mode, compression ratio verification

### `test_fortran.f` — Fortran API tests

15 tests covering:

- Primitive round-trips (int, float, double, short, char, bool, mixed)
- xdr3dfcoord at various sizes (small, compressed, 100 atoms)
- Multi-frame trajectories, negative coordinates
- setpos/getpos, vector float, file reopen

The robustness tests in `test_xdrf.c` call `xdrfclose_` directly, to close a library-owned handle by its id. The trailing underscore matches `conf/linux.m4` and `conf/darwin.m4`, the two configurations this suite builds for.

## Requirements

- `gcc`, `g++` (C++17) and `gfortran`
- The parent directory must contain the xdrf source (`libxdrf.m4`, `ftocstr.c`, etc.)
- macOS and Linux are auto-detected; no manual configuration needed

## Cleanup

Temporary `.xdr` files are removed automatically after a test run. To remove compiled binaries:

    make clean
