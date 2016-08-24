#include "shared.rsh"

half gHalf;
half2 gHalf2;
half3 gHalf3;
half4 gHalf4;

static bool failed = false;

void validateHalf(half h) {
    _RS_ASSERT_EQU((float) h, 10.f);
}

void validateHalf2(half2 h2) {
    _RS_ASSERT_EQU((float) h2.x, 10.f);
    _RS_ASSERT_EQU((float) h2.y, 11.f);
}

void validateHalf3(half3 h3) {
    _RS_ASSERT_EQU((float) h3.x, 10.f);
    _RS_ASSERT_EQU((float) h3.y, 11.f);
    _RS_ASSERT_EQU((float) h3.z, -12.f);
}

void validateHalf4(half4 h4) {
    _RS_ASSERT_EQU((float) h4.x, 10.f);
    _RS_ASSERT_EQU((float) h4.y, 11.f);
    _RS_ASSERT_EQU((float) h4.z, -12.f);
    _RS_ASSERT_EQU((float) h4.w, -13.f);
}

void test(half h, half2 h2, half3 h3, half4 h4) {
    validateHalf(gHalf);
    validateHalf2(gHalf2);
    validateHalf3(gHalf3);
    validateHalf4(gHalf4);

    validateHalf(h);
    validateHalf2(h2);
    validateHalf3(h3);
    validateHalf4(h4);

    if (failed) {
        rsSendToClientBlocking(RS_MSG_TEST_FAILED);
    }
    else {
        rsSendToClientBlocking(RS_MSG_TEST_PASSED);
    }
}
