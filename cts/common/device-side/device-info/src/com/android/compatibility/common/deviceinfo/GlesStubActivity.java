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

import android.app.Activity;
import android.app.ActivityManager;
import android.app.Instrumentation;
import android.content.Context;
import android.content.pm.ConfigurationInfo;
import android.content.res.Configuration;
import android.os.Bundle;
import android.view.Window;
import android.view.WindowManager;
import android.opengl.GLES20;
import android.opengl.GLES30;
import android.opengl.GLSurfaceView;
import android.util.Log;

import java.util.ArrayList;
import java.util.List;
import java.util.Locale;
import java.util.HashSet;
import java.util.Scanner;
import java.util.Set;
import java.util.concurrent.CountDownLatch;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

/** Stub activity to collect data from the GlesView */
public final class GlesStubActivity extends Activity {

    private static final String LOG_TAG = "GlesStubActivity";
    private int mVersion = -1;
    private GraphicsDeviceInfo mGraphicsDeviceInfo;
    private CountDownLatch mDone = new CountDownLatch(1);
    private HashSet<String> mOpenGlExtensions = new HashSet<String>();
    private HashSet<String> mFormats = new HashSet<String>();
    private String mGraphicsVendor;
    private String mGraphicsRenderer;

    @Override
    public void onCreate(Bundle bundle) {
        // Dismiss keyguard and keep screen on while this test is on.
        Window window = getWindow();
        window.addFlags(WindowManager.LayoutParams.FLAG_DISMISS_KEYGUARD |
                WindowManager.LayoutParams.FLAG_TURN_SCREEN_ON |
                WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        super.onCreate(bundle);

        window.setFlags(WindowManager.LayoutParams.FLAG_SHOW_WHEN_LOCKED,
                WindowManager.LayoutParams.FLAG_SHOW_WHEN_LOCKED);

        ActivityManager activityManager =
                (ActivityManager) getSystemService(Context.ACTIVITY_SERVICE);
        ConfigurationInfo info = activityManager.getDeviceConfigurationInfo();

        mVersion = (info.reqGlEsVersion & 0xffff0000) >> 16;

        new Thread() {
            @Override
            public void run() {
                runIterations(mVersion);
            }
        }.start();
    }

     /**
     * Wait for this activity to finish gathering information
     */
    public void waitForActivityToFinish() {
        try {
            mDone.await();
        } catch (InterruptedException e) {
            // just move on
        }
    }

    private void runIterations(int glVersion) {
        for (int i = 1; i <= glVersion; i++) {
            final CountDownLatch done = new CountDownLatch(1);
            final int version = i;
            GlesStubActivity.this.runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    setContentView(new GlesSurfaceView(GlesStubActivity.this, version, done));
                }
            });
            try {
                done.await();
            } catch (InterruptedException e) {
                // just move on
            }
        }
        mDone.countDown();
    }

    int getGlVersion() {
        return mVersion;
    }

    List<String> getOpenGlExtensions() {
        return new ArrayList<>(mOpenGlExtensions);
    }

    void addOpenGlExtension(String openGlExtension) {
        mOpenGlExtensions.add(openGlExtension);
    }

    List<String> getCompressedTextureFormats() {
        return new ArrayList<>(mFormats);
    }

    void addCompressedTextureFormat(String format) {
        mFormats.add(format);
    }

    String getVendor() {
        return mGraphicsVendor;
    }

    void setVendor(String vendor) {
        mGraphicsVendor = vendor;
    }

    String getRenderer() {
        return mGraphicsRenderer;
    }

    void setRenderer(String renderer) {
        mGraphicsRenderer = renderer;
    }

    static class GlesSurfaceView extends GLSurfaceView {

        public GlesSurfaceView(GlesStubActivity parent, int glVersion, CountDownLatch done) {
            super(parent);

            if (glVersion > 1) {
                // Default is 1 so only set if bigger than 1
                setEGLContextClientVersion(glVersion);
            }
            setRenderer(new OpenGlesRenderer(parent, glVersion, done));
        }
    }

    static class OpenGlesRenderer implements GLSurfaceView.Renderer {

        private final GlesStubActivity mParent;
        private final int mGlVersion;
        private final CountDownLatch mDone;

        OpenGlesRenderer(GlesStubActivity parent, int glVersion, CountDownLatch done) {
            mParent = parent;
            mGlVersion = glVersion;
            mDone = done;
        }

        @Override
        public void onSurfaceCreated(GL10 gl, EGLConfig config) {
            String extensions;
            String vendor;
            String renderer;
            if (mGlVersion == 2) {
                extensions = GLES20.glGetString(GLES20.GL_EXTENSIONS);
                vendor = GLES20.glGetString(GLES20.GL_VENDOR);
                renderer = GLES20.glGetString(GLES20.GL_RENDERER);
            } else if (mGlVersion == 3) {
                extensions = GLES30.glGetString(GLES30.GL_EXTENSIONS);
                vendor = GLES30.glGetString(GLES30.GL_VENDOR);
                renderer = GLES30.glGetString(GLES30.GL_RENDERER);
            } else {
                extensions = gl.glGetString(GL10.GL_EXTENSIONS);
                vendor = gl.glGetString(GL10.GL_VENDOR);
                renderer = gl.glGetString(GL10.GL_RENDERER);
            }
            mParent.setVendor(vendor);
            mParent.setRenderer(renderer);
            Scanner scanner = new Scanner(extensions);
            scanner.useDelimiter(" ");
            while (scanner.hasNext()) {
                String ext = scanner.next();
                mParent.addOpenGlExtension(ext);
                if (ext.contains("texture")) {
                    if (ext.contains("compression") || ext.contains("compressed")) {
                        mParent.addCompressedTextureFormat(ext);
                    }
                }
            }
            scanner.close();
            mDone.countDown();
        }

        @Override
        public void onSurfaceChanged(GL10 gl, int width, int height) {}

        @Override
        public void onDrawFrame(GL10 gl) {}
    }
}
