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
package com.android.compatibility.common.deviceinfo;

import android.util.Log;

import com.android.compatibility.common.util.DeviceInfoStore;

import java.io.Closeable;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.Formatter;

/**
 * Library device info collector.
 */
public final class LibraryDeviceInfo extends DeviceInfo {

    private static final String TAG = "LibraryDeviceInfo";
    private static final int BUFFER_SIZE_BYTES = 4096;

    @Override
    protected void collectDeviceInfo(DeviceInfoStore store) throws Exception {
        collectSystemLibs(store);
        collectVendorLibs(store);
        collectFrameworkJars(store);
    }

    private void collectSystemLibs(DeviceInfoStore store) throws Exception {
        store.startArray("lib");
        collectFileDetails(store, "/system/lib", ".so");
        store.endArray();
    }

    private void collectVendorLibs(DeviceInfoStore store) throws Exception {
        store.startArray("vendor_lib");
        collectFileDetails(store, "/system/vendor/lib", ".so");
        store.endArray();
    }

    private void collectFrameworkJars(DeviceInfoStore store) throws Exception {
        store.startArray("framework_jar");
        collectFileDetails(store, "/system/framework", ".jar");
        store.endArray();
    }

    private void collectFileDetails(DeviceInfoStore store, String path, String suffix)
            throws Exception {
        File dir = new File(path);
        for (File file : dir.listFiles()) {
            String name = file.getName();
            if (file.isFile() && name.endsWith(suffix)) {
                String sha1 = "unknown";
                try {
                    sha1 = getSha1sum(file);
                } catch (IOException e) {
                    Log.e(TAG, "Failed to hash " + file + ": ", e);
                }
                store.startGroup();
                store.addResult("name", name);
                store.addResult("sha1", sha1);
                store.endGroup();
            }
        }
    }

    private static String getSha1sum(File file) throws IOException {
        InputStream in = null;
        try {
            in = new FileInputStream(file);
            return sha1(in);
        } finally {
            close(in);
        }
    }

    private static void close(Closeable s) throws IOException {
        if (s == null) {
            return;
        }
        s.close();
    }

    /**
     * @return the SHA-1 digest of input as a hex string
     */
    public static String sha1(InputStream input) throws IOException {
        try {
            return toHexString(digest(input, "sha1"));
        } catch (NoSuchAlgorithmException e) {
            throw new RuntimeException(e);
        }
    }

    private static byte[] digest(InputStream in, String algorithm)
        throws NoSuchAlgorithmException, IOException {
        MessageDigest digest = MessageDigest.getInstance(algorithm);
        byte[] buffer = new byte[BUFFER_SIZE_BYTES];
        while (true) {
            int read = in.read(buffer);
            if (read < 0) {
                break;
            }
            digest.update(buffer, 0, read);
        }
        return digest.digest();
    }

    private static String toHexString(byte[] buffer) {
        Formatter formatter = new Formatter();
        try {
            for (byte b : buffer) {
                formatter.format("%02X", b);
            }
            return formatter.toString();
        } finally {
            formatter.close();
        }
    }
}
