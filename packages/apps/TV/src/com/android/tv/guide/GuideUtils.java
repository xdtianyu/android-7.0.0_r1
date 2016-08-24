/*
 * Copyright (C) 2015 The Android Open Source Project
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

package com.android.tv.guide;

import java.util.concurrent.TimeUnit;

public class GuideUtils {
    private static int sWidthPerHour = 0;

    /**
     * Sets the width in pixels that corresponds to an hour in program guide.
     * Assume that this is called from main thread only, so, no synchronization.
     */
    public static void setWidthPerHour(int widthPerHour) {
        sWidthPerHour = widthPerHour;
    }

    /**
     * Gets the number of pixels in program guide table that corresponds to the given milliseconds.
     */
    public static int convertMillisToPixel(long millis) {
        return (int) (millis * sWidthPerHour / TimeUnit.HOURS.toMillis(1));
    }

    /**
     * Gets the number of pixels in program guide table that corresponds to the given range.
     */
    public static int convertMillisToPixel(long startMillis, long endMillis) {
        // Convert to pixels first to avoid accumulation of rounding errors.
        return GuideUtils.convertMillisToPixel(endMillis)
                - GuideUtils.convertMillisToPixel(startMillis);
    }

    /**
     * Gets the time in millis that corresponds to the given pixels in the program guide.
     */
    public static long convertPixelToMillis(int pixel) {
        return pixel * TimeUnit.HOURS.toMillis(1) / sWidthPerHour;
    }

    private GuideUtils() { }
}
