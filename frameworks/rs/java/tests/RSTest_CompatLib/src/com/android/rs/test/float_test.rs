#include "shared.rsh"

float floatVal;
double val;
long valLong;

double __attribute__((kernel)) foo(float a) {
    return a + val + floatVal;
}

double __attribute__((kernel)) goo(double a) {
    return a + valLong;
}

