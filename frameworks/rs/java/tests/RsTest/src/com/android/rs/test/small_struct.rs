#include "shared.rsh"

int gDimX;
int gDimY;

rs_allocation A;
rs_allocation B;

static int gIntStart = 0x7;
static long gLongStart = 0x12345678abcdef12;

typedef struct small_struct {
    int i;
    long l;
} small_struct;

#define ARRAY_LEN 3

typedef struct struct_of_struct {
    small_struct arr[ARRAY_LEN];
} struct_of_struct;

void test() {
    bool failed = false;
    for (int x = 0; x < gDimX; x ++) {
        for (int y = 0; y < gDimY; y ++) {
            small_struct *v = (small_struct *) rsGetElementAt(A, x, y);
            _RS_ASSERT_EQU(v->i, gIntStart + y * gDimX + x);
            _RS_ASSERT_EQU(v->l, gLongStart + y * gDimX + x);
        }
    }

    for (int x = 0; x < gDimX; x ++) {
        for (int y = 0; y < gDimY; y ++) {
            struct_of_struct *v = (struct_of_struct *) rsGetElementAt(B, x, y);
            for (int idx = 0; idx < ARRAY_LEN; idx ++) {
                _RS_ASSERT_EQU((*v).arr[idx].i, gIntStart + y * gDimX + x + idx);
                _RS_ASSERT_EQU((*v).arr[idx].l, gLongStart + y * gDimX + x + idx);
            }
        }
    }

    if (failed) {
        rsDebug("small_struct test FAILED", 0);
        rsSendToClientBlocking(RS_MSG_TEST_FAILED);
    }
    else {
        rsDebug("small_struct test PASSED", 0);
        rsSendToClientBlocking(RS_MSG_TEST_PASSED);
    }
}

small_struct RS_KERNEL setStruct(int x, int y) {
    small_struct output;
    output.i = gIntStart + y * gDimX + x;
    output.l = gLongStart + y * gDimX + x;
    return output;
}

struct_of_struct RS_KERNEL setArrayOfStruct(int x, int y) {
    struct_of_struct output;
    for (int idx = 0; idx < ARRAY_LEN; idx ++) {
        output.arr[idx].i = gIntStart + y * gDimX + x + idx;
        output.arr[idx].l = gLongStart + y * gDimX + x + idx;
    }
    return output;
}
