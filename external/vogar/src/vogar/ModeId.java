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

package vogar;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public enum ModeId {
    /** ART (works >= L) */
    DEVICE,
    /** ART (works >= L) */
    HOST,
    /** Local Java */
    JVM,
    /** Device, execution as an Android app with Zygote */
    ACTIVITY,
    /** Device using app_process binary */
    APP_PROCESS;

    // $BOOTCLASSPATH defined by system/core/rootdir/init.rc
    private static final String[] DEVICE_JARS = new String[] {
            "core-libart",
            "core-oj",
            "conscrypt",
            "okhttp",
            "core-junit",
            "bouncycastle",
            "ext",
            "framework",
            "telephony-common",
            "mms-common",
            "framework",
            "android.policy",
            "services",
            "apache-xml"};

    private static final String[] HOST_JARS = new String[] {
            "core-libart-hostdex",
            "core-oj-hostdex",
            "conscrypt-hostdex",
            "okhttp-hostdex",
            "bouncycastle-hostdex",
            "apache-xml-hostdex"
    };

    public boolean acceptsVmArgs() {
        return this != ACTIVITY;
    }

    /**
     * Returns {@code true} if execution happens on the local machine. e.g. host-mode android or a
     * JVM.
     */
    public boolean isLocal() {
        return isHost() || this == ModeId.JVM;
    }

    /** Returns {@code true} if execution takes place with a host-mode Android runtime */
    public boolean isHost() {
        return this == HOST;
    }

    /** Returns {@code true} if execution takes place with a device-mode Android runtime */
    public boolean isDevice() {
        return this == ModeId.DEVICE || this == ModeId.APP_PROCESS;
    }

    public boolean requiresAndroidSdk() {
        return this != JVM;
    }

    public boolean supportsVariant(Variant variant) {
        return (variant == Variant.X32)
                || ((this == HOST || this == DEVICE) && (variant == Variant.X64));
    }

    /** The default command to use for the mode unless overridden by --vm-command */
    public String defaultVmCommand(Variant variant) {
        if (!supportsVariant(variant)) {
            throw new AssertionError("Unsupported variant: " + variant + " for " + this);
        }
        switch (this) {
            case DEVICE:
            case HOST:
                if (variant == Variant.X32) {
                    return "dalvikvm32";
                } else {
                    return "dalvikvm64";
                }

            case JVM:
                return "java";
            case APP_PROCESS:
                return "app_process";
            case ACTIVITY:
                return null;
            default:
                throw new IllegalArgumentException("Unknown mode: " + this);
        }
    }

    /**
     * Return the names of jars required to compile in this mode when android.jar is not being used.
     * Also used to generated the classpath in HOST* and DEVICE* modes.
     */
    public String[] getJarNames() {
        List<String> jarNames = new ArrayList<String>();
        switch (this) {
            case ACTIVITY:
            case APP_PROCESS:
            case DEVICE:
                jarNames.addAll(Arrays.asList(DEVICE_JARS));
                break;
            case HOST:
                jarNames.addAll(Arrays.asList(HOST_JARS));
                break;
            default:
                throw new IllegalArgumentException("Unsupported mode: " + this);
        }
        return jarNames.toArray(new String[jarNames.size()]);
    }
}
