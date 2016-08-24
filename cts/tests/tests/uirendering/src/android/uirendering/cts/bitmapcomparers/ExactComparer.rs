#pragma version(1)
#pragma rs java_package_name(android.uirendering.cts)

int WIDTH;
int OFFSET;

rs_allocation ideal;
rs_allocation given;

// This method does a simple comparison of all the values in the given and ideal allocations.
// If any of the pixels are off, then the test will fail.
void exactCompare(const int32_t *v_in, float *v_out){
    int y = v_in[0];
    v_out[0] = 0;

    for(int i = 0 ; i < WIDTH ; i ++){
        uchar4 idealPixel = rsGetElementAt_uchar4(ideal, i + OFFSET, y);
        uchar4 givenPixel = rsGetElementAt_uchar4(given, i + OFFSET, y);
        uchar4 diff = idealPixel - givenPixel;
        int totalDiff = diff.x + diff.y + diff.z;
        if(totalDiff != 0){
            v_out[0] ++;
        }
    }
}
