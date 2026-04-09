/*________________________________________________________________________
 |
 | cxxtest.cpp - Test program for the xdrf C++ interface.
 |
 | Equivalent to ctest.c but using the xdrf::XdrFile C++ API.
 | Reads test.gmx, compresses via xdr3dfcoord, decompresses,
 | and verifies the round-trip.
 |
 | Expected output: "maxdiff = 0.000000"
 |
*/

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include "xdrf.hpp"

static int read_frame(FILE *in, int num_of_coord, float coord[]) {
    double d[3];
    char line[1024];
    for (int i = 0; i < num_of_coord; i++) {
        if (fgets(line, sizeof(line) - 1, in) == nullptr) {
            if (i == 0) return -1;
            fprintf(stderr, "incomplete coordinate frame during read %d", i);
            exit(1);
        }
        sscanf(line, " %lf %lf %lf", &d[0], &d[1], &d[2]);
        *coord++ = static_cast<float>(d[0]);
        *coord++ = static_cast<float>(d[1]);
        *coord++ = static_cast<float>(d[2]);
    }
    return 0;
}

int main() {
    char line[1024];

    /* Read header from test.gmx */
    FILE *fgmx = fopen("test.gmx", "r");
    if (!fgmx) {
        perror("could not open gmx test data");
        return 1;
    }
    if (!fgets(line, sizeof(line) - 1, fgmx)) {
        perror("cannot read gmx test data");
        return 1;
    }

    int num_of_coord;
    float d0, d1, d2, d3;
    sscanf(line, " %d %f %f %f %f", &num_of_coord, &d0, &d1, &d2, &d3);

    std::vector<float> coord(num_of_coord * 3);
    std::vector<float> coord2(num_of_coord * 3);

    /* ---- STEP 1: Compress ---- */
    int framecnt = 0;
    {
        xdrf::XdrFile out("test_cpp.xdr", "a");
        out.write_int(num_of_coord);
        out.write_float(d0);
        out.write_float(d1);
        out.write_float(d2);
        out.write_float(d3);

        while (read_frame(fgmx, num_of_coord, coord.data()) == 0) {
            framecnt++;
            out.write_3dfcoord(coord, 1000.0f);
        }
    } /* XdrFile closes automatically here */
    fclose(fgmx);

    /* ---- STEP 2: Decompress ---- */
    FILE *fout = fopen("test_cpp.out", "w+");
    if (!fout) {
        perror("could not open test_cpp.out");
        return 1;
    }
    {
        xdrf::XdrFile in("test_cpp.xdr", "r");
        int n = in.read_int();
        float r0 = in.read_float();
        float r1 = in.read_float();
        float r2 = in.read_float();
        float r3 = in.read_float();
        fprintf(fout, "%5d%8.3f%8.3f%8.3f%8.3f\n", n, r0, r1, r2, r3);

        for (int i = 0; i < framecnt; i++) {
            float prec = 0;
            int natoms = num_of_coord;
            in.read_3dfcoord(coord2.data(), natoms, prec);
            for (int j = 0; j < natoms * 3; j += 3) {
                fprintf(fout, "%8.3f %8.3f %8.3f\n",
                        coord2[j], coord2[j + 1], coord2[j + 2]);
            }
        }
    }
    fclose(fout);

    /* ---- STEP 3: Verify round-trip ---- */
    fgmx = fopen("test.gmx", "r");
    fout = fopen("test_cpp.out", "r");
    double maxdiff = 0;
    fgets(line, sizeof(line) - 1, fgmx);
    fgets(line, sizeof(line) - 1, fout);
    for (int i = 0; i < framecnt; i++) {
        read_frame(fgmx, num_of_coord, coord.data());
        read_frame(fout, num_of_coord, coord2.data());
        for (int j = 0; j < num_of_coord * 3; j++) {
            double diff = std::fabs(coord[j] - coord2[j]);
            if (diff > maxdiff) maxdiff = diff;
        }
    }
    fprintf(stderr, "\nmaxdiff  = %f\n", maxdiff);
    fclose(fgmx);
    fclose(fout);

    return 0;
}
