# libxdrf

A portable C/Fortran library for reading and writing XDR (External Data Representation) data, with built-in lossy compression for 3D coordinates. Originally developed for the EUROPORT project by Frans van Hoesel, this version is maintained with bug fixes and modern compiler support.

## Overview

`libxdrf` solves two problems common in molecular dynamics simulations:

1. **Portability** — Binary data written on one architecture (e.g. big-endian SPARC) can be read on another (e.g. little-endian x86) using the XDR standard.
2. **Compression** — The `xdr3dfcoord` routine compresses 3D floating-point coordinates to ~30% of their uncompressed binary size, exploiting spatial locality in molecular data.

The library provides both a C API (three functions) and a Fortran API (via m4-generated wrappers). It is commonly used as a build dependency for [CAMPARI](http://campari.sourceforge.net/).

## Requirements

- **C compiler**: `gcc` or `clang`
- **Fortran compiler** (optional, for Fortran tests): `gfortran`
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

- `libxdrf.a` — the static library
- `ctest` — C test program
- `ftest` — Fortran test program

### Compiler options

To use different compilers (e.g. Intel), edit the `CC` and `F77` variables in the `Makefile`:

```makefile
CC  = icc
F77 = ifort
```

### Architecture override

The build auto-detects macOS vs Linux. To force a specific architecture config from the `conf/` directory:

```bash
make ARCH=linux
```

### Supported architectures

The `conf/` directory contains m4 configuration files for many platforms. Each config defines how Fortran function names are mangled and how strings are passed across the C/Fortran boundary.

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

```bash
./ctest    # Should print "maxdiff = 0.000000"
./ftest    # No output means success
```

`ctest` reads `test.gmx`, compresses it to `test.xdr`, decompresses to `test.out`, and verifies the round-trip produces identical coordinates.

## C API

Include `xdrf.h` and link against `libxdrf.a -lm`.

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
// size:      number of atoms (read back on decode)
// precision: multiplier for float→fixed-point conversion (e.g. 1000.0)
// Returns 1 on success, 0 on failure.
int xdr3dfcoord(XDR *xdrs, float *fp, int *size, float *precision);
```

Use standard XDR routines (`xdr_int`, `xdr_float`, `xdr_double`, etc.) for non-coordinate data.

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

## Fortran API

The Fortran interface wraps the C functions, adding an `xdrf` prefix and a `ret` return-value argument. All functions pass an integer `xdrid` (set by `xdrfopen`) to identify the open file.

| Routine | Description |
|---|---|
| `xdrfopen(xdrid, filename, mode, ret)` | Open a file |
| `xdrfclose(xdrid, ret)` | Close a file |
| `xdrf3dfcoord(xdrid, fp, size, precision, ret)` | Compressed 3D coordinates |
| `xdrfint(xdrid, ip, ret)` | Read/write integer |
| `xdrffloat(xdrid, fp, ret)` | Read/write float |
| `xdrfdouble(xdrid, dp, ret)` | Read/write double |
| `xdrfstring(xdrid, sp, maxsize, ret)` | Read/write string |
| `xdrfbool(xdrid, bp, ret)` | Read/write boolean |
| `xdrfchar(xdrid, cp, ret)` | Read/write character |
| `xdrflong(xdrid, lp, ret)` | Read/write long |
| `xdrfshort(xdrid, sp, ret)` | Read/write short |

In all cases, `ret` is 1 on success and 0 on failure.

## Linking with CAMPARI

After building `libxdrf.a`, point your CAMPARI build to this directory so the linker can find it. Consult the CAMPARI documentation for the specific flag (typically `-L/path/to/xdrf -lxdrf`).

## File layout

```
libxdrf.m4    — Main library source (m4 template)
xdrf.h        — C header
ftocstr.c     — Fortran-to-C string helpers
ctest.c       — C test program
ftest.f       — Fortran test program
test.gmx      — Test coordinate data
Makefile      — Build script (auto-detects macOS/Linux)
conf/         — m4 architecture configs (linux.m4, darwin.m4, ...)
Intro.txt     — Background on the compression algorithm
```

## Issues

If you run into problems, please open a GitHub issue. If you don't get a response within a week, email Alex at alex.holehouse@wustl.edu.

## License

MIT License. See [LICENSE](LICENSE).

## Changelog

#### V 1.5 (Apr 2026)
* Converted all K&R function definitions to ANSI C prototypes (C23 compatible) in `libxdrf.m4` and `ftocstr.c`.
* Fixed `xdrstdio_create` undeclared error on macOS by adding conditional declaration in `xdrf.h`.
* Fixed `long *` / `unsigned long *` pointer type mismatches for 64-bit macOS (arm64).
* Fixed `unsigned char *` / `char *` mismatch in `xdrfuchar`.
* Fixed `MAXABS` implicit int-to-float conversion warning.
* Added `conf/darwin.m4` for macOS (x86\_64 and arm64).
* Added auto-detection of macOS vs Linux in `Makefile` (via `uname -s`).
* Updated all 66 architecture config files in `conf/` to use ANSI-compatible macros:
  - Groups 1–4 (61 files): Moved `char *` / `int` types from `STRING_ARG_DECL` into `STRING_ARG` inline.
  - Group 5 (`CRAY.m4`, `CRAY2.m4`): Moved `_fcd` type into `STRING_ARG`.
  - Group 7 (`TITN.m4`): Moved `FSD *` type into `STRING_ARG`.
  - Group 8 (`U370.m4`): Moved `char *` / `int *` types into `STRING_ARG`.
* Rewrote `README.md` with full API documentation and supported architectures table.

#### V 1.4 (Jan 2024)
* Commented out the stub definition of `xdrstdio_create()` as this is now generally provided by `/usr/include/rpc/xdr.h` (part of `libtirpc-dev` / `libntirpc-dev`).

#### V 1.3
* Fixed memory leak in Fortran interface (xdridptr flag at index 20).

#### V 1.2
* Memory error patch for CAMPARI compatibility.

#### V 1.3
* Thanks to @sodiumnitrate for identifying issue(s) with the current implementation, which hasn't been update in ~8 years!

* Changed default compilers to `gcc` and `gfortran` - in general Intel compilers are superior for CAMPARI but it's often convenient to get things working in a GNU universe first

* Updated makefile to work on macOS using gfortran and gcc (update `RMCMD` to be the $PATH dependent `true` function although full paths of macOS and Linux also offered if needed - just need to edit Makefile.

* Updated includes to remove `#include <malloc.h>` which is depricated and reply on `#include <stdlib.h>`. Also added `#include <string.h>` which is needed for `strcpy()` to be called - not clear how things worked without this...

* Added stub declaration for `xdrstdio_create()` in `xdrf.h` - implicit function declaration as no longer allowed

* Tested so far only on macOS 12.2.1 but works. Will test on a Linuxbox when I'm back in US!

## Credits
(c) 1995 Frans van Hoesel (original text and code)

Updated by Alex Holehouse, 2014-2022

hoesel@chem.rug.nl

alex.holehouse@wustl.edu 
