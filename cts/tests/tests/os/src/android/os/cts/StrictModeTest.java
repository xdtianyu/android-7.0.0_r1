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

package android.os.cts;

import android.content.pm.PackageManager;
import android.os.StrictMode;
import android.os.SystemClock;
import android.test.AndroidTestCase;
import android.util.Log;

import libcore.io.Streams;

import java.io.ByteArrayOutputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.text.SimpleDateFormat;
import java.util.Date;

/**
 * Tests for {@link StrictMode}
 */
public class StrictModeTest extends AndroidTestCase {
    private static final String TAG = "StrictModeTest";

    private StrictMode.VmPolicy mPolicy;

    @Override
    protected void setUp() {
        mPolicy = StrictMode.getVmPolicy();
    }

    @Override
    protected void tearDown() {
        StrictMode.setVmPolicy(mPolicy);
    }

    public void testCleartextNetwork() throws Exception {
        if (!hasInternetConnection()) {
            Log.i(TAG, "testCleartextNetwork() ignored on device without Internet");
            return;
        }

        StrictMode.setVmPolicy(new StrictMode.VmPolicy.Builder()
                .detectCleartextNetwork()
                .penaltyLog()
                .build());

        final long millis = System.currentTimeMillis();
        final String msg = "Detected cleartext network traffic from UID "
                + android.os.Process.myUid();

        // Insecure connection should be detected
        ((HttpURLConnection) new URL("http://example.com/").openConnection()).getResponseCode();

        // Give system enough time to finish logging
        SystemClock.sleep(5000);
        assertTrue("Expected cleartext to be caught", readLogSince(millis).contains(msg));
    }

    public void testEncryptedNetwork() throws Exception {
        if (!hasInternetConnection()) {
            Log.i(TAG, "testEncryptedNetwork() ignored on device without Internet");
            return;
        }

        StrictMode.setVmPolicy(new StrictMode.VmPolicy.Builder()
                .detectCleartextNetwork()
                .penaltyLog()
                .build());

        final long millis = System.currentTimeMillis();
        final String msg = "Detected cleartext network traffic from UID "
                + android.os.Process.myUid();

        // Secure connection should be ignored
        ((HttpURLConnection) new URL("https://example.com/").openConnection()).getResponseCode();

        // Give system enough time to finish logging
        SystemClock.sleep(5000);
        assertFalse("Expected encrypted to be ignored", readLogSince(millis).contains(msg));
    }

    private String readLogSince(long millis) throws Exception {
        final SimpleDateFormat format = new SimpleDateFormat("MM-dd HH:mm:ss.SSS");
        final Process proc = new ProcessBuilder("logcat", "-t", format.format(new Date(millis)))
                .redirectErrorStream(true).start();

        final ByteArrayOutputStream buf = new ByteArrayOutputStream();
        Streams.copy(proc.getInputStream(), buf);
        final int res = proc.waitFor();

        Log.d(TAG, "Log output was " + buf.size() + " bytes, exit code " + res);
        return new String(buf.toByteArray());
    }

    private boolean hasInternetConnection() {
        final PackageManager pm = getContext().getPackageManager();
        return pm.hasSystemFeature(PackageManager.FEATURE_TELEPHONY)
                || pm.hasSystemFeature(PackageManager.FEATURE_WIFI)
                || pm.hasSystemFeature(PackageManager.FEATURE_ETHERNET);
    }
}
