/*
 * Copyright (C) 2010 The Android Open Source Project
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

package android.opengl.cts;

import android.app.ActivityManager;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ConfigurationInfo;
import android.content.pm.FeatureInfo;
import android.content.pm.PackageManager;
import android.test.ActivityInstrumentationTestCase2;
import android.util.Log;

import java.util.regex.Pattern;

import javax.microedition.khronos.egl.EGL10;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.egl.EGLContext;
import javax.microedition.khronos.egl.EGLDisplay;

/**
 * Test for checking whether the ro.opengles.version property is set to the correct value.
 */
public class OpenGlEsVersionTest
        extends ActivityInstrumentationTestCase2<OpenGlEsVersionCtsActivity> {

    private static final String TAG = OpenGlEsVersionTest.class.getSimpleName();

    // TODO: switch to android.opengl.EGL14/EGLExt and use the constants from there
    private static final int EGL_OPENGL_ES_BIT = 0x0001;
    private static final int EGL_OPENGL_ES2_BIT = 0x0004;
    private static final int EGL_OPENGL_ES3_BIT_KHR = 0x0040;

    private OpenGlEsVersionCtsActivity mActivity;

    public OpenGlEsVersionTest() {
        super("android.graphics.cts", OpenGlEsVersionCtsActivity.class);
    }

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mActivity = getActivity();
    }

    public void testOpenGlEsVersion() throws InterruptedException {
        int detectedMajorVersion = getDetectedMajorVersion();
        int reportedVersion = getVersionFromActivityManager(mActivity);

        assertEquals("Detected OpenGL ES major version " + detectedMajorVersion
                + " but Activity Manager is reporting " +  getMajorVersion(reportedVersion)
                + " (Check ro.opengles.version)",
                detectedMajorVersion, getMajorVersion(reportedVersion));
        assertEquals("Reported OpenGL ES version from ActivityManager differs from PackageManager",
                reportedVersion, getVersionFromPackageManager(mActivity));

        assertGlVersionString(1);
        if (detectedMajorVersion == 2) {
            restartActivityWithClientVersion(2);
            assertGlVersionString(2);
        } else if (detectedMajorVersion == 3) {
            restartActivityWithClientVersion(3);
            assertGlVersionString(3);
        }
    }

    public void testRequiredExtensions() throws InterruptedException {
        int reportedVersion = getVersionFromActivityManager(mActivity);
        // We only have required extensions on ES3.1+
        if (getMajorVersion(reportedVersion) != 3 || getMinorVersion(reportedVersion) < 1)
            return;

        restartActivityWithClientVersion(3);

        String extensions = mActivity.getExtensionsString();
        final String requiredList[] = {
            "EXT_texture_sRGB_decode",
            "KHR_blend_equation_advanced",
            "KHR_debug",
            "OES_shader_image_atomic",
            "OES_texture_stencil8",
            "OES_texture_storage_multisample_2d_array"
        };

        for (int i = 0; i < requiredList.length; ++i) {
            assertTrue("OpenGL ES version 3.1+ is missing extension " + requiredList[i],
                    hasExtension(extensions, requiredList[i]));
        }
    }

    public void testExtensionPack() throws InterruptedException {
        // Requirements:
        // 1. If the device claims support for the system feature, the extension must be available.
        // 2. If the extension is available, the device must claim support for it.
        // 3. If the extension is available, it must be correct:
        //    - ES 3.1+ must be supported
        //    - All included extensions must be available

        int reportedVersion = getVersionFromActivityManager(mActivity);
        boolean hasAepFeature = mActivity.getPackageManager().hasSystemFeature(
                PackageManager.FEATURE_OPENGLES_EXTENSION_PACK);

        if (getMajorVersion(reportedVersion) != 3 || getMinorVersion(reportedVersion) < 1) {
            assertFalse("FEATURE_OPENGLES_EXTENSION_PACK is available without OpenGL ES 3.1+",
                    hasAepFeature);
            return;
        }

        restartActivityWithClientVersion(3);

        String extensions = mActivity.getExtensionsString();
        if (!hasExtension(extensions, "ANDROID_extension_pack_es31a")) {
            assertFalse("FEATURE_OPENGLES_EXTENSION_PACK is available but ANDROID_extension_pack_es31a isn't in the extension list",
                    hasAepFeature);
            return;
        }

        assertTrue("ANDROID_extension_pack_es31a is present, but support is incomplete",
                mActivity.getAepEs31Support());
    }

    public void testOpenGlEsVersionForVrHighPerformance() throws InterruptedException {
        if (!supportsVrHighPerformance())
            return;
        restartActivityWithClientVersion(3);

        int reportedVersion = getVersionFromActivityManager(mActivity);
        int major = getMajorVersion(reportedVersion);
        int minor = getMinorVersion(reportedVersion);

        assertTrue("OpenGL ES version 3.2 or higher is required for VR high-performance devices " +
            " but this device supports only version " + major + "." + minor,
            (major == 3 && minor >= 2) || major > 3);
    }

    public void testRequiredExtensionsForVrHighPerformance() throws InterruptedException {
        if (!supportsVrHighPerformance())
            return;
        restartActivityWithClientVersion(3);

        String extensions = mActivity.getExtensionsString();
        final String requiredList[] = {
            "EXT_protected_textures",
            "EXT_multisampled_render_to_texture",
            "OVR_multiview",
            "OVR_multiview_multisampled_render_to_texture",
            "OVR_multiview2",
        };

        for (int i = 0; i < requiredList.length; ++i) {
            assertTrue("Required extension for VR high-performance is missing: " + requiredList[i],
                    hasExtension(extensions, requiredList[i]));
        }

        EGL10 egl = (EGL10) EGLContext.getEGL();
        EGLDisplay display = egl.eglGetDisplay(EGL10.EGL_DEFAULT_DISPLAY);
        extensions = egl.eglQueryString(display, EGL10.EGL_EXTENSIONS);
        final String requiredEglList[] = {
            "EGL_ANDROID_create_native_client_buffer",
            "EGL_ANDROID_front_buffer_auto_refresh",
            "EGL_EXT_protected_content",
            "EGL_KHR_mutable_render_buffer",
            "EGL_KHR_wait_sync",
        };

        for (int i = 0; i < requiredList.length; ++i) {
            assertTrue("Required EGL extension for VR high-performance is missing: " +
                requiredEglList[i], hasExtension(extensions, requiredEglList[i]));
        }
    }

    private static boolean hasExtension(String extensions, String name) {
        return OpenGlEsVersionCtsActivity.hasExtension(extensions, name);
    }

    /** @return OpenGL ES major version 1, 2, or 3 or some non-positive number for error */
    private static int getDetectedMajorVersion() {
        /*
         * Get all the device configurations and check the EGL_RENDERABLE_TYPE attribute
         * to determine the highest ES version supported by any config. The
         * EGL_KHR_create_context extension is required to check for ES3 support; if the
         * extension is not present this test will fail to detect ES3 support. This
         * effectively makes the extension mandatory for ES3-capable devices.
         */

        EGL10 egl = (EGL10) EGLContext.getEGL();
        EGLDisplay display = egl.eglGetDisplay(EGL10.EGL_DEFAULT_DISPLAY);
        int[] numConfigs = new int[1];

        if (egl.eglInitialize(display, null)) {
            try {
                boolean checkES3 = hasExtension(egl.eglQueryString(display, EGL10.EGL_EXTENSIONS),
                        "EGL_KHR_create_context");
                if (egl.eglGetConfigs(display, null, 0, numConfigs)) {
                    EGLConfig[] configs = new EGLConfig[numConfigs[0]];
                    if (egl.eglGetConfigs(display, configs, numConfigs[0], numConfigs)) {
                        int highestEsVersion = 0;
                        int[] value = new int[1];
                        for (int i = 0; i < numConfigs[0]; i++) {
                            if (egl.eglGetConfigAttrib(display, configs[i],
                                    EGL10.EGL_RENDERABLE_TYPE, value)) {
                                if (checkES3 && ((value[0] & EGL_OPENGL_ES3_BIT_KHR) ==
                                        EGL_OPENGL_ES3_BIT_KHR)) {
                                    if (highestEsVersion < 3) highestEsVersion = 3;
                                } else if ((value[0] & EGL_OPENGL_ES2_BIT) == EGL_OPENGL_ES2_BIT) {
                                    if (highestEsVersion < 2) highestEsVersion = 2;
                                } else if ((value[0] & EGL_OPENGL_ES_BIT) == EGL_OPENGL_ES_BIT) {
                                    if (highestEsVersion < 1) highestEsVersion = 1;
                                }
                            } else {
                                Log.w(TAG, "Getting config attribute with "
                                        + "EGL10#eglGetConfigAttrib failed "
                                        + "(" + i + "/" + numConfigs[0] + "): "
                                        + egl.eglGetError());
                            }
                        }
                        return highestEsVersion;
                    } else {
                        Log.e(TAG, "Getting configs with EGL10#eglGetConfigs failed: "
                                + egl.eglGetError());
                        return -1;
                    }
                } else {
                    Log.e(TAG, "Getting number of configs with EGL10#eglGetConfigs failed: "
                            + egl.eglGetError());
                    return -2;
                }
              } finally {
                  egl.eglTerminate(display);
              }
        } else {
            Log.e(TAG, "Couldn't initialize EGL.");
            return -3;
        }
    }

    private static int getVersionFromActivityManager(Context context) {
        ActivityManager activityManager =
            (ActivityManager) context.getSystemService(Context.ACTIVITY_SERVICE);
        ConfigurationInfo configInfo = activityManager.getDeviceConfigurationInfo();
        if (configInfo.reqGlEsVersion != ConfigurationInfo.GL_ES_VERSION_UNDEFINED) {
            return configInfo.reqGlEsVersion;
        } else {
            return 1 << 16; // Lack of property means OpenGL ES version 1
        }
    }

    private static int getVersionFromPackageManager(Context context) {
        PackageManager packageManager = context.getPackageManager();
        FeatureInfo[] featureInfos = packageManager.getSystemAvailableFeatures();
        if (featureInfos != null && featureInfos.length > 0) {
            for (FeatureInfo featureInfo : featureInfos) {
                // Null feature name means this feature is the open gl es version feature.
                if (featureInfo.name == null) {
                    if (featureInfo.reqGlEsVersion != FeatureInfo.GL_ES_VERSION_UNDEFINED) {
                        return featureInfo.reqGlEsVersion;
                    } else {
                        return 1 << 16; // Lack of property means OpenGL ES version 1
                    }
                }
            }
        }
        return 1;
    }

    /** @see FeatureInfo#getGlEsVersion() */
    private static int getMajorVersion(int glEsVersion) {
        return ((glEsVersion & 0xffff0000) >> 16);
    }

    /** @see FeatureInfo#getGlEsVersion() */
    private static int getMinorVersion(int glEsVersion) {
        return glEsVersion & 0xffff;
    }

    /**
     * Check that the version string has some form of "Open GL ES X.Y" in it where X is the major
     * version and Y must be some digit.
     */
    private void assertGlVersionString(int majorVersion) throws InterruptedException {
        String versionString = "" + majorVersion;
        String message = "OpenGL version string '" + mActivity.getVersionString()
                + "' is not " + majorVersion + ".0+.";
        assertTrue(message, Pattern.matches(".*OpenGL.*ES.*" + versionString + "\\.\\d.*",
                mActivity.getVersionString()));
    }

    /** Restart {@link GLSurfaceViewCtsActivity} with a specific client version. */
    private void restartActivityWithClientVersion(int version) {
        mActivity.finish();
        setActivity(null);

        try {
            Intent intent = OpenGlEsVersionCtsActivity.createIntent(version);
            setActivityIntent(intent);
            mActivity = getActivity();
        } finally {
            setActivityIntent(null);
        }
    }

    /**
     * Return whether the system supports FEATURE_VR_MODE and
     * FEATURE_VR_MODE_HIGH_PERFORMANCE. This is used to skip some tests.
     */
    private boolean supportsVrHighPerformance() {
        PackageManager pm = mActivity.getPackageManager();
        return pm.hasSystemFeature(PackageManager.FEATURE_VR_MODE_HIGH_PERFORMANCE);
    }
}
