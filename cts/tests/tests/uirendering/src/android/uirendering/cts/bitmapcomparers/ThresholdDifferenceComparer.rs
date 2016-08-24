#pragma version(1)
#pragma rs java_package_name(android.uirendering.cts)

int WIDTH;
int THRESHOLD;

rs_allocation ideal;
rs_allocation given;

// This method does a threshold comparison of the values
void thresholdCompare(const int32_t *v_in, float *v_out){
    int y = v_in[0];
    v_out[0] = 0;

    for(int i = 0 ; i < WIDTH ; i ++){
        uchar4 idealPixel = rsGetElementAt_uchar4(ideal, i, y);
        uchar4 givenPixel = rsGetElementAt_uchar4(given, i, y);
        float l1 = (idealPixel.x * 0.21f) + (idealPixel.y * 0.72f) + (idealPixel.z * 0.07f);
        float l2 = (givenPixel.x * 0.21f) + (givenPixel.y * 0.72f) + (givenPixel.z * 0.07f);
        float diff = l1 - l2;
        if (fabs(diff) >= THRESHOLD) {
            v_out[0]++;
        }
    }
}
