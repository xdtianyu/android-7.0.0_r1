#pragma version(1)
#pragma rs java_package_name(android.uirendering.cts)

int REGION_SIZE;
int WIDTH;

rs_allocation ideal;
rs_allocation given;

// This method does a threshold comparison of the values
void calcMSE(const int32_t *v_in, float *v_out){
    int y = v_in[0];
    v_out[0] = 0.0f;
    for (int x = 0 ; x < WIDTH ; x++) {
        float4 idealFloats = rsUnpackColor8888(rsGetElementAt_uchar4(ideal, x, y));
        float4 givenFloats = rsUnpackColor8888(rsGetElementAt_uchar4(given, x, y));
        float difference = (idealFloats.r - givenFloats.r) + (idealFloats.g - givenFloats.g) +
              (idealFloats.b - givenFloats.b);
        v_out[0] += (difference * difference);
    }
}
