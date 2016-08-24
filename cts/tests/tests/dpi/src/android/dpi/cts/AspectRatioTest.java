/*
 * Copyright (C) 2011 The Android Open Source Project
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

package android.dpi.cts;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.content.pm.PackageManager;
import android.test.ActivityInstrumentationTestCase2;
import android.util.DisplayMetrics;
import android.view.Display;
import android.view.WindowManager;

import java.util.ArrayList;
import java.util.List;

public class AspectRatioTest extends ActivityInstrumentationTestCase2<OrientationActivity> {

    private static final int[] ORIENTATIONS = new int[] {
        ActivityInfo.SCREEN_ORIENTATION_PORTRAIT,
        ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE,
    };

    public AspectRatioTest() {
        super(OrientationActivity.class);
    }

    /**
     * Get the full screen size directly (including system bar) to calculate
     * aspect ratio. With this, the screen orientation doesn't affect the aspect
     * ratio value anymore. Test that the aspect ratio is within the range.
     */
    public void testAspectRatio() throws Exception {
        double aspectRatio = getRealAspectRatio(getActivity());
        if (aspectRatio >= 1.333 && aspectRatio <= 1.86) {
            return;
        }
        if (getActivity().getPackageManager().hasSystemFeature(PackageManager.FEATURE_WATCH)) {
            // Watch allows for a different set of aspect ratios.
            if (aspectRatio >= 1.0 && aspectRatio <= 1.86) {
                return;
            }
        }
        fail("Aspect ratio was not between 1.333 and 1.86: " + aspectRatio);
    }

    private double getRealAspectRatio(Context context) {
        WindowManager windowManager =
                (WindowManager) context.getSystemService(Context.WINDOW_SERVICE);
        Display display = windowManager.getDefaultDisplay();
        DisplayMetrics metrics = new DisplayMetrics();
        display.getRealMetrics(metrics);

        int max = Math.max(metrics.widthPixels, metrics.heightPixels);
        int min = Math.min(metrics.widthPixels, metrics.heightPixels);
        return (double) max / min;
    }
}
