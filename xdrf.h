/*_________________________________________________________________
 |
 | xdrf.h - include file for C routines that want to use the 
 |	    functions below.
*/


// On Linux, xdrstdio_create is declared in <rpc/xdr.h> via libntirpc.
// On macOS, the declaration is missing from the system headers, so we
// provide it here. The function itself exists in the system RPC library.
#if defined(__APPLE__) || defined(darwin)
#include <stdio.h>
extern void xdrstdio_create(XDR *xdrs, FILE *file, enum xdr_op op);
#endif


int xdropen(XDR *xdrs, const char *filename, const char *type);
int xdrclose(XDR *xdrs) ;
int xdr3dfcoord(XDR *xdrs, float *fp, int *size, float *precision) ;

