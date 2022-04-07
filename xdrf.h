/*_________________________________________________________________
 |
 | xdrf.h - include file for C routines that want to use the 
 |	    functions below.
*/


// Update April 2022
// added this definition in to avoid a C99 requirement that no 
// implicit function names are included. The code here actually overwrits
// what xdrstdio_create does but we define it here to ensure the XDRF library
// here can be defined as a stand-alone codebase
int xdrstdio_create(XDR *xdrs, const char *filename, const char *type);


int xdropen(XDR *xdrs, const char *filename, const char *type);
int xdrclose(XDR *xdrs) ;
int xdr3dfcoord(XDR *xdrs, float *fp, int *size, float *precision) ;

