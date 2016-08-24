#include "shared.rsh"

rs_allocation A;
rs_allocation B;
uint32_t gDimX, gDimY;
static bool failed = false;

void init_vars(int *out) {
    *out = 7;
}

void xform(const int *in, int *out, rs_kernel_context context, uint32_t x, uint32_t y) {
    if (!_RS_ASSERT_EQU(*in, 7))
        rsDebug("xform at x, y", x, y);
    uint32_t dimX = rsGetDimX(context);
    uint32_t dimY = rsGetDimY(context);
    _RS_ASSERT_EQU(dimX, gDimX);
    _RS_ASSERT_EQU(dimY, gDimY);
    *out = *in + x + dimX * y;
}

static bool test_xform_output() {
    bool failed = false;
    int i, j;

    for (i = 0; i < gDimX; i++) {
        for (j = 0; j < gDimY; j++) {
            int bElt = rsGetElementAt_int(B, i, j);
            int aElt = rsGetElementAt_int(A, i, j);
            if (!_RS_ASSERT_EQU(bElt, (aElt + i + gDimX * j)))
                rsDebug("test_xform_output at i, j", i, j);
        }
    }

    if (failed) {
        rsDebug("kernel2d (old style) test_xform_output FAILED", 0);
    }
    else {
        rsDebug("kernel2d (old style) test_xform_output PASSED", 0);
    }

    return failed;
}

void verify_xform() {
    failed |= test_xform_output();
}

void kernel_test() {
    if (failed) {
        rsSendToClientBlocking(RS_MSG_TEST_FAILED);
    }
    else {
        rsSendToClientBlocking(RS_MSG_TEST_PASSED);
    }
}
