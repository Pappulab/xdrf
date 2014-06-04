libxdrf
====

libxdrf version 1.12 MEMORY ERROR PATCHED

** *This version of libxdrf has a memory leak corrected as compared to previous versions. We strongly suggest using this version when compiling CAMPARI.* **

This directory contains source of the portable data compression library xdrf, which was developed for EUROPORT. It also contains two test programs (one in written in FORTRAN and one written in C) and a large file with test data.

### Building the libxdrf static library

To create the library, look into the subdirectory conf, and select the architecture (ARCH) that best matches your system. Edit the makefile and use your system definition. Note that you have to set ARCH, LIBS, HASRANLIB and perhaps some compiler specific settings.

The default makefile provided will work for Linux systems where the intel `ifort` compiler can be used. If you don't have `ifort` available please use `gfortran`.

To build the library type;

    make

This will create `libxdrf.a` and the two test programs (both programs
are linked with this new `libxdrf.a` library).


### Testing the compiled library
To test the program type;

    ctest #	(FORTRAN version), or 
    ftest # (C version)

You can compare the input file `test.gmx` with the output `test.out`, which should be the same (numerically at least).

You can also compare the filesize of `test.gmx` and the newly-created compressed data file `test.xdr`. 

Read the included manual page, or look into the source files. `Intro.txt` describes in general terms what xdrf is for.


### Credits
(c) 1995 Frans van Hoesel (original text and code)

Updated by Alex Holehouse, 2014

hoesel@chem.rug.nl

alex.holehouse@wustl.edu
