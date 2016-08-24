#pragma version(1)
#pragma rs java_package_name(android.uirendering.cts)

int WIDTH;
int HEIGHT;

rs_allocation ideal;
rs_allocation given;

static float getPixelWeight(uchar4 pixel) {
    const float MAX_VALUE_COLOR = 255;
    const float RED_WEIGHT = 0.21f / MAX_VALUE_COLOR;
    const float GREEN_WEIGHT = 0.72f / MAX_VALUE_COLOR;
    const float BLUE_WEIGHT = 0.07f / MAX_VALUE_COLOR;
    return (pixel.r * RED_WEIGHT) + (pixel.g * GREEN_WEIGHT) + (pixel.b * BLUE_WEIGHT);
}

// Calculates SSIM of a row of pixels
void calcSSIM(const int32_t *v_in, float *v_out) {
    // TODO Test values for these constants
    const float C1 = 0.0000064516;
    const float C2 = 0.0000580644;

    int y = v_in[0];
    v_out[0] = 0;

    float meanIdeal = 0;
    float meanGiven = 0;

    for (int i = 0 ; i < WIDTH ; i++) {
        uchar4 idealPixel = rsGetElementAt_uchar4(ideal, i, y);
        uchar4 givenPixel = rsGetElementAt_uchar4(given, i, y);
        meanIdeal += getPixelWeight(idealPixel);
        meanGiven += getPixelWeight(givenPixel);
    }

    meanIdeal /= WIDTH;
    meanGiven /= WIDTH;

    float varIdeal = 0;
    float varGiven = 0;
    float varBoth = 0;

    for (int i = 0 ; i < WIDTH ; i++) {
        uchar4 idealPixel = rsGetElementAt_uchar4(ideal, i, y);
        uchar4 givenPixel = rsGetElementAt_uchar4(given, i, y);
        float idealWeight = getPixelWeight(idealPixel);
        float givenWeight = getPixelWeight(givenPixel);
        idealWeight -= meanIdeal;
        givenWeight -= meanGiven;
        varIdeal +=  idealWeight * idealWeight;
        varGiven += givenWeight * givenWeight;
        varBoth += idealWeight * givenWeight;
    }

    varIdeal /= WIDTH - 1;
    varGiven /= WIDTH - 1;
    varBoth /= WIDTH - 1;

    float SSIM = ((2 * meanIdeal * meanGiven) + C1) * ((2 * varBoth) + C2);
    float denom = ((meanIdeal * meanIdeal) + (meanGiven * meanGiven) + C1)
                    * (varIdeal + varGiven + C2);
    SSIM /= denom;

    v_out[0] = SSIM;
}
