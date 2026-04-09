/*________________________________________________________________________
 |
 | ctest.c - Test program for the xdrf library (C interface).
 |
 | This program demonstrates and validates the xdr3dfcoord compression
 | round-trip by:
 |   1. Reading ASCII coordinate data from test.gmx
 |   2. Writing compressed coordinates to test.xdr using xdr3dfcoord
 |   3. Reading the compressed data back from test.xdr
 |   4. Writing decompressed coordinates to test.out as ASCII
 |   5. Comparing the original and decompressed data to verify
 |      that the maximum difference is within precision bounds
 |
 | Expected output: "maxdiff = 0.000000"
 |
*/

#include <limits.h>
#include <math.h>
#include <rpc/rpc.h>
#include <rpc/xdr.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

#include "xdrf.h"

/*________________________________________________________________________
 |
 | read_frame - Read one frame of ASCII coordinate data.
 |
 | Reads num_of_coord lines from the input file, each containing
 | three floating-point numbers (x, y, z). The results are stored
 | sequentially in the coord array (x0, y0, z0, x1, y1, z1, ...).
 |
 | Parameters:
 |   in            - Open file handle positioned at the start of a frame.
 |   num_of_coord  - Number of coordinate triples to read.
 |   coord         - Pre-allocated float array of size 3 * num_of_coord.
 |
 | Returns: 0 on success, -1 on EOF (if no coordinates were read).
 |          Exits on error if a partial frame is encountered.
 |
*/

static int read_frame(FILE *in, int num_of_coord, float coord[]) {
    int i;
    double d[3];
    char line[1024];
    for (i = 0; i < num_of_coord; i++) {
	if (fgets(line, 1024 -1, in) == NULL) {
	    if (i == 0) return -1;
	    fprintf(stderr,"incomplete coordinate frame during read %d", i);
	    exit (1);
	}
	sscanf(line," %lf %lf %lf", &d[0], &d[1], &d[2]);
	*coord++ = (float) d[0];
	*coord++ = (float) d[1];
	*coord++ = (float) d[2];
    }
    return 0;
}


int main() {
    XDR xd, xd2;           /* XDR stream handles for write and read */
    int i, j;
    float prec = 1000.0;   /* Precision: 3 decimal places */
    float *coord, *coord2; /* Original and decompressed coordinate arrays */
    int num_of_coord;
    char *line;
    int framecnt = 0;
    double maxdiff = 0;
    float d0, d1, d2, d3;  /* Header values from the first line of test.gmx */
    FILE *fgmx, *fout;
    
    line = (char *) malloc(1024);

    /* open file containing 3d coordinates */
    fgmx = fopen("test.gmx","r");
    if (fgmx == NULL) {
	perror("could open gmx test data");
	exit(1);
    }
    if (fgets(line, 1024 -1, fgmx) == NULL) {
	perror("cannot read gmx test data");
	exit (1);
    }
    
    /* read first line which contains number of coordinates */
    sscanf(line," %d %f %f %f %f", &num_of_coord, &d0, &d1, &d2, &d3);
    
    /* Allocate coordinate buffers for the number of atoms */
    coord = (float *)malloc(num_of_coord * 3 * sizeof(float));
    coord2 = (float *)malloc(num_of_coord * 3 * sizeof(float));

    /* ---- STEP 1: Compress ---- */
    /* Open XDR file in append mode for writing compressed coordinates */
    if (xdropen(&xd, "test.xdr","a") == 0) {
	fprintf(stderr,"failed to open file\n");
    }
    
    /* just as test write the first line using normal xdr routine */
    xdr_int(&xd, &num_of_coord);
    xdr_float(&xd, &d0);
    xdr_float(&xd, &d1);
    xdr_float(&xd, &d2);
    xdr_float(&xd, &d3);
    /* Read all frames from ASCII input; compress and write each one */
    while ( read_frame(fgmx, num_of_coord, coord) == 0 ) {
	framecnt++;
	if (xdr3dfcoord(&xd, coord, &num_of_coord, &prec) == 0) {
	    fprintf(stderr,"error while writing coordinates\n");
	}
    }
    xdrclose(&xd);
    fclose(fgmx);
    
    
    
    /* ---- STEP 2: Decompress ---- */
    
    /* Open output file for decompressed ASCII coordinates */
    fout = fopen("test.out", "w+");
    if (fout == NULL) {
	perror("could not open test.out to write data\n");
	exit(1);
    }
    /* Open XDR file for reading */
    if (xdropen(&xd2, "test.xdr","r") == 0) {
	fprintf(stderr,"error while opening test.xdr for read\n");
	exit(1);
    }
    *line = '\0';
    /* Read back header values */
    xdr_int(&xd2, &num_of_coord);
    xdr_float(&xd2, &d0);
    xdr_float(&xd2, &d1);
    xdr_float(&xd2, &d2);
    xdr_float(&xd2, &d3);
    fprintf(fout, "%5d%8.3f%8.3f%8.3f%8.3f\n", num_of_coord, d0, d1, d2, d3);
    /* Decompress each frame and write as ASCII */
    for (i = 0; i < framecnt ; i++) {
	if (xdr3dfcoord(&xd2, (float *)coord2, &num_of_coord, &prec) == 0) {
	    fprintf(stderr, "error while reading coordinates\n");
	}
	for (j=0; j < num_of_coord * 3; j += 3) {
	    fprintf(fout, "%8.3f %8.3f %8.3f\n", coord2[j], 
		coord2[j+1] ,coord2[j+2]);
	}
    }
    xdrclose(&xd2);
    fclose(fout);
    
    
    
    /* ---- STEP 3: Verify round-trip ---- */
    /* Compare original ASCII data with decompressed output.
     * The maximum per-coordinate difference should be 0 (within
     * the precision used for compression).
     */
    
    fgmx = fopen("test.gmx", "r");
    fout = fopen("test.out", "r");
    maxdiff = 0;
    fgets(line, 1024 -1, fgmx);
    fgets(line, 1024 -1, fout);
    for (i = 0; i < framecnt ; i++) {
        read_frame(fgmx, num_of_coord, coord);
        read_frame(fout, num_of_coord, coord2);
	for (j=0; j < num_of_coord * 3; j++) {
	    if (fabs(coord[j] - coord2[j]) > maxdiff) 
	    maxdiff = fabs(coord[j] - coord2[j]) ;
	}
    }
    fprintf(stderr,"\nmaxdiff  = %f\n", maxdiff);    

    free(coord);
    free(coord2);
    free(line);
    return 0;
}
