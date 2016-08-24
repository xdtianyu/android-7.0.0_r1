package com.android.functional.otatests;

import android.test.InstrumentationTestCase;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;

public class VersionCheckingTest extends InstrumentationTestCase {

    protected static final String OLD_VERSION = "/sdcard/otatest/version.old";
    protected static final String NEW_VERSION = "/sdcard/otatest/version.new";
    protected static final String KEY_BUILD_ID = "ro.build.version.incremental";
    protected static final String KEY_BOOTLOADER = "ro.bootloader";
    protected static final String KEY_BASEBAND = "ro.build.expect.baseband";
    protected static final String KEY_BASEBAND_GSM = "gsm.version.baseband";

    protected VersionInfo mOldVersion;
    protected VersionInfo mNewVersion;

    @Override
    public void setUp() throws Exception {
        try {
            mOldVersion = VersionInfo.parseFromFile(OLD_VERSION);
            mNewVersion = VersionInfo.parseFromFile(NEW_VERSION);
        } catch (IOException e) {
            throw new RuntimeException(
                    "Couldn't find version file; was this test run with VersionCachePreparer?", e);
        }
    }

    protected void assertNotUpdated() throws IOException {
        assertEquals(mOldVersion.getBuildId(), getProp(KEY_BUILD_ID));
        assertEquals(mOldVersion.getBootloaderVersion(), getProp(KEY_BOOTLOADER));
        assertTrue(mOldVersion.getBasebandVersion().equals(getProp(KEY_BASEBAND))
                || mOldVersion.getBasebandVersion().equals(getProp(KEY_BASEBAND_GSM)));
    }

    protected void assertUpdated() throws IOException {
        assertEquals(mNewVersion.getBuildId(), getProp(KEY_BUILD_ID));
        assertEquals(mNewVersion.getBootloaderVersion(), getProp(KEY_BOOTLOADER));
        // Due to legacy property names (an old meaning to gsm.version.baseband),
        // the KEY_BASEBAND and KEY_BASEBAND_GSM properties may not match each other.
        // At least one of them will always match the baseband version recorded by
        // NEW_VERSION.
        assertTrue(mNewVersion.getBasebandVersion().equals(getProp(KEY_BASEBAND))
                || mNewVersion.getBasebandVersion().equals(getProp(KEY_BASEBAND_GSM)));
    }

    private String getProp(String key) throws IOException {
        Process p = Runtime.getRuntime().exec("getprop " + key);
        BufferedReader r = new BufferedReader(new InputStreamReader(p.getInputStream()));
        String ret = r.readLine().trim();
        r.close();
        return ret;
    }
}
