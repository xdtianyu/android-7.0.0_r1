/*
 * Copyright (C) 2016 The Android Open Source Project
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

package android.jni.cts;

import android.content.Context;
import android.content.pm.PackageManager;
import android.support.test.InstrumentationRegistry;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.File;
import java.io.FileReader;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

class LinkerNamespacesHelper {
    private final static String VENDOR_CONFIG_FILE = "/vendor/etc/public.libraries.txt";
    private final static String[] PUBLIC_SYSTEM_LIBRARIES = {
        "libandroid.so",
        "libcamera2ndk.so",
        "libc.so",
        "libdl.so",
        "libEGL.so",
        "libGLESv1_CM.so",
        "libGLESv2.so",
        "libGLESv3.so",
        "libicui18n.so",
        "libicuuc.so",
        "libjnigraphics.so",
        "liblog.so",
        "libmediandk.so",
        "libm.so",
        "libOpenMAXAL.so",
        "libOpenSLES.so",
        "libRS.so",
        "libstdc++.so",
        "libvulkan.so",
        "libz.so"
    };

    private final static String WEBVIEW_PLAT_SUPPORT_LIB = "libwebviewchromium_plat_support.so";

    public static String runAccessibilityTest() throws IOException {
        List<String> vendorLibs = new ArrayList<>();
        File file = new File(VENDOR_CONFIG_FILE);
        if (file.exists()) {
            try (BufferedReader br = new BufferedReader(new FileReader(file))) {
                String line;
                while ((line = br.readLine()) != null) {
                    line = line.trim();
                    if (line.isEmpty() || line.startsWith("#")) {
                        continue;
                    }
                    vendorLibs.add(line);
                }
            }
        }

        List<String> systemLibs = new ArrayList<>();
        Collections.addAll(systemLibs, PUBLIC_SYSTEM_LIBRARIES);

        if (InstrumentationRegistry.getContext().getPackageManager().
                hasSystemFeature(PackageManager.FEATURE_WEBVIEW)) {
            systemLibs.add(WEBVIEW_PLAT_SUPPORT_LIB);
        }

        return runAccessibilityTestImpl(systemLibs.toArray(new String[systemLibs.size()]),
                                        vendorLibs.toArray(new String[vendorLibs.size()]));
    }

    private static native String runAccessibilityTestImpl(String[] publicSystemLibs,
                                                          String[] publicVendorLibs);
}
