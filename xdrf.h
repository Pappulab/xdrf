/*_________________________________________________________________
 |
 | xdrf.h - Public C header for the xdrf library.
 |
 | Include this file in any C program that uses xdropen(), xdrclose(),
 | or xdr3dfcoord(). You must also include <rpc/rpc.h> and <rpc/xdr.h>
 | before this header so that the XDR type is defined.
 |
 | For standard XDR primitives (xdr_int, xdr_float, xdr_double, etc.)
 | use the system <rpc/xdr.h> functions directly.
 |
*/


// On Linux, xdrstdio_create is declared in <rpc/xdr.h> via libntirpc.
// On macOS, the declaration is missing from the system headers, so we
// provide it here. The function itself exists in the system RPC library.
#if defined(__APPLE__) || defined(darwin)
#include <stdio.h>
extern void xdrstdio_create(XDR *xdrs, FILE *file, enum xdr_op op);
#endif


/*
 * xdropen - Open an XDR file for reading or writing.
 *
 * This replaces xdrstdio_create and must be used instead of it when
 * using xdr3dfcoord(), because xdropen tracks the file mode (read/write)
 * and the file descriptor internally.
 *
 * Parameters:
 *   xdrs     - Pointer to a caller-allocated XDR struct, or NULL.
 *              If NULL (used by the Fortran interface), the library
 *              allocates the XDR struct internally and frees it on close.
 *   filename - Path to the file to open.
 *   type     - "w" or "W" for write, "a" or "A" for append, anything
 *              else for read.
 *
 * Returns: A nonzero xdrid handle on success, 0 on failure.
 */
int xdropen(XDR *xdrs, const char *filename, const char *type);

/*
 * xdrclose - Close an XDR file opened with xdropen.
 *
 * Flushes XDR buffers, destroys the XDR stream, and closes the
 * underlying file descriptor. Do not call xdr_destroy separately.
 *
 * Returns: 1 on success, exits on error.
 */
int xdrclose(XDR *xdrs) ;

/*
 * xdr3dfcoord - Read or write compressed 3D coordinates.
 *
 * Compresses floating-point 3D coordinates for efficient storage.
 * Coordinates are converted to fixed-point integers by multiplying
 * by *precision and rounding. The compression exploits spatial
 * locality and small inter-atom differences (especially effective
 * for water molecules in MD simulations).
 *
 * Parameters:
 *   xdrs      - XDR stream opened with xdropen.
 *   fp        - Array of 3 * (*size) floats (x,y,z triples).
 *   size      - On write: number of coordinate triples.
 *               On read: set to 0 to read whatever was written,
 *               or set to expected count for validation.
 *   precision - Multiplier for float-to-int conversion (e.g. 1000.0
 *               gives 3 decimal places of precision).
 *
 * Returns: 1 on success, 0 on failure.
 */
int xdr3dfcoord(XDR *xdrs, float *fp, int *size, float *precision) ;

