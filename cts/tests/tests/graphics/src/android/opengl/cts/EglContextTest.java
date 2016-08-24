/*
 * Copyright (C) 2013 The Android Open Source Project
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

import android.app.Activity;
import android.opengl.EGL14;
import android.opengl.EGLConfig;
import android.opengl.EGLContext;
import android.opengl.EGLDisplay;
import android.opengl.GLES20;
import android.test.ActivityInstrumentationTestCase2;

/**
 * Tests using EGL contexts.
 */
public class EglContextTest extends ActivityInstrumentationTestCase2<Activity> {

    public EglContextTest() {
        super(Activity.class);
    }

    /**
     * Tests creating then releasing an EGL context.
     */
    public void testCreateAndReleaseContext() {
        EGLDisplay eglDisplay = null;
        EGLContext eglContext = null;
        try {
            eglDisplay = createEglDisplay();
            eglContext = createEglContext(eglDisplay);
            destroyEglContext(eglDisplay, eglContext);
            eglDisplay = null;
            eglContext = null;
        } finally {
            if (eglDisplay != null) {
                if (eglContext != null) {
                    EGL14.eglDestroyContext(eglDisplay, eglContext);
                }

                EGL14.eglTerminate(eglDisplay);
            }
        }
    }

    /**
     * Returns an initialized default display.
     */
    private static EGLDisplay createEglDisplay() {
        EGLDisplay eglDisplay = EGL14.eglGetDisplay(EGL14.EGL_DEFAULT_DISPLAY);
        if (eglDisplay == EGL14.EGL_NO_DISPLAY) {
            throw new IllegalStateException("no EGL display");
        }

        int[] major = new int[1];
        int[] minor = new int[1];
        if (!EGL14.eglInitialize(eglDisplay, major, 0, minor, 0)) {
            throw new IllegalStateException("error in eglInitialize");
        }
        checkGlError();

        return eglDisplay;
    }

    /**
     * Returns a new GL context for the specified {@code eglDisplay}.
     */
    private static EGLContext createEglContext(EGLDisplay eglDisplay) {
        int[] contextAttributes = { EGL14.EGL_CONTEXT_CLIENT_VERSION, 2, EGL14.EGL_NONE };
        EGLContext eglContext = EGL14.eglCreateContext(eglDisplay,
                getEglConfig(eglDisplay), EGL14.EGL_NO_CONTEXT, contextAttributes, 0);
        checkGlError();

        return eglContext;
    }

    /**
     * Destroys the GL context identifier by {@code eglDisplay} and {@code eglContext}.
     */
    private static void destroyEglContext(EGLDisplay eglDisplay, EGLContext eglContext) {
        EGL14.eglMakeCurrent(eglDisplay,
                EGL14.EGL_NO_SURFACE,
                EGL14.EGL_NO_SURFACE,
                EGL14.EGL_NO_CONTEXT);
        int error = EGL14.eglGetError();
        if (error != EGL14.EGL_SUCCESS) {
            throw new RuntimeException("error releasing context: " + error);
        }

        EGL14.eglDestroyContext(eglDisplay, eglContext);
        error = EGL14.eglGetError();
        if (error != EGL14.EGL_SUCCESS) {
            throw new RuntimeException("error destroying context: " + error);
        }

        EGL14.eglReleaseThread();
        error = EGL14.eglGetError();
        if (error != EGL14.EGL_SUCCESS) {
            throw new RuntimeException("error releasing thread: " + error);
        }

        EGL14.eglTerminate(eglDisplay);
        error = EGL14.eglGetError();
        if (error != EGL14.EGL_SUCCESS) {
            throw new RuntimeException("error terminating display: " + error);
        }
    }

    private static EGLConfig getEglConfig(EGLDisplay eglDisplay) {
        // Get an EGLConfig.
        final int EGL_OPENGL_ES2_BIT = 4;
        final int RED_SIZE = 8;
        final int GREEN_SIZE = 8;
        final int BLUE_SIZE = 8;
        final int ALPHA_SIZE = 8;
        final int DEPTH_SIZE = 0;
        final int STENCIL_SIZE = 0;
        final int[] DEFAULT_CONFIGURATION = new int[] {
                EGL14.EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
                EGL14.EGL_RED_SIZE, RED_SIZE,
                EGL14.EGL_GREEN_SIZE, GREEN_SIZE,
                EGL14.EGL_BLUE_SIZE, BLUE_SIZE,
                EGL14.EGL_ALPHA_SIZE, ALPHA_SIZE,
                EGL14.EGL_DEPTH_SIZE, DEPTH_SIZE,
                EGL14.EGL_STENCIL_SIZE, STENCIL_SIZE,
                EGL14.EGL_NONE};

        int[] configsCount = new int[1];
        EGLConfig[] eglConfigs = new EGLConfig[1];
        if (!EGL14.eglChooseConfig(
                eglDisplay, DEFAULT_CONFIGURATION, 0, eglConfigs, 0, 1, configsCount, 0)) {
            throw new RuntimeException("eglChooseConfig failed");
        }
        return eglConfigs[0];
    }

    /**
     * Checks for a GL error using {@link GLES20#glGetError()}.
     *
     * @throws RuntimeException if there is a GL error
     */
    private static void checkGlError() {
        int errorCode;
        if ((errorCode = GLES20.glGetError()) != GLES20.GL_NO_ERROR) {
            throw new RuntimeException("gl error: " + Integer.toHexString(errorCode));
        }
    }

}
