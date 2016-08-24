#include "shared.rsh"

// Testing primitive types
float floatTest = 1.99f;
float2 float2Test = {2.99f, 12.99f};
float3 float3Test = {3.99f, 13.99f, 23.99f};
float4 float4Test = {4.99f, 14.99f, 24.99f, 34.99f};
double doubleTest = 2.05;
double2 double2Test = {2.05, 12.05};
double3 double3Test = {3.05, 13.05, 23.05};
double4 double4Test = {4.05, 14.05, 24.05, 34.05};
char charTest = -8;
short shortTest = -16;
int intTest = -32;
long longTest = 17179869184l; // 1 << 34
long long longlongTest = 68719476736l; // 1 << 36

uchar ucharTest = 8;
ushort ushortTest = 16;
uint uintTest = 32;
ulong ulongTest = 4611686018427387904L;
int64_t int64_tTest = -17179869184l; // - 1 << 34
uint64_t uint64_tTest = 117179869184l;

static bool basic_test(uint32_t index) {
    bool failed = false;

    // This test focuses primarily on compilation-time, not run-time.
    // For this reason, none of the outputs are actually checked.

    // http://b/27526302 - globals of half type cannot be exported and fail compilation
    half halfTest = (half) 1.5f;
    half2 half2Test = {(half) 1.5f, (half) 2.5f};
    half3 half3Test = {(half) 1.5f, (half) 2.5f, (half) 3.5f};
    half4 half4Test = {(half) 0.f, (half) -0.f, (half) 1.f/0.f, (half) 0.f/0.f};

    rsDebug("halfTest", halfTest);
    rsDebug("half2Test", half2Test);
    rsDebug("half3Test", half3Test);
    rsDebug("half4Test", half4Test);

    rsDebug("floatTest", floatTest);
    rsDebug("float2Test", float2Test);
    rsDebug("float3Test", float3Test);
    rsDebug("float4Test", float4Test);
    rsDebug("doubleTest", doubleTest);
    rsDebug("double2Test", double2Test);
    rsDebug("double3Test", double3Test);
    rsDebug("double4Test", double4Test);
    rsDebug("charTest", charTest);
    rsDebug("shortTest", shortTest);
    rsDebug("intTest", intTest);
    rsDebug("longTest", longTest);
    rsDebug("longlongTest", longlongTest);

    rsDebug("ucharTest", ucharTest);
    rsDebug("ushortTest", ushortTest);
    rsDebug("uintTest", uintTest);
    rsDebug("ulongTest", ulongTest);
    rsDebug("int64_tTest", int64_tTest);
    rsDebug("uint64_tTest", uint64_tTest);

    return failed;
}

void test_rsdebug(uint32_t index, int test_num) {
    bool failed = false;
    failed |= basic_test(index);

    if (failed) {
        rsSendToClientBlocking(RS_MSG_TEST_FAILED);
        rsDebug("rsdebug_test FAILED", -1);
    }
    else {
        rsSendToClientBlocking(RS_MSG_TEST_PASSED);
        rsDebug("rsdebug_test PASSED", 0);
    }
}

