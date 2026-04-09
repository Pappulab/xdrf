/*_________________________________________________________________
 |
 | xdrf.hpp - C++ header-only wrapper for the xdrf library.
 |
 | Provides an RAII-based XdrFile class that wraps xdropen/xdrclose
 | and exposes typed read/write methods for XDR primitives and
 | compressed 3D coordinates.
 |
 | Usage:
 |   #include "xdrf.hpp"
 |
 |   // Write
 |   xdrf::XdrFile out("data.xdr", "w");
 |   out.write_int(42);
 |   out.write_float(3.14f);
 |   std::vector<float> coords = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f};
 |   out.write_3dfcoord(coords, 1000.0f);
 |   out.close();   // or let destructor handle it
 |
 |   // Read
 |   xdrf::XdrFile in("data.xdr", "r");
 |   int val = in.read_int();
 |   float fval = in.read_float();
 |   auto [atoms, prec] = in.read_3dfcoord();
 |
 | The class is non-copyable but movable. All read/write methods
 | throw std::runtime_error on failure.
 |
 | Link against libxdrf.a -lm (same as the C interface).
 |
*/

#ifndef XDRF_HPP
#define XDRF_HPP

#include <cstdio>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <rpc/rpc.h>
#include <rpc/xdr.h>

/* Pull in the C API */
extern "C" {
#include "xdrf.h"
}

namespace xdrf {

/*_________________________________________________________________
 |
 | XdrFile - RAII wrapper around the xdrf C API.
 |
 | Opens an XDR file on construction, closes on destruction.
 | Provides typed read/write methods for all XDR primitives
 | and for compressed 3D coordinates via xdr3dfcoord.
 |
*/
class XdrFile {
public:
    /* Open an XDR file.
     * mode: "w" = write, "r" = read, "a" = append.
     * Throws std::runtime_error if the file cannot be opened. */
    XdrFile(const std::string &filename, const std::string &mode)
        : xdr_(new XDR), id_(0), open_(false)
    {
        id_ = xdropen(xdr_.get(), filename.c_str(), mode.c_str());
        if (id_ == 0) {
            throw std::runtime_error("xdropen failed: " + filename);
        }
        open_ = true;
    }

    ~XdrFile() {
        if (open_) {
            xdrclose(xdr_.get());
        }
    }

    /* Non-copyable */
    XdrFile(const XdrFile &) = delete;
    XdrFile &operator=(const XdrFile &) = delete;

    /* Movable */
    XdrFile(XdrFile &&other) noexcept
        : xdr_(std::move(other.xdr_)), id_(other.id_), open_(other.open_)
    {
        other.open_ = false;
        other.id_ = 0;
    }

    XdrFile &operator=(XdrFile &&other) noexcept {
        if (this != &other) {
            if (open_) xdrclose(xdr_.get());
            xdr_ = std::move(other.xdr_);
            id_ = other.id_;
            open_ = other.open_;
            other.open_ = false;
            other.id_ = 0;
        }
        return *this;
    }

    /* Close the file explicitly. Safe to call multiple times. */
    void close() {
        if (open_) {
            xdrclose(xdr_.get());
            open_ = false;
            id_ = 0;
        }
    }

    /* Returns true if the file is currently open. */
    bool is_open() const { return open_; }

    /* Returns the xdrid handle. */
    int id() const { return id_; }

    /* Access the underlying XDR struct (for advanced use). */
    XDR *xdr() { return xdr_.get(); }

    /* ---- Primitive write methods ---- */

    void write_int(int val) {
        check_open();
        if (!xdr_int(xdr_.get(), &val))
            throw std::runtime_error("xdr_int write failed");
    }

    void write_float(float val) {
        check_open();
        if (!xdr_float(xdr_.get(), &val))
            throw std::runtime_error("xdr_float write failed");
    }

    void write_double(double val) {
        check_open();
        if (!xdr_double(xdr_.get(), &val))
            throw std::runtime_error("xdr_double write failed");
    }

    void write_short(short val) {
        check_open();
        if (!xdr_short(xdr_.get(), &val))
            throw std::runtime_error("xdr_short write failed");
    }

    void write_char(char val) {
        check_open();
        if (!xdr_char(xdr_.get(), &val))
            throw std::runtime_error("xdr_char write failed");
    }

    void write_bool(bool val) {
        check_open();
        bool_t bval = val ? TRUE : FALSE;
        if (!xdr_bool(xdr_.get(), &bval))
            throw std::runtime_error("xdr_bool write failed");
    }

    void write_string(const std::string &val) {
        check_open();
        /* xdr_string needs a char* (it won't modify on encode) */
        char *buf = const_cast<char *>(val.c_str());
        unsigned int maxlen = static_cast<unsigned int>(val.size() + 1);
        if (!xdr_string(xdr_.get(), &buf, maxlen))
            throw std::runtime_error("xdr_string write failed");
    }

    /* ---- Primitive read methods ---- */

    int read_int() {
        check_open();
        int val = 0;
        if (!xdr_int(xdr_.get(), &val))
            throw std::runtime_error("xdr_int read failed");
        return val;
    }

    float read_float() {
        check_open();
        float val = 0;
        if (!xdr_float(xdr_.get(), &val))
            throw std::runtime_error("xdr_float read failed");
        return val;
    }

    double read_double() {
        check_open();
        double val = 0;
        if (!xdr_double(xdr_.get(), &val))
            throw std::runtime_error("xdr_double read failed");
        return val;
    }

    short read_short() {
        check_open();
        short val = 0;
        if (!xdr_short(xdr_.get(), &val))
            throw std::runtime_error("xdr_short read failed");
        return val;
    }

    char read_char() {
        check_open();
        char val = 0;
        if (!xdr_char(xdr_.get(), &val))
            throw std::runtime_error("xdr_char read failed");
        return val;
    }

    bool read_bool() {
        check_open();
        bool_t val = FALSE;
        if (!xdr_bool(xdr_.get(), &val))
            throw std::runtime_error("xdr_bool read failed");
        return val != FALSE;
    }

    std::string read_string(unsigned int maxlen = 4096) {
        check_open();
        char *buf = nullptr;
        if (!xdr_string(xdr_.get(), &buf, maxlen))
            throw std::runtime_error("xdr_string read failed");
        std::string result(buf);
        free(buf);
        return result;
    }

    /* ---- 3D coordinate methods ---- */

    /* Write compressed 3D coordinates.
     * coords:    flat array of 3*num_atoms floats (x,y,z triples).
     * precision: float-to-fixed-point multiplier (e.g. 1000.0f). */
    void write_3dfcoord(const std::vector<float> &coords, float precision) {
        check_open();
        if (coords.size() % 3 != 0)
            throw std::runtime_error("coords size must be a multiple of 3");
        int size = static_cast<int>(coords.size() / 3);
        /* xdr3dfcoord won't modify the data on encode, but the
         * C API takes non-const pointers */
        float *fp = const_cast<float *>(coords.data());
        if (!xdr3dfcoord(xdr_.get(), fp, &size, &precision))
            throw std::runtime_error("xdr3dfcoord write failed");
    }

    /* Write compressed 3D coordinates from a raw pointer.
     * fp:        array of 3*num_atoms floats.
     * num_atoms: number of atoms.
     * precision: float-to-fixed-point multiplier. */
    void write_3dfcoord(float *fp, int num_atoms, float precision) {
        check_open();
        if (!xdr3dfcoord(xdr_.get(), fp, &num_atoms, &precision))
            throw std::runtime_error("xdr3dfcoord write failed");
    }

    /* Read compressed 3D coordinates.
     * Returns a pair of (coords vector, precision used). */
    std::pair<std::vector<float>, float> read_3dfcoord() {
        check_open();
        /* First pass: read size and precision by providing a large buffer.
         * xdr3dfcoord reads the size from the stream. */
        int size = 0;
        float precision = 0;

        /* We need to know the size before allocating. The xdr3dfcoord
         * protocol writes the size first, so we read it, allocate,
         * then read the data. Unfortunately the C API does both in one
         * call, so we must over-allocate or use a two-step approach.
         *
         * The safe approach: allocate a reasonable buffer, let
         * xdr3dfcoord fill size, and resize afterwards. We start with
         * a generous initial allocation. */
        std::vector<float> coords(30000); /* 10000 atoms */
        size = 0; /* 0 = accept whatever count is in the file */

        if (!xdr3dfcoord(xdr_.get(), coords.data(), &size, &precision))
            throw std::runtime_error("xdr3dfcoord read failed");

        coords.resize(static_cast<size_t>(size) * 3);
        return {coords, precision};
    }

    /* Read 3D coordinates into a caller-provided buffer.
     * fp:        pre-allocated array (must hold at least 3*max_atoms floats).
     * max_atoms: capacity; overwritten with actual atom count on return.
     * precision: set on return to the precision stored in the file. */
    void read_3dfcoord(float *fp, int &num_atoms, float &precision) {
        check_open();
        if (!xdr3dfcoord(xdr_.get(), fp, &num_atoms, &precision))
            throw std::runtime_error("xdr3dfcoord read failed");
    }

    /* ---- Position methods ---- */

    /* Get the current stream position. */
    unsigned int getpos() {
        check_open();
        return xdr_getpos(xdr_.get());
    }

    /* Set the stream position. Returns true on success. */
    bool setpos(unsigned int pos) {
        check_open();
        return xdr_setpos(xdr_.get(), pos) != 0;
    }

private:
    std::unique_ptr<XDR> xdr_;
    int id_;
    bool open_;

    void check_open() const {
        if (!open_)
            throw std::runtime_error("XdrFile is not open");
    }
};

} /* namespace xdrf */

#endif /* XDRF_HPP */
