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
 * limitations under the License
 */

package android.systemui.cts;

import android.app.ActivityManager;
import android.content.pm.PackageManager;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.support.test.InstrumentationRegistry;
import android.test.ActivityInstrumentationTestCase2;
import android.util.Log;

import java.io.FileOutputStream;
import java.io.IOException;

/**
 * Test for light status bar.
 */
public class LightStatusBarTests extends ActivityInstrumentationTestCase2<LightStatusBarActivity> {

    public static final String TAG = "LightStatusBarTests";

    public static final String DUMP_PATH = "/sdcard/lightstatustest.png";

    public LightStatusBarTests() {
        super(LightStatusBarActivity.class);
    }

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        // As the way to access Instrumentation is changed in the new runner, we need to inject it
        // manually into ActivityInstrumentationTestCase2. ActivityInstrumentationTestCase2 will
        // be marked as deprecated and replaced with ActivityTestRule.
        injectInstrumentation(InstrumentationRegistry.getInstrumentation());
    }

    public void testLightStatusBarIcons() throws Throwable {
        PackageManager pm = getInstrumentation().getContext().getPackageManager();
        if (pm.hasSystemFeature(PackageManager.FEATURE_WATCH)
                || pm.hasSystemFeature(PackageManager.FEATURE_TELEVISION)
                || pm.hasSystemFeature(PackageManager.FEATURE_LEANBACK)) {
            // No status bar on TVs and watches.
            return;
        }

        if (!ActivityManager.isHighEndGfx()) {
            // non-highEndGfx devices don't do colored system bars.
            return;
        }

        requestLightStatusBar(Color.RED /* background */);
        Thread.sleep(1000);

        Bitmap bitmap = takeStatusBarScreenshot();
        Stats s = evaluateLightStatusBarBitmap(bitmap, Color.RED /* background */);
        boolean success = false;

        try {
            assertMoreThan("Not enough background pixels", 0.3f,
                    (float) s.backgroundPixels / s.totalPixels(),
                    "Is the status bar background showing correctly (solid red)?");

            assertMoreThan("Not enough pixels colored as in the spec", 0.1f,
                    (float) s.iconPixels / s.foregroundPixels(),
                    "Are the status bar icons colored according to the spec "
                            + "(60% black and 24% black)?");

            assertLessThan("Too many lighter pixels lighter than the background", 0.05f,
                    (float) s.sameHueLightPixels / s.foregroundPixels(),
                    "Are the status bar icons dark?");

            assertLessThan("Too many pixels with a changed hue", 0.05f,
                    (float) s.unexpectedHuePixels / s.foregroundPixels(),
                    "Are the status bar icons color-free?");

            success = true;
        } finally {
            if (!success) {
                Log.e(TAG, "Dumping failed bitmap to " + DUMP_PATH);
                dumpBitmap(bitmap);
            }
        }
    }

    private void assertMoreThan(String what, float expected, float actual, String hint) {
        if (!(actual > expected)) {
            fail(what + ": expected more than " + expected * 100 + "%, but only got " + actual * 100
                    + "%; " + hint);
        }
    }

    private void assertLessThan(String what, float expected, float actual, String hint) {
        if (!(actual < expected)) {
            fail(what + ": expected less than " + expected * 100 + "%, but got " + actual * 100
                    + "%; " + hint);
        }
    }

    private void requestLightStatusBar(final int background) throws Throwable {
        final LightStatusBarActivity activity = getActivity();
        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
                activity.getWindow().setStatusBarColor(background);
                activity.setLightStatusBar(true);
            }
        });
    }

    private static class Stats {
        int backgroundPixels;
        int iconPixels;
        int sameHueDarkPixels;
        int sameHueLightPixels;
        int unexpectedHuePixels;

        int totalPixels() {
            return backgroundPixels + iconPixels + sameHueDarkPixels
                    + sameHueLightPixels + unexpectedHuePixels;
        }

        int foregroundPixels() {
            return iconPixels + sameHueDarkPixels
                    + sameHueLightPixels + unexpectedHuePixels;
        }

        @Override
        public String toString() {
            return String.format("{bg=%d, ic=%d, dark=%d, light=%d, bad=%d}",
                    backgroundPixels, iconPixels, sameHueDarkPixels, sameHueLightPixels,
                    unexpectedHuePixels);
        }
    }

    private Stats evaluateLightStatusBarBitmap(Bitmap bitmap, int background) {
        int iconColor = 0x99000000;
        int iconPartialColor = 0x3d000000;

        int mixedIconColor = mixSrcOver(background, iconColor);
        int mixedIconPartialColor = mixSrcOver(background, iconPartialColor);

        int[] pixels = new int[bitmap.getHeight() * bitmap.getWidth()];
        bitmap.getPixels(pixels, 0, bitmap.getWidth(), 0, 0, bitmap.getWidth(), bitmap.getHeight());

        Stats s = new Stats();
        float eps = 0.005f;

        for (int c : pixels) {
            if (c == background) {
                s.backgroundPixels++;
                continue;
            }

            // What we expect the icons to be colored according to the spec.
            if (c == mixedIconColor || c == mixedIconPartialColor) {
                s.iconPixels++;
                continue;
            }

            // Due to anti-aliasing, there will be deviations from the ideal icon color, but it
            // should still be mostly the same hue.
            float hueDiff = Math.abs(ColorUtils.hue(background) - ColorUtils.hue(c));
            if (hueDiff < eps || hueDiff > 1 - eps) {
                // .. it shouldn't be lighter than the original background though.
                if (ColorUtils.brightness(c) > ColorUtils.brightness(background)) {
                    s.sameHueLightPixels++;
                } else {
                    s.sameHueDarkPixels++;
                }
                continue;
            }

            s.unexpectedHuePixels++;
        }

        return s;
    }

    private void dumpBitmap(Bitmap bitmap) {
        FileOutputStream fileStream = null;
        try {
            fileStream = new FileOutputStream(DUMP_PATH);
            bitmap.compress(Bitmap.CompressFormat.PNG, 85, fileStream);
            fileStream.flush();
        } catch (Exception e) {
            Log.e(TAG, "Dumping bitmap failed.", e);
        } finally {
            if (fileStream != null) {
                try {
                    fileStream.close();
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
        }
    }

    private int mixSrcOver(int background, int foreground) {
        int bgAlpha = Color.alpha(background);
        int bgRed = Color.red(background);
        int bgGreen = Color.green(background);
        int bgBlue = Color.blue(background);

        int fgAlpha = Color.alpha(foreground);
        int fgRed = Color.red(foreground);
        int fgGreen = Color.green(foreground);
        int fgBlue = Color.blue(foreground);

        return Color.argb(fgAlpha + (255 - fgAlpha) * bgAlpha / 255,
                    fgRed + (255 - fgAlpha) * bgRed / 255,
                    fgGreen + (255 - fgAlpha) * bgGreen / 255,
                    fgBlue + (255 - fgAlpha) * bgBlue / 255);
    }

    private Bitmap takeStatusBarScreenshot() {
        Bitmap fullBitmap = getInstrumentation().getUiAutomation().takeScreenshot();
        return Bitmap.createBitmap(fullBitmap, 0, 0,
                getActivity().getWidth(), getActivity().getTop());
    }
}
