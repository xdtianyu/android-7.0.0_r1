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

package vogar.target;

import java.io.File;
import java.lang.reflect.Field;
import java.lang.reflect.Modifier;
import java.net.Authenticator;
import java.net.CookieHandler;
import java.net.ResponseCache;
import java.text.DateFormat;
import java.util.Locale;
import java.util.HashMap;
import java.util.TimeZone;
import java.util.logging.ConsoleHandler;
import java.util.logging.LogManager;
import java.util.logging.Logger;
import java.util.prefs.BackingStoreException;
import java.util.prefs.Preferences;
import javax.net.ssl.HostnameVerifier;
import javax.net.ssl.HttpsURLConnection;
import javax.net.ssl.SSLSocketFactory;
import vogar.util.IoUtils;

/**
 * This class resets the VM to a relatively pristine state. Useful to defend
 * against tests that muck with system properties and other global state.
 */
public final class TestEnvironment {

    private final HostnameVerifier defaultHostnameVerifier;
    private final SSLSocketFactory defaultSSLSocketFactory;

    /** The DateFormat.is24Hour field. Not present on older versions of Android or the RI. */
    private static final Field dateFormatIs24HourField;
    static {
        Field f;
        try {
            Class<?> dateFormatClass = Class.forName("java.text.DateFormat");
            f = dateFormatClass.getDeclaredField("is24Hour");
        } catch (ClassNotFoundException | NoSuchFieldException e) {
            f = null;
        }
        dateFormatIs24HourField = f;
    }
    private final Boolean defaultDateFormatIs24Hour;

    private static final String JAVA_RUNTIME_VERSION = System.getProperty("java.runtime.version"); 
    private static final String JAVA_VM_INFO = System.getProperty("java.vm.info"); 
    private static final String JAVA_VM_VERSION = System.getProperty("java.vm.version"); 
    private static final String JAVA_VM_VENDOR = System.getProperty("java.vm.vendor"); 
    private static final String JAVA_VM_NAME = System.getProperty("java.vm.name");

    private final String tmpDir;

    public TestEnvironment() {
        this.tmpDir = System.getProperty("java.io.tmpdir");
        if (tmpDir == null || tmpDir.length() == 0) {
            throw new AssertionError("tmpDir is null or empty: " + tmpDir);
        }
        System.setProperties(null); // Reset.

        // On android, behaviour around clearing "java.io.tmpdir" is inconsistent.
        // Some release set it to "null" and others set it to "/tmp" both values are
        // wrong for normal apps (mode=activity), where the value that the framework
        // sets must be used. We unconditionally restore that value here. This code
        // should be correct on the host and on the jvm too, since tmpdir is assumed
        // to be immutable.
        System.setProperty("java.io.tmpdir", tmpDir);

        String userHome = System.getProperty("user.home");
        String userDir = System.getProperty("user.dir");
        if (userHome == null || userDir == null) {
            throw new NullPointerException("user.home=" + userHome + ", user.dir=" + userDir);
        }

        defaultHostnameVerifier = HttpsURLConnection.getDefaultHostnameVerifier();
        defaultSSLSocketFactory = HttpsURLConnection.getDefaultSSLSocketFactory();
        defaultDateFormatIs24Hour = hasDateFormatIs24Hour() ? getDateFormatIs24Hour() : null;

        disableSecurity();
    }

    private String createTempDirectory(String subDirName) {
        String dirName = tmpDir + "/" + subDirName;
        IoUtils.safeMkdirs(new File(dirName));
        return dirName;
    }

    public void reset() {
        // Reset system properties.
        System.setProperties(null);

        // On android, behaviour around clearing "java.io.tmpdir" is inconsistent.
        // Some release set it to "null" and others set it to "/tmp" both values are
        // wrong for normal apps (mode=activity), where the value that the framework
        // sets must be used. We unconditionally restore that value here. This code
        // should be correct on the host and on the jvm too, since tmpdir is assumed
        // to be immutable.
        System.setProperty("java.io.tmpdir", tmpDir);

        // Require writable java.home and user.dir directories for preferences
        if ("Dalvik".equals(System.getProperty("java.vm.name"))) {
            setPropertyIfNull("java.home", createTempDirectory("java.home"));
            setPropertyIfNull("dexmaker.dexcache", createTempDirectory("dexmaker.dexcache"));
        } else {
            // The mode --jvm has these properties writable.
            if (JAVA_RUNTIME_VERSION != null) {
                System.setProperty("java.runtime.version", JAVA_RUNTIME_VERSION);
            }
            if (JAVA_VM_INFO != null) {
                System.setProperty("java.vm.info", JAVA_VM_INFO);
            }
            if (JAVA_VM_VERSION != null) {
                System.setProperty("java.vm.version", JAVA_VM_VERSION);
            }
            if (JAVA_VM_VENDOR != null) {
                System.setProperty("java.vm.vendor", JAVA_VM_VENDOR);
            }
            if (JAVA_VM_NAME != null) {
                System.setProperty("java.vm.name", JAVA_VM_NAME);
            }
        }
        String userHome = System.getProperty("user.home");
        if (userHome.length() == 0) {
            userHome = tmpDir + "/user.home";
            IoUtils.safeMkdirs(new File(userHome));
            System.setProperty("user.home", userHome);
        }

        // Localization
        Locale.setDefault(Locale.US);
        TimeZone.setDefault(TimeZone.getTimeZone("America/Los_Angeles"));
        if (hasDateFormatIs24Hour()) {
            setDateFormatIs24Hour(defaultDateFormatIs24Hour);
        }

        // Preferences
        // Temporarily silence the java.util.prefs logger, which otherwise emits
        // an unactionable warning. See RI bug 4751540.
        Logger loggerToMute = Logger.getLogger("java.util.prefs");
        boolean usedParentHandlers = loggerToMute.getUseParentHandlers();
        loggerToMute.setUseParentHandlers(false);
        try {
            // resetPreferences(Preferences.systemRoot());
            resetPreferences(Preferences.userRoot());
        } finally {
            loggerToMute.setUseParentHandlers(usedParentHandlers);
        }

        // HttpURLConnection
        Authenticator.setDefault(null);
        CookieHandler.setDefault(null);
        ResponseCache.setDefault(null);
        HttpsURLConnection.setDefaultHostnameVerifier(defaultHostnameVerifier);
        HttpsURLConnection.setDefaultSSLSocketFactory(defaultSSLSocketFactory);

        // Logging
        LogManager.getLogManager().reset();
        Logger.getLogger("").addHandler(new ConsoleHandler());

        // Cleanup to force CloseGuard warnings etc
        System.gc();
        System.runFinalization();
    }

    private static void resetPreferences(Preferences root) {
        try {
            root.sync();
        } catch (BackingStoreException e) {
            // Indicates that Preferences is probably not working. It's not really supported on
            // Android so ignore.
            return;
        }

        try {
            for (String child : root.childrenNames()) {
                root.node(child).removeNode();
            }
            root.clear();
            root.flush();
        } catch (BackingStoreException e) {
            throw new RuntimeException(e);
        }
    }

    /** A class that always returns TRUE. */
    @SuppressWarnings("serial")
    public static class LyingMap extends HashMap<Object, Boolean> {
        @Override
        public Boolean get(Object key) {
            return Boolean.TRUE;
        }
    }

    /**
     * Does what is necessary to disable security checks for testing security-related classes.
     */
    @SuppressWarnings("unchecked")
    private static void disableSecurity() {
        try {
            Class<?> securityBrokerClass = Class.forName("javax.crypto.JceSecurity");

            Field modifiersField = Field.class.getDeclaredField("modifiers");
            modifiersField.setAccessible(true);

            Field verifyMapField = securityBrokerClass.getDeclaredField("verificationResults");
            modifiersField.setInt(verifyMapField, verifyMapField.getModifiers() & ~Modifier.FINAL);
            verifyMapField.setAccessible(true);
            verifyMapField.set(null, new LyingMap());

            Field restrictedField = securityBrokerClass.getDeclaredField("isRestricted");
            restrictedField.setAccessible(true);
            restrictedField.set(null, Boolean.FALSE);
        } catch (Exception ignored) {
        }
    }

    private static boolean hasDateFormatIs24Hour() {
        return dateFormatIs24HourField != null;
    }

    private static Boolean getDateFormatIs24Hour() {
        try {
            return (Boolean) dateFormatIs24HourField.get(null);
        } catch (IllegalAccessException e) {
            Error e2 = new AssertionError("Unable to get java.text.DateFormat.is24Hour");
            e2.initCause(e);
            throw e2;
        }
    }

    private static void setDateFormatIs24Hour(Boolean value) {
        try {
            dateFormatIs24HourField.set(null, value);
        } catch (IllegalAccessException e) {
            Error e2 = new AssertionError("Unable to set java.text.DateFormat.is24Hour");
            e2.initCause(e);
            throw e2;
        }
    }

    private static void setPropertyIfNull(String property, String value) {
        if (System.getProperty(property) == null) {
           System.setProperty(property, value);
        }
    }
}
