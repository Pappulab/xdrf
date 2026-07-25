# libxdrf

###### Current version 1.6 (July 2026)

A portable C/C++/Fortran library for reading and writing XDR (External Data Representation) data, with built-in lossy compression for 3D coordinates. Originally developed for the EUROPORT project by [Frans van Hoesel](https://site.acornatom.nl/atom_handleidingen/pcharme/fvh/frans.html), this version is maintained with bug fixes and modern compiler support.

## Overview

`libxdrf` solves two problems common in molecular dynamics simulations:

1. **Portability** — Binary data written on one architecture (e.g. big-endian SPARC) can be read on another (e.g. little-endian x86) using the XDR standard.
2. **Compression** — The `xdr3dfcoord` routine compresses 3D floating-point coordinates to ~30% of their uncompressed binary size, exploiting spatial locality in molecular data.

The library provides three interfaces for the same underlying functionality:

| Interface | Header | Description |
|---|---|---|
| **C** | `xdrf.h` | Core API — three functions (`xdropen`, `xdrclose`, `xdr3dfcoord`) plus standard XDR primitives |
| **C++** | `xdrf.hpp` | Header-only RAII wrapper (`xdrf::XdrFile`) around the C API with typed methods and exception handling |
| **Fortran** | *(via m4 wrappers)* | Fortran-callable wrappers (`xdrfopen`, `xdrfint`, `xdrf3dfcoord`, etc.) generated from `libxdrf.m4` |

All three interfaces read and write the same binary XDR format and can interoperate — a file written with the C++ API can be read from Fortran and vice versa. The C library (`libxdrf.a`) is the common dependency for all three.

The library is commonly used as a build dependency for [CAMPARI](http://campari.sourceforge.net/).

## Requirements

- **C compiler**: `gcc` or `clang`
- **C++ compiler** (optional, for C++ interface): `g++` or `clang++` with C++17 support
- **Fortran compiler** (optional, for Fortran interface): `gfortran`
- **RPC/XDR headers**:
  - **macOS**: Included with Xcode Command Line Tools (no extra packages needed).
  - **Linux (Ubuntu/Debian)**: Install `libtirpc-dev`:
    ```
    sudo apt install libtirpc-dev
    ```
  - **Linux (RHEL/Fedora)**: Install `libtirpc-devel`:
    ```
    sudo dnf install libtirpc-devel
    ```

## Building

The Makefile auto-detects macOS (`Darwin`) vs Linux. Just run:

```bash
make
```

This produces:

- `libxdrf.a` — the static library (used by all three interfaces)
- `ctest` — C smoke test
- `ftest` — Fortran smoke test
- `cxxtest` — C++ smoke test

### Compiler options

To use different compilers (e.g. Intel), edit the `CC`, `CXX`, and `F77` variables in the `Makefile`:

```makefile
CC  = icc
CXX = icpc
F77 = ifort
```

### Architecture override

The build auto-detects macOS vs Linux. To force a specific architecture config from the `conf/` directory:

```bash
make ARCH=linux
```

### Supported architectures

The `conf/` directory contains m4 configuration files for many platforms. Each config defines how Fortran function names are mangled and how strings are passed across the C/Fortran boundary.

Two naming families live side by side: the uppercase PVM\_ARCH names (`SUN4`, `RS6K`, `ALPHA`, …) and the lowercase short codes (`sun`, `sgi`, `linux`, `darwin`, …). If an uppercase `ARCH` has no config of its own, the build falls back to the lowercase spelling, so `make ARCH=SGI` and `make ARCH=sgi` both work.

| Config file(s) | Platform | Name mangling |
|---|---|---|
| `linux.m4` | Linux (GNU/glibc) | `name_` |
| `darwin.m4` | macOS (x86\_64 and arm64) | `name_` |
| `SUN4SOL2.m4`, `sol.m4` | Sun Solaris 2 | `name_` |
| `SUN3.m4`, `SUN4.m4`, `sun.m4` | SunOS | `name_` |
| `sgi.m4`, `sgd.m4`, `sg8.m4`, `sgp.m4` | SGI IRIX | `name_` |
| `ALPHA.m4`, `ald.m4`, `alx.m4` | DEC Alpha / OSF1 | `name_` |
| `dec.m4`, `ded.m4` | DECstation / Ultrix | `name_` |
| `hp.m4`, `hpd.m4`, `HPPA.m4` | HP PA-RISC (conditional `name_` or `name`) | conditional |
| `HP300.m4` | HP 300 | `name` (no transform) |
| `RS6K.m4` | IBM RS/6000 (AIX) | `name` (no transform) |
| `sp2.m4`, `sp8.m4`, `spd.m4`, `spp.m4` | IBM SP2 | `name_` |
| `CRAY.m4`, `CRAY2.m4`, `CRAYSMP.m4`, `ymp.m4` | Cray (Y-MP, C90, T3E) | `NAME` (uppercase) |
| `CNVX.m4`, `CNVXN.m4` | Convex | `name_` |
| `BSD386.m4` | BSD/386 | `NAME` (uppercase) |
| `lnx.m4` | Linux (old, uppercase convention) | `NAME` (uppercase) |
| `win.m4` | Windows | `NAME` (uppercase) |
| `NEXT.m4` | NeXT | `name` (no transform) |
| `KSR1.m4` | Kendall Square KSR-1 | `name` (no transform) |
| `TITN.m4` | Stardent Titan | `NAME` (uppercase, FSD struct) |
| `U370.m4` | IBM 370 / VM/CMS | `name_` (pass-by-ref string length) |
| `BFLY.m4` | BBN Butterfly | `name_` |
| `CM2.m4`, `cm5.m4` | Thinking Machines CM-2/CM-5 | `name_` |
| `I860.m4` | Intel i860 | `name_` |
| `IPSC2.m4` | Intel iPSC/2 | `name_` |
| `MASPAR.m4` | MasPar | `name_` |
| `PGON.m4` | Intel Paragon | `name_` |
| `PMAX.m4` | DECstation (PMAX) | `name_` |
| `SYMM.m4` | Sequent Symmetry | `name_` |
| `UVAX.m4` | MicroVAX / Ultrix | `name_` |
| `RT.m4` | IBM RT | `name_` |
| `BAL.m4` | Sequent Balance | `name_` |
| `s10.m4`, `s1d.m4`, `s5d.m4`, `s5k.m4`, `s8d.m4`, `s8k.m4` | Various Sun configs | `name_` |
| `so8.m4`, `sod.m4`, `smd.m4`, `smp.m4` | Various Sun/Solaris configs | `name_` |
| `ult.m4`, `uld.m4` | Ultrix | `name_` |
| `in8.m4`, `ind.m4`, `ins.m4` | Intel configs | `name_` |
| `ln8.m4`, `lnd.m4` | Linux variants | `name_` |

For most modern systems, either `linux` or `darwin` is the correct choice and is auto-detected.

## Testing

### Quick smoke tests

```bash
./ctest      # Should print "maxdiff = 0.000000"
./ftest      # No output means success
./cxxtest    # Should print "maxdiff = 0.000000"
```

All three read `test.gmx`, compress it to XDR, decompress, and verify the round-trip; `ftest` and `cxxtest` do what `ctest` does through the Fortran and C++ interfaces respectively. Each writes to its own scratch files (`test.xdr`/`test.out`, `test_f.xdr`/`test_f.out`, `test_cpp.xdr`/`test_cpp.out`) so they can be run in any order. The three `.xdr` files should come out byte-identical, which is a useful check that the interfaces agree on the format. `make clean` removes them.

### Full test suite

The `test/` directory contains a comprehensive test suite covering the C, C++, and Fortran APIs:

```bash
make -C test    # or: cd test && make test
```

This runs 32 C tests, 24 C++ tests, and 15 Fortran tests covering XDR primitive round-trips (int, float, double, short, char, string, bool), `xdr3dfcoord` at various sizes and coordinate patterns, RAII and move semantics (C++), exception handling (C++), append mode, multi-file I/O, setpos/getpos, multi-frame trajectories, compression ratio verification, and rejection of malformed input. See [test/README.md](test/README.md) for details.

## C API

The C interface is the core of the library. Include `xdrf.h` and link against `libxdrf.a -lm`.

### Functions

```c
#include <rpc/rpc.h>
#include <rpc/xdr.h>
#include "xdrf.h"

// Open an XDR file. mode is "w" (write/create), "a" (append), or "r" (read).
// Returns a nonzero xdrid on success, 0 on failure.
int xdropen(XDR *xdrs, const char *filename, const char *mode);

// Close an XDR file opened with xdropen. Returns 1 on success.
int xdrclose(XDR *xdrs);

// Read/write compressed 3D coordinates.
// fp:        array of 3*size floats
// size:      number of atoms; on read, the count the frame is expected to
//            hold (0 = accept whatever the file says). Set to the actual
//            count on return.
// precision: multiplier for float→fixed-point conversion (e.g. 1000.0)
// Returns 1 on success, 0 on failure.
int xdr3dfcoord(XDR *xdrs, float *fp, int *size, float *precision);
```

`xdrclose` returns 1 on success and 0 if the stream is NULL or was not opened by `xdropen`. If the library allocated the `XDR` struct (`xdropen` called with `xdrs == NULL`, which is what the Fortran wrappers do) it is freed here; otherwise you keep ownership.

Use standard XDR routines (`xdr_int`, `xdr_float`, `xdr_double`, etc.) for non-coordinate data.

### Reading coordinates safely

On read, `xdr3dfcoord` checks `*size` against the count stored in the file and fails without touching `fp` if they disagree. Passing `*size = 0` disables that check and lets the file decide how many triples to write into `fp` — and the library has no way to know how large `fp` is.

Use 0 only when the atom count is bounded by other means. For a file you did not write yourself, read the count first (it is the first `int` of the frame) and size the buffer from it:

```c
unsigned int pos = xdr_getpos(&xd);
int natoms = 0;
xdr_int(&xd, &natoms);          /* the frame's atom count */
xdr_setpos(&xd, pos);           /* rewind */

float *coords = malloc(natoms * 3 * sizeof(float));
xdr3dfcoord(&xd, coords, &natoms, &precision);
```

The C++ `read_3dfcoord()` does exactly this for you. Everything else the frame specifies — coordinate ranges, compression index, run lengths and the compressed block size — is validated by the library before use, so a corrupt file yields a `0` return rather than a crash.

For frames of 9 atoms or fewer the coordinates are stored uncompressed and no precision value is written, so `*precision` is left untouched on read.

### Example (C)

```c
#include <rpc/rpc.h>
#include <rpc/xdr.h>
#include "xdrf.h"

int main() {
    XDR xd;
    int num_atoms = 100;
    float precision = 1000.0;
    float coords[300]; /* 3 * num_atoms */

    /* ... fill coords ... */

    /* Write */
    xdropen(&xd, "trajectory.xdr", "w");
    xdr_int(&xd, &num_atoms);
    xdr3dfcoord(&xd, coords, &num_atoms, &precision);
    xdrclose(&xd);

    /* Read */
    xdropen(&xd, "trajectory.xdr", "r");
    xdr_int(&xd, &num_atoms);
    xdr3dfcoord(&xd, coords, &num_atoms, &precision);
    xdrclose(&xd);

    return 0;
}
```

Compile with:

```bash
gcc -o myprogram myprogram.c libxdrf.a -lm
```

## C++ API

The C++ interface is a header-only wrapper (`xdrf.hpp`) around the C API. It provides an `xdrf::XdrFile` class with RAII resource management, typed read/write methods, move semantics, and exception-based error handling. It requires C++17.

The C++ API uses `libxdrf.a` internally — `xdrf.hpp` includes `xdrf.h` via `extern "C"`, so you link against the same library.

### Class: `xdrf::XdrFile`

```cpp
#include "xdrf.hpp"

// Construction opens the file; destruction closes it.
// Throws std::runtime_error if the file cannot be opened.
xdrf::XdrFile file("data.xdr", "w");  // "w", "r", or "a"

// Explicit close (optional — destructor handles it).
file.close();

// Query state.
file.is_open();   // bool
file.id();        // int xdrid handle
file.xdr();       // XDR* for advanced/direct C API use
```

### Primitive read/write methods

| Write | Read | Type |
|---|---|---|
| `write_int(int)` | `read_int()` → `int` | integer |
| `write_float(float)` | `read_float()` → `float` | float |
| `write_double(double)` | `read_double()` → `double` | double |
| `write_short(short)` | `read_short()` → `short` | short |
| `write_char(char)` | `read_char()` → `char` | character |
| `write_bool(bool)` | `read_bool()` → `bool` | boolean |
| `write_string(const std::string&)` | `read_string()` → `std::string` | string |

All methods throw `std::runtime_error` on failure.

### 3D coordinate methods

```cpp
// Write from std::vector<float> (size must be a multiple of 3)
file.write_3dfcoord(coords_vec, precision);

// Write from raw pointer
file.write_3dfcoord(float_ptr, num_atoms, precision);

// Read into a new vector (returns {coords, precision})
auto [coords, prec] = file.read_3dfcoord();

// Read into a caller-provided buffer
file.read_3dfcoord(float_ptr, num_atoms, precision);
```

### Position methods

```cpp
unsigned int pos = file.getpos();   // current stream position
file.setpos(pos);                   // seek to position
```

### Example (C++)

```cpp
#include "xdrf.hpp"
#include <vector>

int main() {
    int num_atoms = 100;
    float precision = 1000.0f;
    std::vector<float> coords(num_atoms * 3);

    // ... fill coords ...

    // Write (file closes automatically at end of scope)
    {
        xdrf::XdrFile out("trajectory.xdr", "w");
        out.write_int(num_atoms);
        out.write_3dfcoord(coords, precision);
    }

    // Read
    {
        xdrf::XdrFile in("trajectory.xdr", "r");
        int n = in.read_int();
        auto [result, prec] = in.read_3dfcoord();
    }

    return 0;
}
```

Compile with:

```bash
g++ -std=c++17 -o myprogram myprogram.cpp libxdrf.a -lm
```

## Fortran API

The Fortran interface wraps the C functions via m4-generated wrappers in `libxdrf.a`. Each wrapper adds an `xdrf` prefix and a `ret` return-value argument. The wrappers handle Fortran-to-C string conversion and name mangling automatically (configured per architecture in `conf/*.m4`).

All functions pass an integer `xdrid` (set by `xdrfopen`) to identify the open file.

| Routine | Description |
|---|---|
| `xdrfopen(xdrid, filename, mode, ret)` | Open a file |
| `xdrfclose(xdrid, ret)` | Close a file |
| `xdrf3dfcoord(xdrid, fp, size, precision, ret)` | Compressed 3D coordinates |
| `xdrfint(xdrid, ip, ret)` | Read/write integer |
| `xdrffloat(xdrid, fp, ret)` | Read/write float |
| `xdrfdouble(xdrid, dp, ret)` | Read/write double |
| `xdrfstring(xdrid, sp, maxsize, ret)` | Read/write string, max length `maxsize` |
| `xdrfwrapstring(xdrid, sp, ret)` | Read/write string, length inferred |
| `xdrfbool(xdrid, bp, ret)` | Read/write boolean |
| `xdrfchar(xdrid, cp, ret)` | Read/write character |
| `xdrfuchar(xdrid, ucp, ret)` | Read/write unsigned character |
| `xdrflong(xdrid, lp, ret)` | Read/write long |
| `xdrfulong(xdrid, ulp, ret)` | Read/write unsigned long |
| `xdrfshort(xdrid, sp, ret)` | Read/write short |
| `xdrfushort(xdrid, usp, ret)` | Read/write unsigned short |
| `xdrfopaque(xdrid, cp, ccnt, ret)` | Read/write `ccnt` bytes verbatim, no conversion |
| `xdrfvector(xdrid, cp, size, xdrfproc, ret)` | Read/write an array via one of the routines above |
| `xdrfsetpos(xdrid, pos, ret)` | Seek to a byte offset |
| `xdrf(xdrid, pos)` | Return the current byte offset in `pos` |

In all cases, `ret` is 1 on success and 0 on failure. `xdrf` is the exception: it is named just `xdrf`, and takes no `ret`.

`xdrfvector` takes the name of another routine (e.g. `xdrfdouble`) as `xdrfproc` and applies it to each of `size` elements; unlike the C version you do not pass an element size, and `xdrfstring` cannot be used this way. `xdrfopaque` copies bytes without byte swapping, so opaque data is only portable between machines that agree on its layout.

For `xdrf3dfcoord` on read, `size` is the number of triplets the frame is expected to hold, and a disagreement with the file is an error. Passing 0 accepts whatever count the file specifies, which is only safe when `fp` is known to be large enough — see the warning in the [C API](#c-api) section.

## Linking with CAMPARI

After building `libxdrf.a`, point your CAMPARI build to this directory so the linker can find it. Consult the CAMPARI documentation for the specific flag (typically `-L/path/to/xdrf -lxdrf`).

## File layout

```
libxdrf.m4    — Main library source (m4 template)
xdrf.h        — C header
xdrf.hpp      — C++ header-only wrapper (requires C++17)
ftocstr.c     — Fortran-to-C string helpers
xdrf.man      — Man page, section 3 (man ./xdrf.man)
ctest.c       — C smoke test (3dfcoord round-trip)
cxxtest.cpp   — C++ smoke test (same round-trip via XdrFile)
ftest.f       — Fortran smoke test
test.gmx      — Test coordinate data
Makefile      — Build script (auto-detects macOS/Linux)
conf/         — m4 architecture configs (linux.m4, darwin.m4, ...)
test/         — Comprehensive test suite (C, C++, and Fortran)
about.md      — Background on the compression algorithm
LICENSE       — MIT license
```

`libxdrf.c` is generated from `libxdrf.m4` by the Makefile and is not source; `make clean` removes it.

## Related

You may also be interested in [xtclib](https://github.com/holehouse-lab/xtclib), a separate, stand-alone Python compatible, C/Cython implementation which is useful as a lightweight plug-n-play for Python-centric XTC manipulation, especially in the context of trajectory-related things.

## Issues

If you run into problems, please open a GitHub issue. If you don't get a response within a week, email Alex at alex.holehouse@wustl.edu.

## License

MIT License. See [LICENSE](LICENSE).

## Changelog

#### V 1.6 (Jul 2026)

Bug fixes, mostly memory safety. The on-disk format is unchanged: for real trajectory data this version and 1.5 produce byte-identical files and read each other's output identically. See the compatibility note at the end of this entry for the one degenerate case that behaves differently.

* Fixed a crash when a caller-owned and a library-owned `XDR` struct were open at the same time. Ownership was recorded in a single shared slot that every `xdropen` overwrote, so it only ever described the most recently opened file: closing a C-opened file while a Fortran-opened one was open called `free()` on the caller's struct (an abort, if it was on the stack), and closing them in the other order leaked. Ownership is now tracked per file handle.
* Fixed a heap buffer overflow in `xdrfstring`, which sized its scratch buffer from the Fortran string length but let `xdr_string` write up to `maxsize` bytes into it. `xdrfwrapstring` had the same bug off by one.
* Hardened the `xdr3dfcoord` read path against malformed files. The atom count, coordinate ranges, compression index, run lengths and compressed block size were all used unvalidated; a corrupt file could index the `magicints` table out of bounds, divide by zero, or write past the end of both the internal and the caller's buffers. All are now checked and rejected with a `0` return.
* A `size` that disagrees with the count in the file is now an error rather than a warning followed by decoding into the caller's undersized buffer.
* Fixed an out-of-bounds read of `magicints[]` on the write path: several places clamped an index to the table's element count instead of the last valid index.
* Fixed `xdrfshort` advancing the `xdrfvector` cursor by `sizeof(short *)` (8) rather than `sizeof(short)` (2), which misread every element after the first of a short array.
* Fixed `xdrfopen` recording a string-conversion failure in `ret` and then overwriting it, silently opening a truncated filename.
* Fixed C++ `read_3dfcoord()` allocating a fixed 30000-float buffer and then letting the file decide how much to write into it, overflowing for frames above 10000 atoms. It now reads the atom count from the frame and sizes the buffer to match.
* `xdrclose` and `xdr3dfcoord` now return 0 on a bad handle instead of calling `exit(1)` and taking the calling program down with them.
* Replaced `xdr_vector(..., (xdrproc_t)xdr_float)` with a direct loop; the cast called a two-argument function through a three-argument pointer type, which traps under UBSan and CFI.
* Fixed undefined signed-integer overflow when reconstructing coordinates and coordinate ranges from file values, and an undefined `1 << 32` in `receivebits`.
* `xdropen` now checks its `malloc`, and the coordinate buffers no longer leak when `realloc` fails.
* `ctest`, `cxxtest` and `ftest` opened their output in append mode, so every run stacked another copy of the data onto the file. They now truncate, and `ftest` writes to `test_f.*` instead of colliding with `ctest`'s files.
* `make clean` now removes generated `libxdrf.c` and the smoke-test scratch files.
* Removed the two case-colliding config files, `conf/CM5.m4` and `conf/SGI.m4`. Git tracked them separately from `conf/cm5.m4` and `conf/sgi.m4`, but on macOS and Windows each pair is a single file, so the repository could not be checked out correctly there — two files always arrived with the wrong contents and showed as modified. The contents of each pair were identical, so nothing is lost, and the build now falls back to the lowercase spelling when an uppercase `ARCH` has no config of its own (`make ARCH=SGI` still works).
* Stopped tracking build products that were committed to the repository: the generated `libxdrf.c`, the three compiled `test/test_*` binaries, and `test_cpp.xdr`. All are produced by `make` and are now in `.gitignore`.
* Added regression tests for the above; documented the `size = 0` hazard in `xdrf.h`, the man page and this README; documented the six Fortran routines (`xdrfuchar`, `xdrfushort`, `xdrfulong`, `xdrfopaque`, `xdrfsetpos`, `xdrf`) that existed but appeared in neither.

**Compatibility note.** The one behavioural difference from 1.5 is in the out-of-bounds regime described above. When the minimum Manhattan separation between consecutive atoms exceeded `magicints[64]` (2097152 scaled units), 1.5 selected a compression index of 73 and encoded the frame using the value one past the end of that 73-entry table — whatever happened to follow it in memory. Such a frame only decoded correctly when read by the same binary that wrote it, so 1.6 refuses it rather than guessing, and never writes one. Reaching this needs a minimum inter-atom separation of about 2097 nm at the conventional precision of 1000; real MD data sits around index 21, and everything below index 65 is byte-for-byte identical to 1.5.

#### V 1.5 (Apr 2026)
* Converted all K&R function definitions to ANSI C prototypes (C23 compatible) in `libxdrf.m4` and `ftocstr.c`.
* Fixed `xdrstdio_create` undeclared error on macOS by adding conditional declaration in `xdrf.h`.
* Fixed `long *` / `unsigned long *` pointer type mismatches for 64-bit macOS (arm64).
* Fixed `unsigned char *` / `char *` mismatch in `xdrfuchar`.
* Fixed `MAXABS` implicit int-to-float conversion warning.
* Added `conf/darwin.m4` for macOS (x86\_64 and arm64).
* Added auto-detection of macOS vs Linux in `Makefile` (via `uname -s`).
* Added comprehensive test suite in `test/` with 29 C tests, 24 C++ tests, and 15 Fortran tests covering all API functions.
* Added C++ header-only wrapper (`xdrf.hpp`) providing RAII-based `xdrf::XdrFile` class with typed methods, move semantics, and exception handling.
* Updated all 66 architecture config files in `conf/` to use ANSI-compatible macros:
  - Groups 1–4 (61 files): Moved `char *` / `int` types from `STRING_ARG_DECL` into `STRING_ARG` inline.
  - Group 5 (`CRAY.m4`, `CRAY2.m4`): Moved `_fcd` type into `STRING_ARG`.
  - Group 7 (`TITN.m4`): Moved `FSD *` type into `STRING_ARG`.
  - Group 8 (`U370.m4`): Moved `char *` / `int *` types into `STRING_ARG`.
* Rewrote `README.md` with full API documentation and supported architectures table.

#### V 1.4 (Jan 2024)
* Commented out the stub definition of `xdrstdio_create()` as this is now generally provided by `/usr/include/rpc/xdr.h` (part of `libtirpc-dev` / `libntirpc-dev`).

#### V 1.3
* Fixed memory leak in Fortran interface (xdridptr flag at index 20). Superseded in 1.6 by per-handle ownership tracking.

* Thanks to @sodiumnitrate for identifying issue(s) with the current implementation, which hasn't been update in ~8 years!

* Changed default compilers to `gcc` and `gfortran` - in general Intel compilers are superior for CAMPARI but it's often convenient to get things working in a GNU universe first

* Updated makefile to work on macOS using gfortran and gcc (update `RMCMD` to be the $PATH dependent `true` function although full paths of macOS and Linux also offered if needed - just need to edit Makefile.

* Updated includes to remove `#include <malloc.h>` which is depricated and reply on `#include <stdlib.h>`. Also added `#include <string.h>` which is needed for `strcpy()` to be called - not clear how things worked without this...

* Added stub declaration for `xdrstdio_create()` in `xdrf.h` - implicit function declaration as no longer allowed

* Tested so far only on macOS 12.2.1 but works. Will test on a Linuxbox when I'm back in US!

#### V 1.2
* Memory error patch for CAMPARI compatibility.

## Credits
(C) 1995 Frans van Hoesel (original text and code - see `about.md` for more details on the original implementation.

Updated and maintained by [Alex Holehouse](https://holehouselab.com/), 2014-2026. 

hoesel@chem.rug.nl 

alex.holehouse@wustl.edu 
