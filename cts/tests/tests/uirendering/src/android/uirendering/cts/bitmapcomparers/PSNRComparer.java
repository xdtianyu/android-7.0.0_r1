/*
 * Copyright (C) 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package android.uirendering.cts.bitmapcomparers;

import android.graphics.Color;
import android.util.Log;

/**
 * Uses the Peak Signal-to-Noise Ratio approach to determine if two images are considered the same.
 */
public class PSNRComparer extends BitmapComparer {
    private static final String TAG = "PSNR";
    private final float MAX = 255;
    private final int REGION_SIZE = 10;

    private float mThreshold;

    /**
     * @param threshold the PSNR necessary to pass the test, if the calculated PSNR is below this
     *                  value, then the test will fail.
     */
    public PSNRComparer(float threshold) {
        mThreshold = threshold;
    }

    @Override
    public boolean verifySame(int[] ideal, int[] given, int offset, int stride, int width,
            int height) {
        float MSE = 0f;
        int interestingRegions = 0;
        for (int y = 0 ; y < height ; y += REGION_SIZE) {
            for (int x = 0 ; x < width ; x += REGION_SIZE) {
                int index = indexFromXAndY(x, y, stride, offset);
                if (inspectRegion(ideal, index)) {
                    interestingRegions++;
                }
            }
        }

        if (interestingRegions == 0) {
            return true;
        }

        for (int y = 0 ; y < height ; y += REGION_SIZE) {
            for (int x = 0 ; x < width ; x += REGION_SIZE) {
                int index = indexFromXAndY(x, y, stride, offset);
                if (ideal[index] == given[index]) {
                    continue;
                }
                MSE += (Color.red(ideal[index]) - Color.red(given[index])) *
                        (Color.red(ideal[index]) - Color.red(given[index]));
                MSE += (Color.blue(ideal[index]) - Color.blue(given[index])) *
                        (Color.blue(ideal[index]) - Color.blue(given[index]));
                MSE += (Color.green(ideal[index]) - Color.green(given[index])) *
                        (Color.green(ideal[index]) - Color.green(given[index]));
            }
        }
        MSE /= (interestingRegions * REGION_SIZE * 3);

        float fraction = (MAX * MAX) / MSE;
        fraction = (float) Math.log(fraction);
        fraction *= 10;

        Log.d(TAG, "PSNR : " + fraction);

        return (fraction > mThreshold);
    }

    private boolean inspectRegion(int[] ideal, int index) {
        int regionColor = ideal[index];
        for (int i = 0 ; i < REGION_SIZE ; i++) {
            if (regionColor != ideal[index + i]) {
                return true;
            }
        }
        return false;
    }
}
