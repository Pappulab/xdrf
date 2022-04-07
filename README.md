# libxdrf: 


##### Last updated 2022-04-07

libxdrf version 1.12 MEMORY ERROR PATCHED

** *This version of libxdrf has a memory leak corrected as compared to previous versions. We strongly suggest using this version when compiling CAMPARI.* **

This directory contains source of the portable data compression library xdrf, which was developed for EUROPORT. It also contains two test programs (one in written in FORTRAN and one written in C) and a large file with test data.

## Building the libxdrf static library

Please download the `libxdrf_v1.3.tar.gz` file, unpack, and compile to generate the static libraries against which CAMPARI can be linked.

Once the tarball is unpacked, to create the library, look into the subdirectory conf, and select the architecture (ARCH) that best matches your system. Edit the makefile and use your system definition. Note that you have to set ARCH, LIBS, HASRANLIB and perhaps some compiler specific settings.

The default makefile provided will work for Linux of macOS systems using GNU compilers (`gcc` and `gfortran`). However, if the Intel compilers (`ifort`, and `icc`) are available we recommend these - this can be changed by editing the Makefile appropriately.

To build the library type;

    make

This will create `libxdrf.a` and the two test programs (both programs
are linked with this new `libxdrf.a` library).


### Testing the compiled library
To test the program type;

    ./ctest   #	(C version - if works will print out a difference of 0.00)
    ./ftest   #  (FORTRAN version - no output printed if things work!)

You can compare the input file `test.gmx` with the output `test.out`, which should be the same (numerically at least).

You can also compare the filesize of `test.gmx` and the newly-created compressed data file `test.xdr`. 

Read the included manual page, or look into the source files. `Intro.txt` describes in general terms what xdrf is for.

## Issues?
If you run into any problems please raise a GitHub issue, and if I (Alex) don't respond in ~1 week send me an email as I've probably not see it!

## Changelog

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
