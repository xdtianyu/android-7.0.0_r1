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

package android.security.cts;

import android.test.AndroidTestCase;
import junit.framework.TestCase;

import android.app.ActivityManager;
import android.content.Context;
import android.os.SystemProperties;
import android.util.Log;
import java.io.BufferedReader;
import java.io.FileReader;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class EncryptionTest extends AndroidTestCase {

    static {
        System.loadLibrary("ctssecurity_jni");
    }

    private static final int min_api_level = 23;

    private static final String TAG = "EncryptionTest";

    private static final String crypto = "/proc/crypto";

    private static native boolean deviceIsEncrypted();

    private static native boolean cpuHasAes();

    private static native boolean cpuHasNeon();

    private static native boolean neonIsEnabled();

    private static native boolean aesIsFast();

    private boolean hasKernelCrypto(String driver) throws Exception {
        BufferedReader br = new BufferedReader(new FileReader(crypto));
        Pattern p = Pattern.compile("driver\\s*:\\s*" + driver);

        try {
            String line;
            while ((line = br.readLine()) != null) {
                if (p.matcher(line).matches()) {
                    Log.i(TAG, crypto + " has " + driver + " (" + line + ")");
                    return true;
                }
            }
       } finally {
           br.close();
       }

       return false;
    }

    private boolean hasLowRAM() {
        ActivityManager activityManager =
            (ActivityManager) getContext().getSystemService(Context.ACTIVITY_SERVICE);

        return activityManager.isLowRamDevice();
    }

    private boolean isRequired() {
        int first_api_level =
            SystemProperties.getInt("ro.product.first_api_level", 0);

        // Optional before min_api_level or if the device has low RAM
        if (first_api_level > 0 && first_api_level < min_api_level) {
            return false;
        } else {
            return !hasLowRAM();
        }
    }

    public void testConfig() throws Exception {
        if (!isRequired()) {
            return;
        }

        if (cpuHasAes()) {
            // If CPU has AES CE, it must be enabled in kernel
            assertTrue(crypto + " is missing xts-aes-ce or xts-aes-aesni",
                hasKernelCrypto("xts-aes-ce") ||
                hasKernelCrypto("xts-aes-aesni"));
        } else if (cpuHasNeon()) {
            // Otherwise, if CPU has NEON, it must be enabled
            assertTrue(crypto + " is missing xts-aes-neon (or xts-aes-neonbs)",
                hasKernelCrypto("xts-aes-neon") ||
                hasKernelCrypto("xts-aes-neonbs") ||
                hasKernelCrypto("aes-asm")); // Not recommended alone
        }

        if (cpuHasNeon()) {
            assertTrue("libcrypto must have NEON", neonIsEnabled());
        }
    }

    public void testEncryption() throws Exception {
        if (!isRequired() || deviceIsEncrypted()) {
            return;
        }

        // Required if performance is sufficient
        assertFalse("Device encryption is required", aesIsFast());
    }
}
