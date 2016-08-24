#include <stdio.h>

__attribute__((noinline)) float E(float c) {
    return c * 11.0;
}
__attribute__((noinline)) float D(float c) {
    return E(c) / (2.0*E(c+117.0));
}
__attribute__((noinline)) float C(float c) {
    return D(c) / (c*11.11111);
}
__attribute__((noinline)) float B(float c) {
    return C(c-5000.1) * D(c);
}
__attribute__((noinline)) float A(float c) {
    return B(c) / C(c+2.3);
}

int main() {
    float count2 = 0;
    for (int i = 0; i < 100; i++) {
        float count = 0;
        for (float j = 0; j < 10000; ++j) {
            count += A(j);
        }
        for (float k = 0; k < 20000; ++k) {
            count = count / C(k);
        }
        count2 += count;
    }
    printf("%f", count2);
    return 0;
}
