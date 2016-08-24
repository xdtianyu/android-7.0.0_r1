#include "shared.rsh"

int dimX;
int dimY;
int xStart = 0;
int xEnd = 0;
int yStart = 0;
int yEnd = 0;

rs_script s;
rs_allocation aRaw;
rs_allocation ain;
rs_allocation aout;

void root(int *out, uint32_t x, uint32_t y) {
    *out = x + y * dimX;
}

int RS_KERNEL zero() {
    return 0;
}

void foreach_bounds_test() {
    static bool failed = false;

    if (failed) {
        rsSendToClientBlocking(RS_MSG_TEST_FAILED);
    }
    else {
        rsSendToClientBlocking(RS_MSG_TEST_PASSED);
    }
}

