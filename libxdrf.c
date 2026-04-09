
/*____________________________________________________________________________
 |
 | libxdrf - portable fortran interface to xdr. some xdr routines
 |	     are C routines for compressed coordinates
 |
 | version 1.1
 |
 | This collection of routines is intended to write and read
 | data in a portable way to a file, so data written on one type
 | of machine can be read back on a different type.
 |
 | all fortran routines use an integer 'xdrid', which is an id to the
 | current xdr file, and is set by xdrfopen.
 | most routines have in integer 'ret' which is the return value.
 | The value of 'ret' is zero on failure, and most of the time one
 | on succes.
 |
 | There are three routines useful for C users:
 |  xdropen(), xdrclose(), xdr3dfcoord().
 | The first two replace xdrstdio_create and xdr_destroy, and *must* be
 | used when you plan to use xdr3dfcoord(). (they are also a bit
 | easier to interface). For writing data other than compressed coordinates 
 | you should use the standard C xdr routines (see xdr man page)
 |
 | xdrfopen(xdrid, filename, mode, ret)
 |	character *(*) filename
 |	character *(*) mode
 |
 |	this will open the file with the given filename (string)
 |	and the given mode, it returns an id in xdrid, which is
 |	to be used in all other calls to xdrf routines.
 |	mode is 'w' to create, or update an file, for all other
 |	values of mode the file is opened for reading
 |
 |	you need to call xdrfclose to flush the output and close
 |	the file.
 |	Note that you should not use xdrstdio_create, which comes with the
 |	standard xdr library
 |
 | xdrfclose(xdrid, ret)
 |	flush the data to the file, and closes the file;
 |	You should not use xdr_destroy (which comes standard with
 |	the xdr libraries.
 |
 | xdrfbool(xdrid, bp, ret)
 |	integer pb
 |
 | 	This filter produces values of either 1 or 0	
 |
 | xdrfchar(xdrid, cp, ret)
 |	character cp
 |
 |	filter that translate between characters and their xdr representation
 |	Note that the characters in not compressed and occupies 4 bytes.
 |
 | xdrfdouble(xdrid, dp, ret)
 |	double dp
 |
 |	read/write a double.
 |
 | xdrffloat(xdrid, fp, ret)
 |	float fp
 |
 |	read/write a float.
 |
 | xdrfint(xdrid, ip, ret)
 |	integer ip
 |
 |	read/write integer.
 |
 | xdrflong(xdrid, lp, ret)
 |	integer lp
 |
 |	this routine has a possible portablility problem due to 64 bits longs.
 |
 | xdrfshort(xdrid, sp, ret)
 |	integer *2 sp
 |
 | xdrfstring(xdrid, sp, maxsize, ret)
 |	character *(*)
 |	integer maxsize
 |
 |	read/write a string, with maximum length given by maxsize
 |
 | xdrfwrapstring(xdris, sp, ret)
 |	character *(*)
 |
 |	read/write a string (it is the same as xdrfstring accept that it finds
 |	the stringlength itself.
 |
 | xdrfvector(xdrid, cp, size, xdrfproc, ret)
 |	character *(*)
 |	integer size
 |	external xdrfproc
 |
 |	read/write an array pointed to by cp, with number of elements
 |	defined by 'size'. the routine 'xdrfproc' is the name
 |	of one of the above routines to read/write data (like xdrfdouble)
 |	In contrast with the c-version you don't need to specify the
 |	byte size of an element.
 |	xdrfstring is not allowed here (it is in the c version)
 |	
 | xdrf3dfcoord(xdrid, fp, size, precision, ret)
 |	real (*) fp
 |	real precision
 |	integer size
 |
 |	this is *NOT* a standard xdr routine. I named it this way, because
 |	it invites people to use the other xdr routines.
 | 	It is introduced to store specifically 3d coordinates of molecules
 |	(as found in molecular dynamics) and it writes it in a compressed way.
 |	It starts by multiplying all numbers by precision and
 |	rounding the result to integer. effectively converting
 |	all floating point numbers to fixed point.
 |	it uses an algorithm for compression that is optimized for
 |	molecular data, but could be used for other 3d coordinates
 |	as well. There is subtantial overhead involved, so call this
 |	routine only if you have a large number of coordinates to read/write
 |
 | ________________________________________________________________________
 |
 | Below are the routines to be used by C programmers. Use the 'normal'
 | xdr routines to write integers, floats, etc (see man xdr)	
 |
 | int xdropen(XDR *xdrs, const char *filename, const char *type)
 |	This will open the file with the given filename and the 
 |	given mode. You should pass it an allocated XDR struct
 |	in xdrs, to be used in all other calls to xdr routines.
 |	Mode is 'w' to create, or update an file, and for all 
 |	other values of mode the file is opened for reading. 
 |	You need to call xdrclose to flush the output and close
 |	the file.
 |
 |	Note that you should not use xdrstdio_create, which
 |	comes with the standard xdr library.
 |
 | int xdrclose(XDR *xdrs)
 |	Flush the data to the file, and close the file;
 |	You should not use xdr_destroy (which comes standard
 |	with the xdr libraries).
 |	 
 | int xdr3dfcoord(XDR *xdrs, float *fp, int *size, float *precision)
 |	This is \fInot\fR a standard xdr routine. I named it this 
 |	way, because it invites people to use the other xdr 
 |	routines.
 |
 |	(c) 1995 Frans van Hoesel, hoesel@chem.rug.nl
*/	



#include <limits.h>
#include <math.h>
#include <string.h>
#include <rpc/rpc.h>
#include <rpc/xdr.h>
#include <stdio.h>
#include <stdlib.h>
#include "xdrf.h"

/* Forward declarations for Fortran-to-C string conversion helpers
 * (implemented in ftocstr.c). These convert between Fortran's
 * fixed-length space-padded strings and C's null-terminated strings.
 */
int ftocstr(char *, int, char *, int);
int ctofstr(char *, int, char *);

/* Maximum number of simultaneously open XDR files */
#define MAXID 20

/* Size of the xdridptr array: MAXID slots for file handles plus
 * one extra slot (index 20) used as a flag to track whether the
 * XDR struct was allocated by Fortran (needs freeing) or C (caller
 * manages memory). See xdropen() and xdrclose() for details.
 */
#define FORTRANTEST 21


/* By setting the length of the xdridptr array to 21 we reserve the 
   position at index 20 as a flag to set if the xdrid structure
   was generated by a FORTRAN call (in which case libxdrf must
   free the memory) or a C call (in which case the calling
   program is responsible for memory freeing). This fixes a massive
   memory leak in the FORTRAN implementation.

   ~alex [alex.holehouse@wustl.edu]   
*/

/* Global state for managing open XDR file handles.
 *
 * xdrfiles[]  - FILE pointers for each open XDR file (indexed by xdrid).
 * xdridptr[]  - XDR stream pointers. Slots 1..MAXID-1 hold active streams.
 *               Slot MAXID (index 20) is a flag: non-NULL means the library
 *               allocated the XDR struct and must free it on close.
 * xdrmodes[]  - The open mode character ('w', 'a', or 'r') for each file.
 * cnt         - Running byte counter used by the Fortran wrapper functions
 *               to track offsets within xdrfvector() iterations.
 */
static FILE *xdrfiles[MAXID];
static XDR *xdridptr[FORTRANTEST]; 
static char xdrmodes[MAXID];
static unsigned int cnt;

/* Function pointer type for the Fortran element-processing callback
 * used by xdrfvector. The callback signature is:
 *   void func(int *xdrid, void *data, int *ret)
 */
typedef void (* xdrfproc_) (int *, void *, int *);

/*___________________________________________________________________________
 |
 | Fortran wrapper functions
 |
 | These functions provide the Fortran-callable interface to XDR routines.
 | Each takes an xdrid (file handle index), the data to read/write, and
 | a ret pointer that receives 1 on success or 0 on failure.
 |
 | The _ and () macros are expanded by m4 using the
 | architecture-specific config files in the conf directory to handle Fortran name
 | mangling (e.g. appending underscores) and string passing conventions.
 |
*/

/* xdrfbool - Read/write a boolean value.
 * The boolean is stored as a 4-byte XDR integer (0 or 1).
 */
void
xdrfbool_ (int *xdrid, int *pb, int *ret)
{
	*ret = xdr_bool(xdridptr[*xdrid], (bool_t *) pb);
	cnt += sizeof(int);
}

/* xdrfchar - Read/write a single character.
 * Note: XDR stores characters in 4 bytes (not compressed).
 */
void
xdrfchar_ (int *xdrid, char *cp, int *ret)
{
	*ret = xdr_char(xdridptr[*xdrid], cp);
	cnt += sizeof(char);
}

/* xdrfdouble - Read/write a double-precision floating point value. */
void
xdrfdouble_ (int *xdrid, double *dp, int *ret)
{
	*ret = xdr_double(xdridptr[*xdrid], dp);
	cnt += sizeof(double);
}

/* xdrffloat - Read/write a single-precision floating point value. */
void
xdrffloat_ (int *xdrid, float *fp, int *ret)
{
	*ret = xdr_float(xdridptr[*xdrid], fp);
	cnt += sizeof(float);
}

/* xdrfint - Read/write a 32-bit integer. */
void
xdrfint_ (int *xdrid, int *ip, int *ret)
{
	*ret = xdr_int(xdridptr[*xdrid], ip);
	cnt += sizeof(int);
}

/* xdrflong - Read/write a long integer.
 * On macOS (arm64/x86_64), long is 8 bytes but xdr_long() expects int*
 * (4 bytes), so we use a temporary int to avoid pointer type mismatch.
 */
void
xdrflong_ (int *xdrid, long *lp, int *ret)
{
#ifdef __APPLE__
	int tmp = (int)*lp;
	*ret = xdr_long(xdridptr[*xdrid], &tmp);
	*lp = tmp;
#else
	*ret = xdr_long(xdridptr[*xdrid], lp);
#endif
	cnt += sizeof(long);
}

/* xdrfshort - Read/write a 16-bit short integer. */
void
xdrfshort_ (int *xdrid, short *sp, int *ret)
{
	*ret = xdr_short(xdridptr[*xdrid], sp);
	cnt += sizeof(sp);
}

/* xdrfuchar - Read/write an unsigned character. */
void
xdrfuchar_ (int *xdrid, unsigned char *ucp, int *ret)
{
	*ret = xdr_u_char(xdridptr[*xdrid], ucp);
	cnt += sizeof(char);
}

/* xdrfulong - Read/write an unsigned long integer.
 * Same macOS workaround as xdrflong (see above).
 */
void
xdrfulong_ (int *xdrid, unsigned long *ulp, int *ret)
{
#ifdef __APPLE__
	unsigned int tmp = (unsigned int)*ulp;
	*ret = xdr_u_long(xdridptr[*xdrid], &tmp);
	*ulp = tmp;
#else
	*ret = xdr_u_long(xdridptr[*xdrid], ulp);
#endif
	cnt += sizeof(unsigned long);
}

/* xdrfushort - Read/write an unsigned 16-bit short integer. */
void
xdrfushort_ (int *xdrid, unsigned short *usp, int *ret)
{
	*ret = xdr_u_short(xdridptr[*xdrid], usp);
	cnt += sizeof(unsigned short);
}

/* xdrf3dfcoord - Read/write compressed 3D coordinates (Fortran interface).
 * Delegates to the C xdr3dfcoord() function. See that function's
 * documentation for details on the compression algorithm.
 */
void 
xdrf3dfcoord_ (int *xdrid, float *fp, int *size, float *precision, int *ret)
{
	*ret = xdr3dfcoord(xdridptr[*xdrid], fp, size, precision);
}

/* xdrfstring - Read/write a string with a specified maximum length.
 * Converts from Fortran string format (fixed-length, space-padded)
 * to C string format (null-terminated) before passing to xdr_string,
 * then converts the result back to Fortran format.
 */
void
xdrfstring_ (int *xdrid, char *sp_ptr, int *maxsize, int *ret, int sp_len)
{
	char *tsp;

	tsp = (char*) malloc(((sp_len) + 1) * sizeof(char));
	if (tsp == NULL) {
	    *ret = -1;
	    return;
	}
	if (ftocstr(tsp, *maxsize+1, sp_ptr, sp_len)) {
	    *ret = -1;
	    free(tsp);
	    return;
	}
	*ret = xdr_string(xdridptr[*xdrid], (char **) &tsp, (u_int) *maxsize);
	ctofstr( sp_ptr, sp_len, tsp);
	cnt += *maxsize;
	free(tsp);
}

/* xdrfwrapstring - Read/write a string, automatically determining its length.
 * Like xdrfstring but infers the max size from the Fortran string length
 * rather than requiring an explicit maxsize parameter.
 */
void
xdrfwrapstring_ (int *xdrid, char *sp_ptr, int *ret, int sp_len)
{
	char *tsp;
	int maxsize;
	maxsize = (sp_len) + 1;
	tsp = (char*) malloc(maxsize * sizeof(char));
	if (tsp == NULL) {
	    *ret = -1;
	    return;
	}
	if (ftocstr(tsp, maxsize, sp_ptr, sp_len)) {
	    *ret = -1;
	    free(tsp);
	    return;
	}
	*ret = xdr_string(xdridptr[*xdrid], (char **) &tsp, (u_int)maxsize);
	ctofstr( sp_ptr, sp_len, tsp);
	cnt += maxsize;
	free(tsp);
}

/* xdrfopaque - Read/write opaque (untyped) data of a fixed byte count.
 * The data is written as-is without any type conversion.
 */
void
xdrfopaque_ (int *xdrid, caddr_t *cp, int *ccnt, int *ret)
{
	*ret = xdr_opaque(xdridptr[*xdrid], (caddr_t)*cp, (u_int)*ccnt);
	cnt += *ccnt;
}

/* xdrfsetpos - Set the current position in the XDR stream.
 * Allows seeking to a specific byte offset for random access.
 */
void
xdrfsetpos_ (int *xdrid, int *pos, int *ret)
{
	*ret = xdr_setpos(xdridptr[*xdrid], (u_int) *pos);
}

/* xdrf - Get the current position in the XDR stream.
 * Returns the current byte offset in *pos.
 */
void
xdrf_ (int *xdrid, int *pos)
{
	*pos = xdr_getpos(xdridptr[*xdrid]);
}

/* xdrfvector - Read/write an array of elements using a callback.
 * Iterates over *size elements, calling elproc for each one.
 * The element-processing function (e.g. xdrffloat, xdrfdouble)
 * is passed as a function pointer. The global 'cnt' variable
 * tracks the byte offset so each element is read from the
 * correct position in the buffer.
 * Note: xdrfstring cannot be used as the element procedure.
 */
void
xdrfvector_ (int *xdrid, char *cp, int *size, xdrfproc_ elproc, int *ret)
{
	int lcnt;
	cnt = 0;
	for (lcnt = 0; lcnt < *size; lcnt++) {
		elproc(xdrid, (cp+cnt) , ret);
	}
}


/* xdrfclose - Close an XDR file (Fortran interface).
 * Delegates to xdrclose() and resets the byte counter.
 */
void
xdrfclose_ (int *xdrid, int *ret)
{
	*ret = xdrclose(xdridptr[*xdrid]);
	cnt = 0;
}

/* xdrfopen - Open an XDR file (Fortran interface).
 * Converts Fortran strings (filename and mode) to C strings,
 * then calls xdropen() with xdrs=NULL so the library allocates
 * the XDR struct. Returns the xdrid handle and sets ret to 1
 * on success, 0 on failure.
 */
void
xdrfopen_ (int *xdrid, char *fp_ptr, char *mode_ptr, int *ret, int fp_len, int mode_len)
{
	char fname[512];
	char fmode[3];

	if (ftocstr(fname, sizeof(fname), fp_ptr, fp_len)) {
		*ret = 0;
	}
	if (ftocstr(fmode, sizeof(fmode), mode_ptr,
			mode_len)) {
		*ret = 0;
	}

	*xdrid = xdropen(NULL, fname, fmode);
	if (*xdrid == 0)
		*ret = 0;
	else 
		*ret = 1;	
}

/*___________________________________________________________________________
 |
 | what follows are the C routines for opening, closing xdr streams
 | and the routine to read/write compressed coordinates together
 | with some routines to assist in this task (those are marked
 | static and cannot be called from user programs)
*/
/* Maximum absolute value for coordinate compression. Coordinates
 * scaled beyond this range will cause an overflow error. Cast to
 * double to avoid implicit int-to-float conversion warnings.
 */
#define MAXABS ((double)(INT_MAX-2))

#ifndef MIN
#define MIN(x,y) ((x) < (y) ? (x):(y))
#endif
#ifndef MAX
#define MAX(x,y) ((x) > (y) ? (x):(y))
#endif
#ifndef SQR
#define SQR(x) ((x)*(x))
#endif
/* Lookup table of "magic" integers used for adaptive compression.
 * These values define the range of the small-integer encoding at
 * each compression level. The compressor dynamically selects a
 * level (index into this table) based on the differences between
 * successive coordinates. Larger indices allow larger differences
 * but use more bits.
 */
static int magicints[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0,
    8, 10, 12, 16, 20, 25, 32, 40, 50, 64,
    80, 101, 128, 161, 203, 256, 322, 406, 512, 645,
    812, 1024, 1290, 1625, 2048, 2580, 3250, 4096, 5060, 6501,
    8192, 10321, 13003, 16384, 20642, 26007, 32768, 41285, 52015, 65536,
    82570, 104031, 131072, 165140, 208063, 262144, 330280, 416127, 524287, 660561,
    832255, 1048576, 1321122, 1664510, 2097152, 2642245, 3329021, 4194304, 5284491, 6658042,
    8388607, 10568983, 13316085, 16777216 };

/* FIRSTIDX: index of the first usable entry in magicints
 * (entries before this are zero and cannot be used for encoding).
 * LASTIDX: one past the last valid index in magicints.
 */
#define FIRSTIDX 9
/* note that magicints[FIRSTIDX-1] == 0 */
#define LASTIDX (sizeof(magicints) / sizeof(*magicints))


/*__________________________________________________________________________
 |
 | xdropen - open xdr file
 |
 | This versions differs from xdrstdio_create, because I need to know
 | the state of the file (read or write) so I can use xdr3dfcoord
 | in eigther read or write mode, and the file descriptor
 | so I can close the file (something xdr_destroy doesn't do).
 |
*/

int xdropen(XDR *xdrs, const char *filename, const char *type) {
    static int init_done = 0;
    enum xdr_op lmode;
    int xdrid;
    char newtype[5];
    
    /* One-time initialization: clear all XDR stream pointers */
    if (init_done == 0) {
	for (xdrid = 1; xdrid < MAXID; xdrid++) {
	    xdridptr[xdrid] = NULL;
	}
	init_done = 1;
    }
    /* Find the first unused slot in the xdridptr table */
    xdrid = 1;
    while (xdrid < MAXID && xdridptr[xdrid] != NULL) {
	xdrid++;
    }
    if (xdrid == MAXID) {
	return 0;
    }
    /* Map the mode character to fopen flags and XDR direction */
    if (*type == 'w' || *type == 'W') {
            strcpy(newtype,"wb+");
	    lmode = XDR_ENCODE;
    } else if (*type == 'a' || *type == 'A') {
            strcpy(newtype,"ab+");
            lmode = XDR_ENCODE;
    } else {
            strcpy(newtype,"rb");
	    lmode = XDR_DECODE;
    }
    xdrfiles[xdrid] = fopen(filename, newtype);
    if (xdrfiles[xdrid] == NULL) {
	xdrs = NULL;
	return 0;
    }
    xdrmodes[xdrid] = *type;
    /* Determine whether this is a Fortran or C call.
     * Fortran calls pass xdrs=NULL, so the library must allocate
     * the XDR struct and mark it for freeing on close (via the
     * sentinel at xdridptr[MAXID]).
     * C calls pass a pre-allocated XDR struct that the caller owns.
     */
    if (xdrs == NULL) {

        xdridptr[MAXID] = (XDR*)1; // Set the pointermask to a none NULL value (DO NOT TRY
                                   // AND DO ANYTHING WITH THIS MEMORY LOCATION!!!!)

	xdridptr[xdrid] = (XDR *) malloc(sizeof(XDR)); // allocate that memory


    } else {
        xdridptr[MAXID] = NULL; // Set the pointermask to NULL (no need to free memory)
	xdridptr[xdrid] = xdrs; // Set the pointer location to the already allocated memory
    }

    /* Initialize the XDR stream on the opened file */
    xdrstdio_create(xdridptr[xdrid], xdrfiles[xdrid], lmode);	
    return xdrid;
}

/*_________________________________________________________________________
 |
 | xdrclose - close a xdr file
 |
 | This will flush the xdr buffers, and destroy the xdr stream.
 | It also closes the associated file descriptor (this is *not*
 | done by xdr_destroy).
 |
*/
 
int xdrclose(XDR *xdrs) {
    int xdrid;
    
    if (xdrs == NULL) {
	fprintf(stderr, "xdrclose: passed a NULL pointer\n");
	exit(1);
    }
    for (xdrid = 1; xdrid < MAXID; xdrid++) {
	if (xdridptr[xdrid] == xdrs) {
	    
	    xdr_destroy(xdrs);      /* Flush and destroy the XDR stream */
	    fclose(xdrfiles[xdrid]); /* Close the underlying file */

            /* If xdridptr[MAXID] is non-NULL, this XDR struct was
             * allocated by the library (Fortran path) and must be freed.
             */
	    if (xdridptr[MAXID] != NULL) {	        
	       	free(xdridptr[xdrid]);
            }	
            xdridptr[xdrid] = NULL;
            return 1;          
	}
    } 
    fprintf(stderr, "xdrclose: no such open xdr file\n");
    exit(1);
    
}

/*____________________________________________________________________________
 |
 | sendbits - encode num into buf using the specified number of bits
 |
 | This routines appends the value of num to the bits already present in
 | the array buf. You need to give it the number of bits to use and you
 | better make sure that this number of bits is enough to hold the value
 | Also num must be positive.
 |
*/

static void sendbits(int buf[], int num_of_bits, int num) {
    
    unsigned int cnt, lastbyte;
    int lastbits;
    unsigned char * cbuf;
    
    cbuf = ((unsigned char *)buf) + 3 * sizeof(*buf);
    cnt = (unsigned int) buf[0];
    lastbits = buf[1];
    lastbyte =(unsigned int) buf[2];
    while (num_of_bits >= 8) {
	lastbyte = (lastbyte << 8) | ((num >> (num_of_bits -8)) /* & 0xff*/);
	cbuf[cnt++] = lastbyte >> lastbits;
	num_of_bits -= 8;
    }
    if (num_of_bits > 0) {
	lastbyte = (lastbyte << num_of_bits) | num;
	lastbits += num_of_bits;
	if (lastbits >= 8) {
	    lastbits -= 8;
	    cbuf[cnt++] = lastbyte >> lastbits;
	}
    }
    buf[0] = cnt;
    buf[1] = lastbits;
    buf[2] = lastbyte;
    if (lastbits>0) {
	cbuf[cnt] = lastbyte << (8 - lastbits);
    }
}

/*_________________________________________________________________________
 |
 | sizeofint - calculate bitsize of an integer
 |
 | return the number of bits needed to store an integer with given max size
 |
*/

static int sizeofint(const int size) {
    unsigned int num = 1;
    int num_of_bits = 0;
    
    while (size >= num && num_of_bits < 32) {
	num_of_bits++;
	num <<= 1;
    }
    return num_of_bits;
}

/*___________________________________________________________________________
 |
 | sizeofints - calculate 'bitsize' of compressed ints
 |
 | given the number of small unsigned integers and the maximum value
 | return the number of bits needed to read or write them with the
 | routines receiveints and sendints. You need this parameter when
 | calling these routines. Note that for many calls I can use
 | the variable 'smallidx' which is exactly the number of bits, and
 | So I don't need to call 'sizeofints for those calls.
*/

static int sizeofints( const int num_of_ints, unsigned int sizes[]) {
    int i, num;
    unsigned int num_of_bytes, num_of_bits, bytes[32], bytecnt, tmp;
    num_of_bytes = 1;
    bytes[0] = 1;
    num_of_bits = 0;
    for (i=0; i < num_of_ints; i++) {	
	tmp = 0;
	for (bytecnt = 0; bytecnt < num_of_bytes; bytecnt++) {
	    tmp = bytes[bytecnt] * sizes[i] + tmp;
	    bytes[bytecnt] = tmp & 0xff;
	    tmp >>= 8;
	}
	while (tmp != 0) {
	    bytes[bytecnt++] = tmp & 0xff;
	    tmp >>= 8;
	}
	num_of_bytes = bytecnt;
    }
    num = 1;
    num_of_bytes--;
    while (bytes[num_of_bytes] >= num) {
	num_of_bits++;
	num *= 2;
    }
    return num_of_bits + num_of_bytes * 8;

}
    
/*____________________________________________________________________________
 |
 | sendints - send a small set of small integers in compressed format
 |
 | this routine is used internally by xdr3dfcoord, to send a set of
 | small integers to the buffer. 
 | Multiplication with fixed (specified maximum ) sizes is used to get
 | to one big, multibyte integer. Allthough the routine could be
 | modified to handle sizes bigger than 16777216, or more than just
 | a few integers, this is not done, because the gain in compression
 | isn't worth the effort. Note that overflowing the multiplication
 | or the byte buffer (32 bytes) is unchecked and causes bad results.
 |
 */
 
static void sendints(int buf[], const int num_of_ints, const int num_of_bits,
	unsigned int sizes[], unsigned int nums[]) {

    int i;
    unsigned int bytes[32], num_of_bytes, bytecnt, tmp;

    tmp = nums[0];
    num_of_bytes = 0;
    do {
	bytes[num_of_bytes++] = tmp & 0xff;
	tmp >>= 8;
    } while (tmp != 0);

    for (i = 1; i < num_of_ints; i++) {
	if (nums[i] >= sizes[i]) {
	    fprintf(stderr,"major breakdown in sendints num %d doesn't "
		    "match size %d\n", nums[i], sizes[i]);
	    exit(1);
	}
	/* use one step multiply */    
	tmp = nums[i];
	for (bytecnt = 0; bytecnt < num_of_bytes; bytecnt++) {
	    tmp = bytes[bytecnt] * sizes[i] + tmp;
	    bytes[bytecnt] = tmp & 0xff;
	    tmp >>= 8;
	}
	while (tmp != 0) {
	    bytes[bytecnt++] = tmp & 0xff;
	    tmp >>= 8;
	}
	num_of_bytes = bytecnt;
    }
    if (num_of_bits >= num_of_bytes * 8) {
	for (i = 0; i < num_of_bytes; i++) {
	    sendbits(buf, 8, bytes[i]);
	}
	sendbits(buf, num_of_bits - num_of_bytes * 8, 0);
    } else {
	for (i = 0; i < num_of_bytes-1; i++) {
	    sendbits(buf, 8, bytes[i]);
	}
	sendbits(buf, num_of_bits- (num_of_bytes -1) * 8, bytes[i]);
    }
}


/*___________________________________________________________________________
 |
 | receivebits - decode number from buf using specified number of bits
 | 
 | extract the number of bits from the array buf and construct an integer
 | from it. Return that value.
 |
*/

static int receivebits(int buf[], int num_of_bits) {

    int cnt, num; 
    unsigned int lastbits, lastbyte;
    unsigned char * cbuf;
    int mask = (1 << num_of_bits) -1;

    cbuf = ((unsigned char *)buf) + 3 * sizeof(*buf);
    cnt = buf[0];
    lastbits = (unsigned int) buf[1];
    lastbyte = (unsigned int) buf[2];
    
    num = 0;
    while (num_of_bits >= 8) {
	lastbyte = ( lastbyte << 8 ) | cbuf[cnt++];
	num |=  (lastbyte >> lastbits) << (num_of_bits - 8);
	num_of_bits -=8;
    }
    if (num_of_bits > 0) {
	if (lastbits < num_of_bits) {
	    lastbits += 8;
	    lastbyte = (lastbyte << 8) | cbuf[cnt++];
	}
	lastbits -= num_of_bits;
	num |= (lastbyte >> lastbits) & ((1 << num_of_bits) -1);
    }
    num &= mask;
    buf[0] = cnt;
    buf[1] = lastbits;
    buf[2] = lastbyte;
    return num; 
}

/*____________________________________________________________________________
 |
 | receiveints - decode 'small' integers from the buf array
 |
 | this routine is the inverse from sendints() and decodes the small integers
 | written to buf by calculating the remainder and doing divisions with
 | the given sizes[]. You need to specify the total number of bits to be
 | used from buf in num_of_bits.
 |
*/

static void receiveints(int buf[], const int num_of_ints, int num_of_bits,
	unsigned int sizes[], int nums[]) {
    int bytes[32];
    int i, j, num_of_bytes, p, num;
    
    bytes[1] = bytes[2] = bytes[3] = 0;
    num_of_bytes = 0;
    while (num_of_bits > 8) {
	bytes[num_of_bytes++] = receivebits(buf, 8);
	num_of_bits -= 8;
    }
    if (num_of_bits > 0) {
	bytes[num_of_bytes++] = receivebits(buf, num_of_bits);
    }
    for (i = num_of_ints-1; i > 0; i--) {
	num = 0;
	for (j = num_of_bytes-1; j >=0; j--) {
	    num = (num << 8) | bytes[j];
	    p = num / sizes[i];
	    bytes[j] = p;
	    num = num - p * sizes[i];
	}
	nums[i] = num;
    }
    nums[0] = bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24);
}
    
/*____________________________________________________________________________
 |
 | xdr3dfcoord - read or write compressed 3d coordinates to xdr file.
 |
 | this routine reads or writes (depending on how you opened the file with
 | xdropen() ) a large number of 3d coordinates (stored in *fp).
 | The number of coordinates triplets to write is given by *size. On
 | read this number may be zero, in which case it reads as many as were written
 | or it may specify the number if triplets to read (which should match the
 | number written).
 | Compression is achieved by first converting all floating numbers to integer
 | using multiplication by *precision and rounding to the nearest integer.
 | Then the minimum and maximum value are calculated to determine the range.
 | The limited range of integers so found, is used to compress the coordinates.
 | In addition the differences between succesive coordinates is calculated.
 | If the difference happens to be 'small' then only the difference is saved,
 | compressing the data even more. The notion of 'small' is changed dynamically
 | and is enlarged or reduced whenever needed or possible.
 | Extra compression is achieved in the case of GROMOS and coordinates of
 | water molecules. GROMOS first writes out the Oxygen position, followed by
 | the two hydrogens. In order to make the differences smaller (and thereby
 | compression the data better) the order is changed into first one hydrogen
 | then the oxygen, followed by the other hydrogen. This is rather special, but
 | it shouldn't harm in the general case.
 |
 */
 
int xdr3dfcoord(XDR *xdrs, float *fp, int *size, float *precision) {
    
    /* Static buffers for integer coordinates and the bit-packing buffer.
     * These persist across calls to avoid repeated allocation for
     * frames of the same size.
     */
    static int *ip = NULL;
    static int oldsize;
    static int *buf;

    /* minint/maxint: per-axis min/max of integer coordinates (for range encoding).
     * mindiff: minimum Manhattan distance between successive coordinate triples
     *          (used to choose initial compression level).
     */
    int minint[3], maxint[3], mindiff, *lip, diff;
    int lint1, lint2, lint3, oldlint1, oldlint2, oldlint3, smallidx;
    int minidx, maxidx;
    /* sizeint: per-axis range (max - min + 1) for absolute coordinate encoding.
     * sizesmall: per-axis range for difference encoding (set to magicints[smallidx]).
     * bitsizeint: per-axis bit widths when ranges are too large for combined encoding.
     */
    unsigned sizeint[3], sizesmall[3], bitsizeint[3], size3, *luip;
    int flag, k;
    /* small/smaller/larger: half the magicints values at the current, previous,
     * and next compression levels. Used to test whether coordinate differences
     * fit within the current encoding window.
     */
    int small, smaller, larger, i, is_small, is_smaller, run, prevrun;
    float *lfp, lf;
    int tmp, *thiscoord,  prevcoord[3];
    unsigned int tmpcoord[30];  /* Temp buffer for up to 10 coordinate triples in a run */

    int bufsize, xdrid, lsize;
    unsigned int bitsize;
    float inv_precision;
    int errval = 1;

    /* find out if xdrs is opened for reading or for writing */
    xdrid = 0;
    while (xdridptr[xdrid] != xdrs) {
	xdrid++;
	if (xdrid >= MAXID) {
	    fprintf(stderr, "xdr error. no open xdr stream\n");
	    exit (1);
	}
    }
    if ((xdrmodes[xdrid] == 'w') || (xdrmodes[xdrid] == 'a')) {

	/* xdrs is open for writing */

	if (xdr_int(xdrs, size) == 0)
	    return 0;
	size3 = *size * 3;
	/* when the number of coordinates is small, don't try to compress; just
	 * write them as floats using xdr_vector
	 */
	if (*size <= 9 ) {
	    return (xdr_vector(xdrs, (char *) fp, size3, sizeof(*fp),
		(xdrproc_t)xdr_float));
	}
	
	xdr_float(xdrs, precision);
	if (ip == NULL) {
	    ip = (int *)malloc(size3 * sizeof(*ip));
	    if (ip == NULL) {
		fprintf(stderr,"malloc failed\n");
		exit(1);
	    }
	    bufsize = size3 * 1.2;
	    buf = (int *)malloc(bufsize * sizeof(*buf));
	    if (buf == NULL) {
		fprintf(stderr,"malloc failed\n");
		exit(1);
	    }
	    oldsize = *size;
	} else if (*size > oldsize) {
	    ip = (int *)realloc(ip, size3 * sizeof(*ip));
	    if (ip == NULL) {
		fprintf(stderr,"malloc failed\n");
		exit(1);
	    }
	    bufsize = size3 * 1.2;
	    buf = (int *)realloc(buf, bufsize * sizeof(*buf));
	    if (buf == NULL) {
		fprintf(stderr,"malloc failed\n");
		exit(1);
	    }
	    oldsize = *size;
	}
	/* buf[0-2] are reserved metadata for the bit-packing routines:
	 *   buf[0] = byte count written so far
	 *   buf[1] = number of leftover bits in the current byte
	 *   buf[2] = value of the current partial byte
	 * Actual compressed data starts at buf[3].
	 */
	buf[0] = buf[1] = buf[2] = 0;
	minint[0] = minint[1] = minint[2] = INT_MAX;
	maxint[0] = maxint[1] = maxint[2] = INT_MIN;
	prevrun = -1;
	lfp = fp;
	lip = ip;
	mindiff = INT_MAX;
	oldlint1 = oldlint2 = oldlint3 = 0;
	/* PASS 1: Convert all float coordinates to fixed-point integers
	 * by multiplying by precision and rounding. Track per-axis
	 * min/max and the minimum inter-coordinate Manhattan distance.
	 */
	while(lfp < fp + size3 ) {
	    /* find nearest integer */
	    if (*lfp >= 0.0)
		lf = *lfp * *precision + 0.5;
	    else
		lf = *lfp * *precision - 0.5;
	    if (fabs(lf) > MAXABS) {
		/* scaling would cause overflow */
		errval = 0;
	    }
	    lint1 = lf;
	    if (lint1 < minint[0]) minint[0] = lint1;
	    if (lint1 > maxint[0]) maxint[0] = lint1;
	    *lip++ = lint1;
	    lfp++;
	    if (*lfp >= 0.0)
		lf = *lfp * *precision + 0.5;
	    else
		lf = *lfp * *precision - 0.5;
	    if (fabs(lf) > MAXABS) {
		/* scaling would cause overflow */
		errval = 0;
	    }
	    lint2 = lf;
	    if (lint2 < minint[1]) minint[1] = lint2;
	    if (lint2 > maxint[1]) maxint[1] = lint2;
	    *lip++ = lint2;
	    lfp++;
	    if (*lfp >= 0.0)
		lf = *lfp * *precision + 0.5;
	    else
		lf = *lfp * *precision - 0.5;
	    if (fabs(lf) > MAXABS) {
		/* scaling would cause overflow */
		errval = 0;
	    }
	    lint3 = lf;
	    if (lint3 < minint[2]) minint[2] = lint3;
	    if (lint3 > maxint[2]) maxint[2] = lint3;
	    *lip++ = lint3;
	    lfp++;
	    diff = abs(oldlint1-lint1)+abs(oldlint2-lint2)+abs(oldlint3-lint3);
	    if (diff < mindiff && lfp > fp + 3)
		mindiff = diff;
	    oldlint1 = lint1;
	    oldlint2 = lint2;
	    oldlint3 = lint3;
	}
	/* Write the per-axis min and max values to the stream.
	 * These are needed by the reader to reconstruct absolute
	 * coordinates from the compressed (offset-from-minimum) form.
	 */
	xdr_int(xdrs, &(minint[0]));
	xdr_int(xdrs, &(minint[1]));
	xdr_int(xdrs, &(minint[2]));
	
	xdr_int(xdrs, &(maxint[0]));
	xdr_int(xdrs, &(maxint[1]));
	xdr_int(xdrs, &(maxint[2]));
	
	if ((float)maxint[0] - (float)minint[0] >= MAXABS ||
		(float)maxint[1] - (float)minint[1] >= MAXABS ||
		(float)maxint[2] - (float)minint[2] >= MAXABS) {
	    /* turning value in unsigned by subtracting minint
	     * would cause overflow
	     */
	    errval = 0;
	}
	sizeint[0] = maxint[0] - minint[0]+1;
	sizeint[1] = maxint[1] - minint[1]+1;
	sizeint[2] = maxint[2] - minint[2]+1;
	
	/* check if one of the sizes is to big to be multiplied */
	if ((sizeint[0] | sizeint[1] | sizeint[2] ) > 0xffffff) {
	    bitsizeint[0] = sizeofint(sizeint[0]);
	    bitsizeint[1] = sizeofint(sizeint[1]);
	    bitsizeint[2] = sizeofint(sizeint[2]);
	    bitsize = 0; /* flag the use of large sizes */
	} else {
	    bitsize = sizeofints(3, sizeint);
	}
	lip = ip;
	luip = (unsigned int *) ip;
	/* Choose the initial compression level (smallidx) based on the
	 * minimum difference between successive coordinates. This picks
	 * the smallest magicints entry that can represent the typical
	 * inter-coordinate difference, giving the best initial compression.
	 */
	smallidx = FIRSTIDX;
	while (smallidx < LASTIDX && magicints[smallidx] < mindiff) {
	    smallidx++;
	}
	xdr_int(xdrs, &smallidx);
	maxidx = MIN(LASTIDX, smallidx + 8) ;
	minidx = maxidx - 8; /* often this equal smallidx */
	smaller = magicints[MAX(FIRSTIDX, smallidx-1)] / 2;
	small = magicints[smallidx] / 2;
	sizesmall[0] = sizesmall[1] = sizesmall[2] = magicints[smallidx];
	larger = magicints[maxidx] / 2;
	/* PASS 2: Encode integer coordinates into the bit-packing buffer.
	 * For each coordinate triple:
	 *   1. Write the absolute position (offset from minint) using
	 *      either per-axis bits or combined multi-int encoding.
	 *   2. Check if the next coordinate is "small" (close to this one).
	 *      If so, start a run of difference-encoded coordinates.
	 *   3. For water molecules, swap atom order (H-O-H instead of
	 *      O-H-H) to reduce differences and improve compression.
	 *   4. Adaptively adjust the compression level (smallidx) up
	 *      or down based on whether differences fit the current window.
	 */
	i = 0;
	while (i < *size) {
	    is_small = 0;
	    thiscoord = (int *)(luip) + i * 3;
	    if (smallidx < maxidx && i >= 1 &&
		    abs(thiscoord[0] - prevcoord[0]) < larger &&
		    abs(thiscoord[1] - prevcoord[1]) < larger &&
		    abs(thiscoord[2] - prevcoord[2]) < larger) {
		is_smaller = 1;
	    } else if (smallidx > minidx) {
		is_smaller = -1;
	    } else {
		is_smaller = 0;
	    }
	    if (i + 1 < *size) {
		if (abs(thiscoord[0] - thiscoord[3]) < small &&
			abs(thiscoord[1] - thiscoord[4]) < small &&
			abs(thiscoord[2] - thiscoord[5]) < small) {
		    /* interchange first with second atom for better
		     * compression of water molecules
		     */
		    tmp = thiscoord[0]; thiscoord[0] = thiscoord[3];
			thiscoord[3] = tmp;
		    tmp = thiscoord[1]; thiscoord[1] = thiscoord[4];
			thiscoord[4] = tmp;
		    tmp = thiscoord[2]; thiscoord[2] = thiscoord[5];
			thiscoord[5] = tmp;
		    is_small = 1;
		}
    
	    }
	    tmpcoord[0] = thiscoord[0] - minint[0];
	    tmpcoord[1] = thiscoord[1] - minint[1];
	    tmpcoord[2] = thiscoord[2] - minint[2];
	    if (bitsize == 0) {
		sendbits(buf, bitsizeint[0], tmpcoord[0]);
		sendbits(buf, bitsizeint[1], tmpcoord[1]);
		sendbits(buf, bitsizeint[2], tmpcoord[2]);
	    } else {
		sendints(buf, 3, bitsize, sizeint, tmpcoord);
	    }
	    prevcoord[0] = thiscoord[0];
	    prevcoord[1] = thiscoord[1];
	    prevcoord[2] = thiscoord[2];
	    thiscoord = thiscoord + 3;
	    i++;
	    
	    run = 0;
	    if (is_small == 0 && is_smaller == -1)
		is_smaller = 0;
	    while (is_small && run < 8*3) {
		if (is_smaller == -1 && (
			SQR(thiscoord[0] - prevcoord[0]) +
			SQR(thiscoord[1] - prevcoord[1]) +
			SQR(thiscoord[2] - prevcoord[2]) >= smaller * smaller)) {
		    is_smaller = 0;
		}

		tmpcoord[run++] = thiscoord[0] - prevcoord[0] + small;
		tmpcoord[run++] = thiscoord[1] - prevcoord[1] + small;
		tmpcoord[run++] = thiscoord[2] - prevcoord[2] + small;
		
		prevcoord[0] = thiscoord[0];
		prevcoord[1] = thiscoord[1];
		prevcoord[2] = thiscoord[2];

		i++;
		thiscoord = thiscoord + 3;
		is_small = 0;
		if (i < *size &&
			abs(thiscoord[0] - prevcoord[0]) < small &&
			abs(thiscoord[1] - prevcoord[1]) < small &&
			abs(thiscoord[2] - prevcoord[2]) < small) {
		    is_small = 1;
		}
	    }
	    if (run != prevrun || is_smaller != 0) {
		prevrun = run;
		sendbits(buf, 1, 1); /* flag the change in run-length */
		sendbits(buf, 5, run+is_smaller+1);
	    } else {
		sendbits(buf, 1, 0); /* flag the fact that runlength did not change */
	    }
	    for (k=0; k < run; k+=3) {
		sendints(buf, 3, smallidx, sizesmall, &tmpcoord[k]);	
	    }
	    if (is_smaller != 0) {
		smallidx += is_smaller;
		if (is_smaller < 0) {
		    small = smaller;
		    smaller = magicints[smallidx-1] / 2;
		} else {
		    smaller = small;
		    small = magicints[smallidx] / 2;
		}
		sizesmall[0] = sizesmall[1] = sizesmall[2] = magicints[smallidx];
	    }
	}
	if (buf[1] != 0) buf[0]++;; /* Account for any remaining partial byte */
	/* Write the compressed data: first the byte count, then the raw bytes */
	xdr_int(xdrs, &(buf[0])); /* buf[0] holds the length in bytes */
	return errval * (xdr_opaque(xdrs, (caddr_t)&(buf[3]), (u_int)buf[0]));
    } else {
	
	/* ================================================================
	 * READ PATH: Decode compressed coordinates back to floats.
	 * The format mirrors the write path:
	 *   1. Read size and precision.
	 *   2. Read min/max ranges and reconstruct encoding parameters.
	 *   3. Read the compressed bit buffer.
	 *   4. Decode each coordinate triple, handling both absolute
	 *      and difference-encoded (run) coordinates.
	 *   5. Convert fixed-point integers back to floats.
	 * ================================================================
	 */
	
	if (xdr_int(xdrs, &lsize) == 0) 
	    return 0;
	if (*size != 0 && lsize != *size) {
	    fprintf(stderr, "wrong number of coordinates in xdr3dfcoor; "
		    "%d arg vs %d in file", *size, lsize);
	}
	*size = lsize;
	size3 = *size * 3;
	if (*size <= 9) {
	    return (xdr_vector(xdrs, (char *) fp, size3, sizeof(*fp),
		(xdrproc_t)xdr_float));
	}
	xdr_float(xdrs, precision);
	if (ip == NULL) {
	    ip = (int *)malloc(size3 * sizeof(*ip));
	    if (ip == NULL) {
		fprintf(stderr,"malloc failed\n");
		exit(1);
	    }
	    bufsize = size3 * 1.2;
	    buf = (int *)malloc(bufsize * sizeof(*buf));
	    if (buf == NULL) {
		fprintf(stderr,"malloc failed\n");
		exit(1);
	    }
	    oldsize = *size;
	} else if (*size > oldsize) {
	    ip = (int *)realloc(ip, size3 * sizeof(*ip));
	    if (ip == NULL) {
		fprintf(stderr,"malloc failed\n");
		exit(1);
	    }
	    bufsize = size3 * 1.2;
	    buf = (int *)realloc(buf, bufsize * sizeof(*buf));
	    if (buf == NULL) {
		fprintf(stderr,"malloc failed\n");
		exit(1);
	    }
	    oldsize = *size;
	}
	buf[0] = buf[1] = buf[2] = 0;
	
	/* Read per-axis min/max used to reconstruct absolute coordinates */
	xdr_int(xdrs, &(minint[0]));
	xdr_int(xdrs, &(minint[1]));
	xdr_int(xdrs, &(minint[2]));

	xdr_int(xdrs, &(maxint[0]));
	xdr_int(xdrs, &(maxint[1]));
	xdr_int(xdrs, &(maxint[2]));
		
	sizeint[0] = maxint[0] - minint[0]+1;
	sizeint[1] = maxint[1] - minint[1]+1;
	sizeint[2] = maxint[2] - minint[2]+1;
	
	/* check if one of the sizes is to big to be multiplied */
	if ((sizeint[0] | sizeint[1] | sizeint[2] ) > 0xffffff) {
	    bitsizeint[0] = sizeofint(sizeint[0]);
	    bitsizeint[1] = sizeofint(sizeint[1]);
	    bitsizeint[2] = sizeofint(sizeint[2]);
	    bitsize = 0; /* flag the use of large sizes */
	} else {
	    bitsize = sizeofints(3, sizeint);
	}
	
	xdr_int(xdrs, &smallidx);
	maxidx = MIN(LASTIDX, smallidx + 8) ;
	minidx = maxidx - 8; /* often this equal smallidx */
	smaller = magicints[MAX(FIRSTIDX, smallidx-1)] / 2;
	small = magicints[smallidx] / 2;
	sizesmall[0] = sizesmall[1] = sizesmall[2] = magicints[smallidx] ;
	larger = magicints[maxidx];

    	/* Read the compressed bit buffer from the stream */

	if (xdr_int(xdrs, &(buf[0])) == 0)  /* buf[0] = byte count */
	    return 0;
	if (xdr_opaque(xdrs, (caddr_t)&(buf[3]), (u_int)buf[0]) == 0)
	    return 0;
	buf[0] = buf[1] = buf[2] = 0;  /* Reset bit-unpacking state */
	
	lfp = fp;
	inv_precision = 1.0 / * precision;  /* Precompute for int-to-float conversion */
	run = 0;
	i = 0;
	lip = ip;
	/* Decode each coordinate triple from the bit buffer */
	while ( i < lsize ) {
	    thiscoord = (int *)(lip) + i * 3;

	    if (bitsize == 0) {
		thiscoord[0] = receivebits(buf, bitsizeint[0]);
		thiscoord[1] = receivebits(buf, bitsizeint[1]);
		thiscoord[2] = receivebits(buf, bitsizeint[2]);
	    } else {
		receiveints(buf, 3, bitsize, sizeint, thiscoord);
	    }
	    
	    i++;
	    thiscoord[0] += minint[0];
	    thiscoord[1] += minint[1];
	    thiscoord[2] += minint[2];
	    
	    prevcoord[0] = thiscoord[0];
	    prevcoord[1] = thiscoord[1];
	    prevcoord[2] = thiscoord[2];
	    
	   
	    flag = receivebits(buf, 1);
	    is_smaller = 0;
	    if (flag == 1) {
		run = receivebits(buf, 5);
		is_smaller = run % 3;
		run -= is_smaller;
		is_smaller--;
	    }
	    if (run > 0) {
		thiscoord += 3;
		for (k = 0; k < run; k+=3) {
		    receiveints(buf, 3, smallidx, sizesmall, thiscoord);
		    i++;
		    thiscoord[0] += prevcoord[0] - small;
		    thiscoord[1] += prevcoord[1] - small;
		    thiscoord[2] += prevcoord[2] - small;
		    if (k == 0) {
			/* interchange first with second atom for better
			 * compression of water molecules
			 */
			tmp = thiscoord[0]; thiscoord[0] = prevcoord[0];
				prevcoord[0] = tmp;
			tmp = thiscoord[1]; thiscoord[1] = prevcoord[1];
				prevcoord[1] = tmp;
			tmp = thiscoord[2]; thiscoord[2] = prevcoord[2];
				prevcoord[2] = tmp;
			*lfp++ = prevcoord[0] * inv_precision;
			*lfp++ = prevcoord[1] * inv_precision;
			*lfp++ = prevcoord[2] * inv_precision;
		    } else {
			prevcoord[0] = thiscoord[0];
			prevcoord[1] = thiscoord[1];
			prevcoord[2] = thiscoord[2];
		    }
		    *lfp++ = thiscoord[0] * inv_precision;
		    *lfp++ = thiscoord[1] * inv_precision;
		    *lfp++ = thiscoord[2] * inv_precision;
		}
	    } else {
		*lfp++ = thiscoord[0] * inv_precision;
		*lfp++ = thiscoord[1] * inv_precision;
		*lfp++ = thiscoord[2] * inv_precision;		
	    }
	    smallidx += is_smaller;
	    if (is_smaller < 0) {
		small = smaller;
		if (smallidx > FIRSTIDX) {
		    smaller = magicints[smallidx - 1] /2;
		} else {
		    smaller = 0;
		}
	    } else if (is_smaller > 0) {
		smaller = small;
		small = magicints[smallidx] / 2;
	    }
	    sizesmall[0] = sizesmall[1] = sizesmall[2] = magicints[smallidx] ;
	}
    }
    return 1;
}


   
