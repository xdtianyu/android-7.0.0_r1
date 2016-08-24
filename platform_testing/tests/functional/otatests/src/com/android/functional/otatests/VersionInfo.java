package com.android.functional.otatests;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStreamReader;

public class VersionInfo {
    private final String mBuildId;
    private final String mBootloaderVersion;
    private final String mBasebandVersion;

    private VersionInfo(String buildId, String bootVersion, String radioVersion) {
        mBuildId = buildId;
        mBootloaderVersion = bootVersion;
        mBasebandVersion = radioVersion;
    }

    public String getBuildId() {
        return mBuildId;
    }

    public String getBootloaderVersion() {
        return mBootloaderVersion;
    }

    public String getBasebandVersion() {
        return mBasebandVersion;
    }

    public static VersionInfo parseFromFile(String fileName) throws IOException {
        BufferedReader r = new BufferedReader(
                new InputStreamReader(new FileInputStream(new File(fileName))));
        try {
            return new VersionInfo(
                    denull(r.readLine()),
                    denull(r.readLine()),
                    denull(r.readLine()));
        } finally {
            r.close();
        }
    }

    private static String denull(String s) {
        return  s == null || s.equals("null") ? "" : s;
    }
}
