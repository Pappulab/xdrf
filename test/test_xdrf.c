/*____________________________________________________________________________
 |
 | test_xdrf.c - Comprehensive test suite for the xdrf C API.
 |
 | Tests xdropen, xdrclose, xdr3dfcoord, and the standard XDR primitives
 | accessed through xdropen-managed streams. Each test writes data to a
 | temporary XDR file, reads it back, and verifies correctness.
 |
 | Exit code: 0 if all tests pass, 1 if any test fails.
 |
*/

#include <limits.h>
#include <math.h>
#include <rpc/rpc.h>
#include <rpc/xdr.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "../xdrf.h"

/* ---------- Minimal test framework ---------- */

static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

#define ASSERT_MSG(cond, msg) do { \
    tests_run++; \
    if (!(cond)) { \
        fprintf(stderr, "  FAIL: %s (line %d): %s\n", __func__, __LINE__, msg); \
        tests_failed++; \
        return 1; \
    } else { \
        tests_passed++; \
    } \
} while(0)

#define ASSERT_INT_EQ(a, b) do { \
    int _a = (a), _b = (b); \
    tests_run++; \
    if (_a != _b) { \
        fprintf(stderr, "  FAIL: %s (line %d): expected %d, got %d\n", \
                __func__, __LINE__, _b, _a); \
        tests_failed++; \
        return 1; \
    } else { tests_passed++; } \
} while(0)

#define ASSERT_FLOAT_EQ(a, b, eps) do { \
    float _a = (a), _b = (b); \
    tests_run++; \
    if (fabs(_a - _b) > eps) { \
        fprintf(stderr, "  FAIL: %s (line %d): expected %f, got %f (eps=%e)\n", \
                __func__, __LINE__, (double)_b, (double)_a, (double)eps); \
        tests_failed++; \
        return 1; \
    } else { tests_passed++; } \
} while(0)

#define ASSERT_DOUBLE_EQ(a, b, eps) do { \
    double _a = (a), _b = (b); \
    tests_run++; \
    if (fabs(_a - _b) > eps) { \
        fprintf(stderr, "  FAIL: %s (line %d): expected %f, got %f (eps=%e)\n", \
                __func__, __LINE__, _b, _a, eps); \
        tests_failed++; \
        return 1; \
    } else { tests_passed++; } \
} while(0)

#define RUN_TEST(fn) do { \
    int _r; \
    fprintf(stderr, "  %s ... ", #fn); \
    _r = fn(); \
    if (_r == 0) fprintf(stderr, "ok\n"); \
    else fprintf(stderr, "\n"); \
} while(0)


/* ---------- Helper: remove temp file ---------- */

static void cleanup(const char *path) {
    unlink(path);
}


/* ================================================================
 * TEST: xdropen / xdrclose basics
 * ================================================================ */

static int test_open_close_write(void) {
    XDR xd;
    int id;
    const char *f = "tmp_test_oc.xdr";

    id = xdropen(&xd, f, "w");
    ASSERT_MSG(id != 0, "xdropen(w) should return nonzero id");

    int val = 42;
    int ok = xdr_int(&xd, &val);
    ASSERT_MSG(ok != 0, "xdr_int write should succeed");

    int rc = xdrclose(&xd);
    ASSERT_INT_EQ(rc, 1);

    cleanup(f);
    return 0;
}

static int test_open_close_read(void) {
    XDR xd;
    int id;
    const char *f = "tmp_test_ocr.xdr";

    /* Write something first */
    id = xdropen(&xd, f, "w");
    ASSERT_MSG(id != 0, "xdropen(w) should succeed");
    int val = 99;
    xdr_int(&xd, &val);
    xdrclose(&xd);

    /* Read it back */
    id = xdropen(&xd, f, "r");
    ASSERT_MSG(id != 0, "xdropen(r) should succeed");
    int rval = 0;
    xdr_int(&xd, &rval);
    ASSERT_INT_EQ(rval, 99);
    xdrclose(&xd);

    cleanup(f);
    return 0;
}

static int test_open_nonexistent_read(void) {
    XDR xd;
    int id;

    id = xdropen(&xd, "nonexistent_file_xyzzy.xdr", "r");
    ASSERT_INT_EQ(id, 0);

    return 0;
}


/* ================================================================
 * TEST: Standard XDR primitives through xdropen
 * ================================================================ */

static int test_int_roundtrip(void) {
    XDR xd;
    const char *f = "tmp_test_int.xdr";
    int values[] = {0, 1, -1, INT_MAX, INT_MIN, 12345, -67890};
    int n = sizeof(values) / sizeof(values[0]);
    int i;

    xdropen(&xd, f, "w");
    for (i = 0; i < n; i++)
        xdr_int(&xd, &values[i]);
    xdrclose(&xd);

    xdropen(&xd, f, "r");
    for (i = 0; i < n; i++) {
        int v = 0;
        xdr_int(&xd, &v);
        ASSERT_INT_EQ(v, values[i]);
    }
    xdrclose(&xd);

    cleanup(f);
    return 0;
}

static int test_float_roundtrip(void) {
    XDR xd;
    const char *f = "tmp_test_float.xdr";
    float values[] = {0.0f, 1.0f, -1.0f, 3.14159f, 1e10f, -1e-10f, 1e30f};
    int n = sizeof(values) / sizeof(values[0]);
    int i;

    xdropen(&xd, f, "w");
    for (i = 0; i < n; i++)
        xdr_float(&xd, &values[i]);
    xdrclose(&xd);

    xdropen(&xd, f, "r");
    for (i = 0; i < n; i++) {
        float v = 0;
        xdr_float(&xd, &v);
        ASSERT_FLOAT_EQ(v, values[i], 1e-30f);
    }
    xdrclose(&xd);

    cleanup(f);
    return 0;
}

static int test_double_roundtrip(void) {
    XDR xd;
    const char *f = "tmp_test_double.xdr";
    double values[] = {0.0, 1.0, -1.0, 3.141592653589793, 1e100, -1e-100};
    int n = sizeof(values) / sizeof(values[0]);
    int i;

    xdropen(&xd, f, "w");
    for (i = 0; i < n; i++)
        xdr_double(&xd, &values[i]);
    xdrclose(&xd);

    xdropen(&xd, f, "r");
    for (i = 0; i < n; i++) {
        double v = 0;
        xdr_double(&xd, &v);
        ASSERT_DOUBLE_EQ(v, values[i], 1e-200);
    }
    xdrclose(&xd);

    cleanup(f);
    return 0;
}

static int test_short_roundtrip(void) {
    XDR xd;
    const char *f = "tmp_test_short.xdr";
    short values[] = {0, 1, -1, SHRT_MAX, SHRT_MIN, 1234, -5678};
    int n = sizeof(values) / sizeof(values[0]);
    int i;

    xdropen(&xd, f, "w");
    for (i = 0; i < n; i++)
        xdr_short(&xd, &values[i]);
    xdrclose(&xd);

    xdropen(&xd, f, "r");
    for (i = 0; i < n; i++) {
        short v = 0;
        xdr_short(&xd, &v);
        ASSERT_MSG(v == values[i], "short roundtrip mismatch");
    }
    xdrclose(&xd);

    cleanup(f);
    return 0;
}

static int test_char_roundtrip(void) {
    XDR xd;
    const char *f = "tmp_test_char.xdr";
    char values[] = {'A', 'z', '0', '\n', '\0'};
    int n = sizeof(values) / sizeof(values[0]);
    int i;

    xdropen(&xd, f, "w");
    for (i = 0; i < n; i++)
        xdr_char(&xd, &values[i]);
    xdrclose(&xd);

    xdropen(&xd, f, "r");
    for (i = 0; i < n; i++) {
        char v = '?';
        xdr_char(&xd, &v);
        ASSERT_MSG(v == values[i], "char roundtrip mismatch");
    }
    xdrclose(&xd);

    cleanup(f);
    return 0;
}

static int test_string_roundtrip(void) {
    XDR xd;
    const char *f = "tmp_test_string.xdr";
    char *str_write = "Hello, XDR world!";
    char *str_read = NULL;

    xdropen(&xd, f, "w");
    xdr_string(&xd, &str_write, 256);
    xdrclose(&xd);

    xdropen(&xd, f, "r");
    xdr_string(&xd, &str_read, 256);
    ASSERT_MSG(str_read != NULL, "xdr_string read should allocate");
    ASSERT_MSG(strcmp(str_read, str_write) == 0, "string content mismatch");
    free(str_read);
    xdrclose(&xd);

    cleanup(f);
    return 0;
}

static int test_bool_roundtrip(void) {
    XDR xd;
    const char *f = "tmp_test_bool.xdr";
    bool_t values[] = {TRUE, FALSE, TRUE, TRUE, FALSE};
    int n = sizeof(values) / sizeof(values[0]);
    int i;

    xdropen(&xd, f, "w");
    for (i = 0; i < n; i++)
        xdr_bool(&xd, &values[i]);
    xdrclose(&xd);

    xdropen(&xd, f, "r");
    for (i = 0; i < n; i++) {
        bool_t v = FALSE;
        xdr_bool(&xd, &v);
        ASSERT_MSG(v == values[i], "bool roundtrip mismatch");
    }
    xdrclose(&xd);

    cleanup(f);
    return 0;
}


/* ================================================================
 * TEST: Mixed types in a single file
 * ================================================================ */

static int test_mixed_types(void) {
    XDR xd;
    const char *f = "tmp_test_mixed.xdr";

    int wi = 42;
    float wf = 3.14f;
    double wd = 2.718281828;
    short ws = -1234;
    char wc = 'X';

    xdropen(&xd, f, "w");
    xdr_int(&xd, &wi);
    xdr_float(&xd, &wf);
    xdr_double(&xd, &wd);
    xdr_short(&xd, &ws);
    xdr_char(&xd, &wc);
    xdrclose(&xd);

    int ri = 0; float rf = 0; double rd = 0; short rs = 0; char rc = 0;
    xdropen(&xd, f, "r");
    xdr_int(&xd, &ri);
    xdr_float(&xd, &rf);
    xdr_double(&xd, &rd);
    xdr_short(&xd, &rs);
    xdr_char(&xd, &rc);
    xdrclose(&xd);

    ASSERT_INT_EQ(ri, 42);
    ASSERT_FLOAT_EQ(rf, 3.14f, 1e-6f);
    ASSERT_DOUBLE_EQ(rd, 2.718281828, 1e-9);
    ASSERT_MSG(rs == -1234, "short mismatch");
    ASSERT_MSG(rc == 'X', "char mismatch");

    cleanup(f);
    return 0;
}


/* ================================================================
 * TEST: xdr3dfcoord compression
 * ================================================================ */

/* Test with a very small set of coordinates (<=9 atoms).
 * These are stored uncompressed as raw floats. */
static int test_3dfcoord_small(void) {
    XDR xd;
    const char *f = "tmp_test_3df_small.xdr";
    int size = 5;
    float prec = 1000.0f;
    float coords[15], coords2[15];
    int i;

    /* Generate simple coordinates */
    for (i = 0; i < 15; i++)
        coords[i] = (float)(i) * 0.123f;

    xdropen(&xd, f, "w");
    int ret = xdr3dfcoord(&xd, coords, &size, &prec);
    ASSERT_MSG(ret != 0, "xdr3dfcoord write (small) should succeed");
    xdrclose(&xd);

    int rsize = 0;
    float rprec = 0;
    memset(coords2, 0, sizeof(coords2));
    xdropen(&xd, f, "r");
    ret = xdr3dfcoord(&xd, coords2, &rsize, &rprec);
    ASSERT_MSG(ret != 0, "xdr3dfcoord read (small) should succeed");
    ASSERT_INT_EQ(rsize, 5);
    xdrclose(&xd);

    for (i = 0; i < 15; i++)
        ASSERT_FLOAT_EQ(coords2[i], coords[i], 0.001f);

    cleanup(f);
    return 0;
}

/* Test with exactly 9 atoms (boundary of uncompressed path) */
static int test_3dfcoord_boundary_9(void) {
    XDR xd;
    const char *f = "tmp_test_3df_9.xdr";
    int size = 9;
    float prec = 1000.0f;
    float coords[27], coords2[27];
    int i;

    for (i = 0; i < 27; i++)
        coords[i] = (float)(i % 10) * 0.5f - 2.0f;

    xdropen(&xd, f, "w");
    xdr3dfcoord(&xd, coords, &size, &prec);
    xdrclose(&xd);

    int rsize = 0;
    float rprec = 0;
    memset(coords2, 0, sizeof(coords2));
    xdropen(&xd, f, "r");
    xdr3dfcoord(&xd, coords2, &rsize, &rprec);
    ASSERT_INT_EQ(rsize, 9);
    xdrclose(&xd);

    for (i = 0; i < 27; i++)
        ASSERT_FLOAT_EQ(coords2[i], coords[i], 0.001f);

    cleanup(f);
    return 0;
}

/* Test with 10 atoms (smallest compressed case) */
static int test_3dfcoord_10_atoms(void) {
    XDR xd;
    const char *f = "tmp_test_3df_10.xdr";
    int size = 10;
    float prec = 1000.0f;
    float coords[30], coords2[30];
    int i;

    for (i = 0; i < 30; i++)
        coords[i] = (float)(i % 7) * 0.25f + 1.0f;

    xdropen(&xd, f, "w");
    int ret = xdr3dfcoord(&xd, coords, &size, &prec);
    ASSERT_MSG(ret != 0, "xdr3dfcoord write should succeed for 10 atoms");
    xdrclose(&xd);

    int rsize = 0;
    float rprec = 0;
    memset(coords2, 0, sizeof(coords2));
    xdropen(&xd, f, "r");
    ret = xdr3dfcoord(&xd, coords2, &rsize, &rprec);
    ASSERT_MSG(ret != 0, "xdr3dfcoord read should succeed for 10 atoms");
    ASSERT_INT_EQ(rsize, 10);
    xdrclose(&xd);

    for (i = 0; i < 30; i++)
        ASSERT_FLOAT_EQ(coords2[i], coords[i], 0.002f);

    cleanup(f);
    return 0;
}

/* Test with a moderate number of atoms in a tight cluster */
static int test_3dfcoord_100_atoms(void) {
    XDR xd;
    const char *f = "tmp_test_3df_100.xdr";
    int size = 100;
    float prec = 1000.0f;
    int i;

    float *coords = (float *)malloc(300 * sizeof(float));
    float *coords2 = (float *)malloc(300 * sizeof(float));

    /* Simulate a small protein: atoms in a tight cluster */
    for (i = 0; i < 300; i++)
        coords[i] = 1.0f + (float)(i % 50) * 0.1f;

    xdropen(&xd, f, "w");
    int ret = xdr3dfcoord(&xd, coords, &size, &prec);
    ASSERT_MSG(ret != 0, "write 100 atoms should succeed");
    xdrclose(&xd);

    int rsize = 0;
    float rprec = 0;
    memset(coords2, 0, 300 * sizeof(float));
    xdropen(&xd, f, "r");
    ret = xdr3dfcoord(&xd, coords2, &rsize, &rprec);
    ASSERT_MSG(ret != 0, "read 100 atoms should succeed");
    ASSERT_INT_EQ(rsize, 100);
    xdrclose(&xd);

    for (i = 0; i < 300; i++)
        ASSERT_FLOAT_EQ(coords2[i], coords[i], 0.002f);

    free(coords);
    free(coords2);
    cleanup(f);
    return 0;
}

/* Test with varying precision values */
static int test_3dfcoord_precision(void) {
    XDR xd;
    const char *f = "tmp_test_3df_prec.xdr";
    int size = 20;
    int i;
    float coords[60], coords2[60];

    for (i = 0; i < 60; i++)
        coords[i] = (float)(i) * 0.01f;

    /* Low precision */
    float prec = 10.0f;
    xdropen(&xd, f, "w");
    xdr3dfcoord(&xd, coords, &size, &prec);
    xdrclose(&xd);

    int rsize = 0;
    float rprec = 0;
    xdropen(&xd, f, "r");
    xdr3dfcoord(&xd, coords2, &rsize, &rprec);
    xdrclose(&xd);

    ASSERT_FLOAT_EQ(rprec, 10.0f, 0.001f);
    for (i = 0; i < 60; i++)
        ASSERT_FLOAT_EQ(coords2[i], coords[i], 0.11f);  /* 1/10 precision */

    /* High precision */
    prec = 10000.0f;
    xdropen(&xd, f, "w");
    size = 20;
    xdr3dfcoord(&xd, coords, &size, &prec);
    xdrclose(&xd);

    rsize = 0;
    rprec = 0;
    xdropen(&xd, f, "r");
    xdr3dfcoord(&xd, coords2, &rsize, &rprec);
    xdrclose(&xd);

    ASSERT_FLOAT_EQ(rprec, 10000.0f, 0.001f);
    for (i = 0; i < 60; i++)
        ASSERT_FLOAT_EQ(coords2[i], coords[i], 0.0002f);  /* 1/10000 precision */

    cleanup(f);
    return 0;
}


/* ================================================================
 * TEST: Multiple frames
 * ================================================================ */

static int test_multiple_frames(void) {
    XDR xd;
    const char *f = "tmp_test_multi.xdr";
    int nframes = 5;
    int size = 50;
    float prec = 1000.0f;
    int i, j;

    float *coords = (float *)malloc(150 * sizeof(float));
    float *coords2 = (float *)malloc(150 * sizeof(float));

    /* Write multiple frames */
    xdropen(&xd, f, "w");
    xdr_int(&xd, &nframes);
    xdr_int(&xd, &size);
    for (j = 0; j < nframes; j++) {
        for (i = 0; i < 150; i++)
            coords[i] = (float)(j * 150 + i) * 0.001f;
        int ret = xdr3dfcoord(&xd, coords, &size, &prec);
        ASSERT_MSG(ret != 0, "write frame should succeed");
    }
    xdrclose(&xd);

    /* Read back all frames */
    xdropen(&xd, f, "r");
    int rnframes = 0, rsize = 0;
    float rprec = 0;
    xdr_int(&xd, &rnframes);
    xdr_int(&xd, &rsize);
    ASSERT_INT_EQ(rnframes, nframes);
    ASSERT_INT_EQ(rsize, size);

    for (j = 0; j < nframes; j++) {
        int rs = 0;
        int ret = xdr3dfcoord(&xd, coords2, &rs, &rprec);
        ASSERT_MSG(ret != 0, "read frame should succeed");
        ASSERT_INT_EQ(rs, size);

        /* Regenerate expected coords */
        for (i = 0; i < 150; i++)
            coords[i] = (float)(j * 150 + i) * 0.001f;
        for (i = 0; i < 150; i++)
            ASSERT_FLOAT_EQ(coords2[i], coords[i], 0.002f);
    }
    xdrclose(&xd);

    free(coords);
    free(coords2);
    cleanup(f);
    return 0;
}


/* ================================================================
 * TEST: Negative coordinates
 * ================================================================ */

static int test_negative_coords(void) {
    XDR xd;
    const char *f = "tmp_test_neg.xdr";
    int size = 20;
    float prec = 1000.0f;
    int i;

    float coords[60], coords2[60];
    for (i = 0; i < 60; i++)
        coords[i] = -5.0f + (float)(i) * 0.1f;  /* range: -5.0 to 0.9 */

    xdropen(&xd, f, "w");
    xdr3dfcoord(&xd, coords, &size, &prec);
    xdrclose(&xd);

    int rsize = 0;
    float rprec = 0;
    xdropen(&xd, f, "r");
    xdr3dfcoord(&xd, coords2, &rsize, &rprec);
    xdrclose(&xd);

    ASSERT_INT_EQ(rsize, 20);
    for (i = 0; i < 60; i++)
        ASSERT_FLOAT_EQ(coords2[i], coords[i], 0.002f);

    cleanup(f);
    return 0;
}


/* ================================================================
 * TEST: Large coordinate values
 * ================================================================ */

static int test_large_coords(void) {
    XDR xd;
    const char *f = "tmp_test_large.xdr";
    int size = 20;
    float prec = 100.0f;
    int i;

    float coords[60], coords2[60];
    for (i = 0; i < 60; i++)
        coords[i] = 1000.0f + (float)(i) * 10.0f;

    xdropen(&xd, f, "w");
    xdr3dfcoord(&xd, coords, &size, &prec);
    xdrclose(&xd);

    int rsize = 0;
    float rprec = 0;
    xdropen(&xd, f, "r");
    xdr3dfcoord(&xd, coords2, &rsize, &rprec);
    xdrclose(&xd);

    ASSERT_INT_EQ(rsize, 20);
    for (i = 0; i < 60; i++)
        ASSERT_FLOAT_EQ(coords2[i], coords[i], 0.02f);

    cleanup(f);
    return 0;
}


/* ================================================================
 * TEST: Append mode
 * ================================================================ */

static int test_append_mode(void) {
    XDR xd;
    const char *f = "tmp_test_append.xdr";
    int i;

    /* Write first batch */
    xdropen(&xd, f, "w");
    int n1 = 3;
    xdr_int(&xd, &n1);
    for (i = 0; i < n1; i++) {
        int v = i * 10;
        xdr_int(&xd, &v);
    }
    xdrclose(&xd);

    /* Append second batch */
    xdropen(&xd, f, "a");
    int n2 = 2;
    xdr_int(&xd, &n2);
    for (i = 0; i < n2; i++) {
        int v = 100 + i;
        xdr_int(&xd, &v);
    }
    xdrclose(&xd);

    /* Read everything back */
    xdropen(&xd, f, "r");
    int rn1 = 0;
    xdr_int(&xd, &rn1);
    ASSERT_INT_EQ(rn1, 3);
    for (i = 0; i < 3; i++) {
        int v = 0;
        xdr_int(&xd, &v);
        ASSERT_INT_EQ(v, i * 10);
    }
    int rn2 = 0;
    xdr_int(&xd, &rn2);
    ASSERT_INT_EQ(rn2, 2);
    for (i = 0; i < 2; i++) {
        int v = 0;
        xdr_int(&xd, &v);
        ASSERT_INT_EQ(v, 100 + i);
    }
    xdrclose(&xd);

    cleanup(f);
    return 0;
}


/* ================================================================
 * TEST: Multiple simultaneous open files
 * ================================================================ */

static int test_multiple_open_files(void) {
    XDR xd1, xd2;
    const char *f1 = "tmp_test_multi1.xdr";
    const char *f2 = "tmp_test_multi2.xdr";

    int id1 = xdropen(&xd1, f1, "w");
    int id2 = xdropen(&xd2, f2, "w");
    ASSERT_MSG(id1 != 0, "first open should succeed");
    ASSERT_MSG(id2 != 0, "second open should succeed");
    ASSERT_MSG(id1 != id2, "ids should be different");

    int v1 = 111, v2 = 222;
    xdr_int(&xd1, &v1);
    xdr_int(&xd2, &v2);
    xdrclose(&xd1);
    xdrclose(&xd2);

    /* Read back independently */
    xdropen(&xd1, f1, "r");
    xdropen(&xd2, f2, "r");
    int r1 = 0, r2 = 0;
    xdr_int(&xd1, &r1);
    xdr_int(&xd2, &r2);
    ASSERT_INT_EQ(r1, 111);
    ASSERT_INT_EQ(r2, 222);
    xdrclose(&xd1);
    xdrclose(&xd2);

    cleanup(f1);
    cleanup(f2);
    return 0;
}


/* ================================================================
 * TEST: Reopen same file multiple times
 * ================================================================ */

static int test_reopen_file(void) {
    XDR xd;
    const char *f = "tmp_test_reopen.xdr";
    int i;

    /* Write, close, read, close -- repeated */
    for (i = 1; i <= 3; i++) {
        xdropen(&xd, f, "w");
        int v = i * 100;
        xdr_int(&xd, &v);
        xdrclose(&xd);

        xdropen(&xd, f, "r");
        int rv = 0;
        xdr_int(&xd, &rv);
        ASSERT_INT_EQ(rv, i * 100);
        xdrclose(&xd);
    }

    cleanup(f);
    return 0;
}


/* ================================================================
 * TEST: xdr3dfcoord with water-like molecule pattern
 * (O-H-H triples that trigger the atom-swap optimization)
 * ================================================================ */

static int test_3dfcoord_water_pattern(void) {
    XDR xd;
    const char *f = "tmp_test_water.xdr";
    int nwaters = 100;
    int size = nwaters * 3;  /* O + H + H per water */
    float prec = 1000.0f;
    int i;

    float *coords = (float *)malloc(size * 3 * sizeof(float));
    float *coords2 = (float *)malloc(size * 3 * sizeof(float));

    /* Generate water-like coordinates: each water has O, H1, H2
     * with H atoms close to O (within ~0.1 nm as in real water) */
    for (i = 0; i < nwaters; i++) {
        float cx = (float)(i % 10) * 0.3f;
        float cy = (float)((i / 10) % 10) * 0.3f;
        float cz = (float)(i / 100) * 0.3f;
        /* Oxygen */
        coords[(i*3+0)*3 + 0] = cx;
        coords[(i*3+0)*3 + 1] = cy;
        coords[(i*3+0)*3 + 2] = cz;
        /* Hydrogen 1 (close to O) */
        coords[(i*3+1)*3 + 0] = cx + 0.096f;
        coords[(i*3+1)*3 + 1] = cy;
        coords[(i*3+1)*3 + 2] = cz;
        /* Hydrogen 2 (close to O) */
        coords[(i*3+2)*3 + 0] = cx - 0.024f;
        coords[(i*3+2)*3 + 1] = cy + 0.093f;
        coords[(i*3+2)*3 + 2] = cz;
    }

    xdropen(&xd, f, "w");
    int ret = xdr3dfcoord(&xd, coords, &size, &prec);
    ASSERT_MSG(ret != 0, "write water coords should succeed");
    xdrclose(&xd);

    int rsize = 0;
    float rprec = 0;
    xdropen(&xd, f, "r");
    ret = xdr3dfcoord(&xd, coords2, &rsize, &rprec);
    ASSERT_MSG(ret != 0, "read water coords should succeed");
    ASSERT_INT_EQ(rsize, size);
    xdrclose(&xd);

    for (i = 0; i < size * 3; i++)
        ASSERT_FLOAT_EQ(coords2[i], coords[i], 0.002f);

    free(coords);
    free(coords2);
    cleanup(f);
    return 0;
}


/* ================================================================
 * TEST: xdr3dfcoord with 1000 atoms (larger dataset)
 * ================================================================ */

static int test_3dfcoord_1000_atoms(void) {
    XDR xd;
    const char *f = "tmp_test_3df_1000.xdr";
    int size = 1000;
    float prec = 1000.0f;
    int i;

    float *coords = (float *)malloc(3000 * sizeof(float));
    float *coords2 = (float *)malloc(3000 * sizeof(float));

    /* Simulate atoms distributed in a 10x10x10 nm box */
    for (i = 0; i < 3000; i++)
        coords[i] = (float)(i % 1000) * 0.01f;

    xdropen(&xd, f, "w");
    int ret = xdr3dfcoord(&xd, coords, &size, &prec);
    ASSERT_MSG(ret != 0, "write 1000 atoms should succeed");
    xdrclose(&xd);

    int rsize = 0;
    float rprec = 0;
    xdropen(&xd, f, "r");
    ret = xdr3dfcoord(&xd, coords2, &rsize, &rprec);
    ASSERT_MSG(ret != 0, "read 1000 atoms should succeed");
    ASSERT_INT_EQ(rsize, 1000);
    xdrclose(&xd);

    for (i = 0; i < 3000; i++)
        ASSERT_FLOAT_EQ(coords2[i], coords[i], 0.002f);

    free(coords);
    free(coords2);
    cleanup(f);
    return 0;
}


/* ================================================================
 * TEST: Compression effectiveness
 * (verify compressed file is smaller than raw float storage)
 * ================================================================ */

static int test_compression_ratio(void) {
    XDR xd;
    const char *f = "tmp_test_compress.xdr";
    int size = 500;
    float prec = 1000.0f;
    int i;
    long filesize;

    float *coords = (float *)malloc(1500 * sizeof(float));

    /* Clustered coordinates (good for compression) */
    for (i = 0; i < 1500; i++)
        coords[i] = 1.0f + (float)(i % 100) * 0.05f;

    xdropen(&xd, f, "w");
    xdr3dfcoord(&xd, coords, &size, &prec);
    xdrclose(&xd);

    /* Check file size */
    FILE *fp = fopen(f, "rb");
    fseek(fp, 0, SEEK_END);
    filesize = ftell(fp);
    fclose(fp);

    long raw_size = 1500 * sizeof(float);  /* 6000 bytes */
    ASSERT_MSG(filesize < raw_size,
        "compressed file should be smaller than raw float storage");

    free(coords);
    cleanup(f);
    return 0;
}


/* ================================================================
 * TEST: xdr_setpos / xdr_getpos through xdropen
 * ================================================================ */

static int test_setpos_getpos(void) {
    XDR xd;
    const char *f = "tmp_test_pos.xdr";

    xdropen(&xd, f, "w");
    int v1 = 11, v2 = 22, v3 = 33;
    xdr_int(&xd, &v1);
    unsigned int pos_after_v1 = xdr_getpos(&xd);
    xdr_int(&xd, &v2);
    xdr_int(&xd, &v3);
    xdrclose(&xd);

    /* Read: skip first int using setpos */
    xdropen(&xd, f, "r");
    xdr_setpos(&xd, pos_after_v1);
    int rv2 = 0;
    xdr_int(&xd, &rv2);
    ASSERT_INT_EQ(rv2, 22);
    int rv3 = 0;
    xdr_int(&xd, &rv3);
    ASSERT_INT_EQ(rv3, 33);
    xdrclose(&xd);

    cleanup(f);
    return 0;
}


/* ================================================================
 * TEST: xdr3dfcoord with identical coordinates
 * (worst case for delta encoding)
 * ================================================================ */

static int test_3dfcoord_identical_coords(void) {
    XDR xd;
    const char *f = "tmp_test_3df_ident.xdr";
    int size = 50;
    float prec = 1000.0f;
    int i;

    float coords[150], coords2[150];
    for (i = 0; i < 150; i += 3) {
        coords[i]   = 1.234f;
        coords[i+1] = 5.678f;
        coords[i+2] = 9.012f;
    }

    xdropen(&xd, f, "w");
    int ret = xdr3dfcoord(&xd, coords, &size, &prec);
    ASSERT_MSG(ret != 0, "write identical coords should succeed");
    xdrclose(&xd);

    int rsize = 0;
    float rprec = 0;
    xdropen(&xd, f, "r");
    ret = xdr3dfcoord(&xd, coords2, &rsize, &rprec);
    ASSERT_MSG(ret != 0, "read identical coords should succeed");
    ASSERT_INT_EQ(rsize, 50);
    xdrclose(&xd);

    for (i = 0; i < 150; i++)
        ASSERT_FLOAT_EQ(coords2[i], coords[i], 0.002f);

    cleanup(f);
    return 0;
}


/* ================================================================
 * TEST: xdr3dfcoord with widely scattered coordinates
 * (stress test for large ranges)
 * ================================================================ */

static int test_3dfcoord_scattered(void) {
    XDR xd;
    const char *f = "tmp_test_3df_scatter.xdr";
    int size = 30;
    float prec = 100.0f;
    int i;

    float coords[90], coords2[90];
    for (i = 0; i < 90; i++)
        coords[i] = (i % 2 == 0) ? (float)(i) * 100.0f : (float)(-i) * 100.0f;

    xdropen(&xd, f, "w");
    int ret = xdr3dfcoord(&xd, coords, &size, &prec);
    /* This may or may not succeed depending on overflow — just test the path */
    xdrclose(&xd);

    if (ret != 0) {
        int rsize = 0;
        float rprec = 0;
        xdropen(&xd, f, "r");
        xdr3dfcoord(&xd, coords2, &rsize, &rprec);
        ASSERT_INT_EQ(rsize, 30);
        xdrclose(&xd);

        for (i = 0; i < 90; i++)
            ASSERT_FLOAT_EQ(coords2[i], coords[i], 0.02f);
    }

    cleanup(f);
    return 0;
}


/* ================================================================
 * TEST: xdr3dfcoord intermixed with other XDR data
 * (simulating a real trajectory file format)
 * ================================================================ */

static int test_3dfcoord_with_metadata(void) {
    XDR xd;
    const char *f = "tmp_test_3df_meta.xdr";
    int size = 50;
    float prec = 1000.0f;
    int i;

    float *coords = (float *)malloc(150 * sizeof(float));
    float *coords2 = (float *)malloc(150 * sizeof(float));
    for (i = 0; i < 150; i++)
        coords[i] = (float)(i) * 0.032f;

    /* Write: header + 3dfcoord + trailer */
    xdropen(&xd, f, "w");
    int step = 42;
    float time_val = 1.5f;
    float box[9] = {3.0f, 0, 0, 0, 3.0f, 0, 0, 0, 3.0f};
    xdr_int(&xd, &step);
    xdr_float(&xd, &time_val);
    for (i = 0; i < 9; i++)
        xdr_float(&xd, &box[i]);
    xdr3dfcoord(&xd, coords, &size, &prec);
    int checksum = 12345;
    xdr_int(&xd, &checksum);
    xdrclose(&xd);

    /* Read back */
    xdropen(&xd, f, "r");
    int rstep = 0;
    float rtime = 0;
    float rbox[9];
    xdr_int(&xd, &rstep);
    xdr_float(&xd, &rtime);
    for (i = 0; i < 9; i++)
        xdr_float(&xd, &rbox[i]);
    ASSERT_INT_EQ(rstep, 42);
    ASSERT_FLOAT_EQ(rtime, 1.5f, 1e-6f);
    ASSERT_FLOAT_EQ(rbox[0], 3.0f, 1e-6f);
    ASSERT_FLOAT_EQ(rbox[4], 3.0f, 1e-6f);

    int rsize = 0;
    float rprec = 0;
    xdr3dfcoord(&xd, coords2, &rsize, &rprec);
    ASSERT_INT_EQ(rsize, 50);

    int rchecksum = 0;
    xdr_int(&xd, &rchecksum);
    ASSERT_INT_EQ(rchecksum, 12345);
    xdrclose(&xd);

    for (i = 0; i < 150; i++)
        ASSERT_FLOAT_EQ(coords2[i], coords[i], 0.002f);

    free(coords);
    free(coords2);
    cleanup(f);
    return 0;
}


/* ================================================================
 * MAIN
 * ================================================================ */

int main(void) {
    fprintf(stderr, "\n=== xdrf C API test suite ===\n\n");

    fprintf(stderr, "--- Open/Close ---\n");
    RUN_TEST(test_open_close_write);
    RUN_TEST(test_open_close_read);
    RUN_TEST(test_open_nonexistent_read);

    fprintf(stderr, "\n--- XDR primitive round-trips ---\n");
    RUN_TEST(test_int_roundtrip);
    RUN_TEST(test_float_roundtrip);
    RUN_TEST(test_double_roundtrip);
    RUN_TEST(test_short_roundtrip);
    RUN_TEST(test_char_roundtrip);
    RUN_TEST(test_string_roundtrip);
    RUN_TEST(test_bool_roundtrip);
    RUN_TEST(test_mixed_types);

    fprintf(stderr, "\n--- xdr3dfcoord ---\n");
    RUN_TEST(test_3dfcoord_small);
    RUN_TEST(test_3dfcoord_boundary_9);
    RUN_TEST(test_3dfcoord_10_atoms);
    RUN_TEST(test_3dfcoord_100_atoms);
    RUN_TEST(test_3dfcoord_1000_atoms);
    RUN_TEST(test_3dfcoord_precision);
    RUN_TEST(test_negative_coords);
    RUN_TEST(test_large_coords);
    RUN_TEST(test_3dfcoord_identical_coords);
    RUN_TEST(test_3dfcoord_scattered);
    RUN_TEST(test_3dfcoord_water_pattern);

    fprintf(stderr, "\n--- Advanced ---\n");
    RUN_TEST(test_append_mode);
    RUN_TEST(test_multiple_open_files);
    RUN_TEST(test_reopen_file);
    RUN_TEST(test_setpos_getpos);
    RUN_TEST(test_multiple_frames);
    RUN_TEST(test_3dfcoord_with_metadata);
    RUN_TEST(test_compression_ratio);

    fprintf(stderr, "\n=== Results: %d passed, %d failed, %d total ===\n\n",
            tests_passed, tests_failed, tests_run);

    return tests_failed > 0 ? 1 : 0;
}
