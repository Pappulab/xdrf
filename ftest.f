c     ================================================================
c     ftest.f - Test program for the xdrf library (Fortran interface)
c
c     This program validates the xdrf Fortran wrappers by performing
c     the same compress/decompress round-trip as ctest.c:
c       1. Reads ASCII coordinate data from test.gmx
c       2. Writes compressed coordinates to test.xdr via xdrf3dfcoord
c       3. Reads the compressed data back from test.xdr
c       4. Writes decompressed coordinates to test.out as ASCII
c
c     If the round-trip succeeds, no output is printed (silent success).
c     ================================================================

	program xdr_test
	
	implicit none
	integer xd, xd2         ! XDR file handles for write and read

	integer i, j, num_of_coord, ret, framecnt
	real coord(10000)        ! Original coordinate buffer
	real coord2(10000)       ! Decompressed coordinate buffer
	real d0, d1, d2, d3      ! Header values from first line
	real prec                ! Precision for compression
	
	prec = 1000.0

c     --- STEP 1: Read ASCII data and write compressed ---
c     Open the ASCII coordinate file
	open(7, file="test.gmx", status="old")

c     Read header line: number of coordinates and 4 float values
	read(7, *) num_of_coord, d0, d1, d2, d3

c     Open XDR file for writing (append mode)
	call xdrfopen(xd, "test.xdr", "a", ret)
	
c     Write header values using standard XDR routines
	call xdrfint(xd, num_of_coord, ret)
	call xdrffloat(xd, d0, ret)
	call xdrffloat(xd, d1, ret)
	call xdrffloat(xd, d2, ret)
	call xdrffloat(xd, d3, ret)

c     Read and compress each frame of coordinates
	framecnt = 0
 10	continue
	call read_frame(7, num_of_coord, coord, ret)
	if (ret .eq. 0) goto 20
	call xdrf3dfcoord(xd, coord, num_of_coord, prec, ret)
	framecnt = framecnt + 1
	goto 10
 20	continue

	call xdrfclose(xd, ret)
	close(7)

c     --- STEP 2: Read compressed data and write decompressed ASCII ---

c     Open output file for decompressed coordinates
	open(8, file="test.out", status="unknown")

c     Open XDR file for reading
	call xdrfopen(xd2, "test.xdr", "r", ret)

c     Read back header values
	call xdrfint(xd2, num_of_coord, ret)
	call xdrffloat(xd2, d0, ret)
	call xdrffloat(xd2, d1, ret)
	call xdrffloat(xd2, d2, ret)
	call xdrffloat(xd2, d3, ret)
	write(8,'(i5,f8.3,f8.3,f8.3,f8.3)') num_of_coord, d0, d1,
     &		d2, d3

c     Decompress and write each frame
	do 30, i=1,framecnt
	    call xdrf3dfcoord(xd2, coord2, num_of_coord, prec, ret)
	    do 40, j=1, num_of_coord * 3, 3
	    	write (8,'(f8.3,f9.3,f9.3)') coord2(j), coord2(j+1),
     &			coord2(j+2)
 40	    continue
 30	continue
	call xdrfclose(xd2, ret)
	close(8)
	end

	
c     ================================================================
c     read_frame - Read one frame of ASCII coordinates from a file.
c
c     Reads num_of_coord lines, each with 3 floating-point values.
c     Sets ret=1 on success, ret=0 on EOF or read error.
c     ================================================================
	subroutine read_frame(in, num_of_coord,  coord, ret)

	implicit none
	integer in
	integer i, num_of_coord
	real coord(*)
	integer ret

	do 100, i=1, num_of_coord*3, 3
	    read (in, *, err = 120, end=120) coord(i), coord(i+1),
     &		coord(i+2)
 100	continue
	ret = 1
	return
 120	ret = 0
	return
	end
