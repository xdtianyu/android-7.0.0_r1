/*
 * Copyright (C) 2009 The Android Open Source Project
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

import android.os.Build;
import android.platform.test.annotations.RestrictedBuildTest;
import android.util.Log;

import java.util.Arrays;
import java.util.HashSet;
import java.util.Set;

import junit.framework.TestCase;

public class BuildVersionTest extends TestCase {

    private static final String LOG_TAG = "BuildVersionTest";
    private static final Set<String> EXPECTED_RELEASES =
            new HashSet<String>(Arrays.asList("7.0"));
    private static final int EXPECTED_SDK = 24;
    private static final String EXPECTED_BUILD_VARIANT = "user";
    private static final String EXPECTED_TAG = "release-keys";

    @SuppressWarnings("deprecation")
    @RestrictedBuildTest
    public void testReleaseVersion() {
        // Applications may rely on the exact release version
        assertAnyOf("BUILD.VERSION.RELEASE", Build.VERSION.RELEASE, EXPECTED_RELEASES);
        assertEquals("Build.VERSION.SDK", "" + EXPECTED_SDK, Build.VERSION.SDK);
        assertEquals("Build.VERSION.SDK_INT", EXPECTED_SDK, Build.VERSION.SDK_INT);
    }

    public void testIncremental() {
        assertNotEmpty(Build.VERSION.INCREMENTAL);
    }

    /**
     * Verifies {@link Build#FINGERPRINT} follows expected format:
     * <p/>
     * <code>
     * (BRAND)/(PRODUCT)/(DEVICE):(VERSION.RELEASE)/(BUILD_ID)/
     * (BUILD_NUMBER):(BUILD_VARIANT)/(TAGS)
     * </code>
     */
    @RestrictedBuildTest
    public void testBuildFingerprint() {
        final String fingerprint = Build.FINGERPRINT;
        Log.i(LOG_TAG, String.format("Testing fingerprint %s", fingerprint));

        assertEquals("Build fingerprint must not include whitespace", -1,
                fingerprint.indexOf(' '));
        final String[] fingerprintSegs = fingerprint.split("/");
        assertEquals("Build fingerprint does not match expected format", 6, fingerprintSegs.length);
        assertEquals(Build.BRAND, fingerprintSegs[0]);
        assertEquals(Build.PRODUCT, fingerprintSegs[1]);

        String[] devicePlatform = fingerprintSegs[2].split(":");
        assertEquals(2, devicePlatform.length);
        assertEquals(Build.DEVICE, devicePlatform[0]);
        assertEquals(Build.VERSION.RELEASE, devicePlatform[1]);

        assertEquals(Build.ID, fingerprintSegs[3]);

        assertTrue(fingerprintSegs[4].contains(":"));
        String[] buildNumberVariant = fingerprintSegs[4].split(":");
        String buildVariant = buildNumberVariant[1];
        assertEquals("Variant", EXPECTED_BUILD_VARIANT, buildVariant);
        assertEquals("Tag", EXPECTED_TAG, fingerprintSegs[5]);
    }

    private void assertNotEmpty(String value) {
        assertNotNull(value);
        assertFalse(value.isEmpty());
    }

    /** Assert that {@code actualValue} is equals to one of {@code permittedValues}. */
    private void assertAnyOf(String label, String actualValue, Set<String> permittedValues) {
        if (!permittedValues.contains(actualValue)) {
             fail("For: " + label + ", the value: " + actualValue +
                     ", should be one of: " + permittedValues);
        }
    }
}
