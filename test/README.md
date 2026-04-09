# xdrf test suite

Unit tests for the xdrf library covering both the C and Fortran APIs.

## Building and running

From the `test/` directory:

    make test

Or from the project root:

    make -C test

This will build the library (if needed), compile both test programs, run them, and clean up temporary files.

## Test programs

### `test_xdrf.c` — C API tests

29 tests covering:

- **Open/Close**: write mode, read mode, nonexistent file handling
- **Primitive round-trips**: int, float, double, short, char, string, bool, mixed types
- **xdr3dfcoord**: small arrays, boundary cases (9/10 atoms), 100 and 1000 atoms, precision levels, negative/large/identical/scattered coordinates, water-like repeating patterns
- **Advanced**: append mode, multiple open files, reopen, setpos/getpos, multi-frame trajectories, metadata interleaved with coordinates, compression ratio verification

### `test_fortran.f` — Fortran API tests

15 tests covering:

- Primitive round-trips (int, float, double, short, char, bool, mixed)
- xdr3dfcoord at various sizes (small, compressed, 100 atoms)
- Multi-frame trajectories, negative coordinates
- setpos/getpos, vector float, file reopen

## Requirements

- `gcc` and `gfortran`
- The parent directory must contain the xdrf source (`libxdrf.m4`, `ftocstr.c`, etc.)
- macOS and Linux are auto-detected; no manual configuration needed

## Cleanup

Temporary `.xdr` files are removed automatically after a test run. To remove compiled binaries:

    make clean
