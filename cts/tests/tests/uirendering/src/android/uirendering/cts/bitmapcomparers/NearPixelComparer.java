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
 * Checks to see that a pixel at a given location is the same as the corresponding pixel. If the
 * pixel is different, it checks the pixels around it to see if it is the same.
 */
public class NearPixelComparer extends BitmapComparer {
    private static final String TAG = "NearPixelComparer";
    private static final int THRESHOLD = 10;
    private static final int NEAR_PIXEL_RADIUS = 1;

    @Override
    public boolean verifySame(int[] ideal, int[] given, int offset, int stride, int width,
            int height) {
        for (int y = 0 ; y < height ; y++) {
            for (int x = 0 ; x < width ; x++) {
                boolean success = false;

                // we need to check the surrounding pixels
                for (int dx = -NEAR_PIXEL_RADIUS ; dx <= NEAR_PIXEL_RADIUS ; dx++) {
                    for (int dy = -NEAR_PIXEL_RADIUS ; dy <= NEAR_PIXEL_RADIUS ; dy++) {
                        // need to be sure we don't hit pixels that aren't there
                        if (x + dx >= width || x + dx < 0 || y + dy >= height || y + dy < 0) {
                            continue;
                        }
                        int index = indexFromXAndY(x + dx, y + dy, stride, offset);
                        if (!pixelsAreSame(ideal[index], given[index])) {
                            success = true;
                            break;
                        }
                    }
                }
                if (!success) {
                    Log.d(TAG, "Failure at pixel (" + x + "," + y + ")");
                    return false;
                }
            }
        }
        return true;
    }

    private boolean pixelsAreSame(int ideal, int given) {
        int error = Math.abs(Color.red(ideal) - Color.red(given));
        error += Math.abs(Color.green(ideal) - Color.green(given));
        error += Math.abs(Color.blue(ideal) - Color.blue(given));
        return (error < THRESHOLD);
    }
}
