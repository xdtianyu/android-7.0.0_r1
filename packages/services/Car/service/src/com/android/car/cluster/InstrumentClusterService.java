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
package com.android.car.cluster;

import static com.android.car.cluster.InstrumentClusterRendererLoader.createRenderer;
import static com.android.car.cluster.InstrumentClusterRendererLoader.createRendererPackageContext;

import android.annotation.IntDef;
import android.annotation.Nullable;
import android.annotation.SystemApi;
import android.car.cluster.renderer.InstrumentClusterRenderer;
import android.car.cluster.renderer.NavigationRenderer;
import android.content.Context;
import android.hardware.display.DisplayManager;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.util.Log;
import android.view.Display;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import com.android.car.CarLog;
import com.android.car.CarServiceBase;
import com.android.car.CarServiceUtils;
import com.android.car.R;

import java.io.PrintWriter;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.util.concurrent.CopyOnWriteArrayList;

/**
 * Service responsible for interaction with car's instrument cluster.
 *
 * @hide
 */
@SystemApi
public class InstrumentClusterService implements CarServiceBase {

    private static final String TAG = CarLog.TAG_CLUSTER + "."
            + InstrumentClusterService.class.getSimpleName();

    private static final int RENDERER_INIT_TIMEOUT_MS = 10 * 1000;
    private final static int MSG_TIMEOUT = 1;

    private final Context mContext;
    private final Object mHalSync = new Object();
    private final CopyOnWriteArrayList<RendererInitializationListener>
            mRendererInitializationListeners = new CopyOnWriteArrayList<>();

    private InstrumentClusterRenderer mRenderer;

    private final Handler mHandler;

    public InstrumentClusterService(Context context) {
        mContext = context;
        mHandler = new TimeoutHandler();
    }

    public interface RendererInitializationListener {
        void onRendererInitSucceeded();
    }

    @Override
    public void init() {
        Log.d(TAG, "init");

        if (getInstrumentClusterType() == InstrumentClusterType.GRAPHICS) {
            Display display = getInstrumentClusterDisplay(mContext);
            boolean rendererFound = InstrumentClusterRendererLoader.isRendererAvailable(mContext);

            if (display != null && rendererFound) {
                initRendererOnMainThread(display);
                mHandler.sendMessageDelayed(mHandler.obtainMessage(MSG_TIMEOUT),
                        RENDERER_INIT_TIMEOUT_MS);
            } else {
                mClusterType = InstrumentClusterType.NONE;
                Log.w(TAG, "Failed to initialize InstrumentClusterRenderer"
                        + ", renderer found: " + rendererFound
                        + ", secondary display: " + (display != null), new RuntimeException());

                return;
            }
        }
    }

    private void initRendererOnMainThread(final Display display) {
        CarServiceUtils.runOnMain(new Runnable() {
            @Override
            public void run() {
                Log.d(TAG, "initRendererOnMainThread");
                try {
                    InstrumentClusterPresentation presentation =
                            new InstrumentClusterPresentation(mContext, display);

                    ViewGroup rootView = (ViewGroup) LayoutInflater.from(mContext).inflate(
                            R.layout.instrument_cluster, null);

                    presentation.setContentView(rootView);
                    InstrumentClusterRenderer renderer = createRenderer(mContext);
                    renderer.onCreate(createRendererPackageContext(mContext));
                    View rendererView = renderer.onCreateView(null);
                    renderer.initialize();
                    rootView.addView(rendererView);
                    presentation.show();
                    renderer.onStart();
                    initUiDone(renderer);
                } catch (Exception e) {
                    Log.e(TAG, e.getMessage(), e);
                    throw e;
                }
            }
        });
    }

    private void initUiDone(final InstrumentClusterRenderer renderer) {
        Log.d(TAG, "initUiDone");
        mHandler.removeMessages(MSG_TIMEOUT);

        // Call listeners in service thread.
        runOnServiceThread(new Runnable() {
            @Override
            public void run() {
                mRenderer = renderer;

                for (RendererInitializationListener listener : mRendererInitializationListeners) {
                    listener.onRendererInitSucceeded();
                }
            }
        });
    }

    @Override
    public void release() {
        Log.d(TAG, "release");
    }

    @Override
    public void dump(PrintWriter writer) {
        // TODO
    }

    @Retention(RetentionPolicy.SOURCE)
    @IntDef({
            InstrumentClusterType.NONE,
            InstrumentClusterType.METADATA,
            InstrumentClusterType.GRAPHICS
    })
    public @interface InstrumentClusterType {
        /**
         * For use privately in this class.
         * @hide
         */
        int UNDEFINED = -1;

        /** Access to instrument cluster is not available */
        int NONE = 0;

        /** Access to instrument cluster through vehicle HAL using meta-data. */
        int METADATA = 1;

        /** Access instrument cluster as a secondary display. */
        int GRAPHICS = 2;
    }

    @InstrumentClusterType private int mClusterType = InstrumentClusterType.UNDEFINED;

    public boolean isInstrumentClusterAvailable() {
        return mClusterType != InstrumentClusterType.NONE
                && mClusterType != InstrumentClusterType.UNDEFINED;
    }

    public int getInstrumentClusterType() {
        if (mClusterType == InstrumentClusterType.UNDEFINED) {
            synchronized (mHalSync) {
                // TODO: need to pull this information from the HAL
                mClusterType = getInstrumentClusterDisplay(mContext) != null
                        ? InstrumentClusterType.GRAPHICS : InstrumentClusterType.NONE;
            }
        }
        return mClusterType;
    }

    @Nullable
    public NavigationRenderer getNavigationRenderer() {
        return mRenderer != null ? mRenderer.getNavigationRenderer() : null;
    }

    public void registerListener(RendererInitializationListener listener) {
        mRendererInitializationListeners.add(listener);
    }

    public void unregisterListener(RendererInitializationListener listener) {
        mRendererInitializationListeners.remove(listener);
    }

    private static Display getInstrumentClusterDisplay(Context context) {
        DisplayManager displayManager = context.getSystemService(DisplayManager.class);
        Display[] displays = displayManager.getDisplays();

        Log.d(TAG, "There are currently " + displays.length + " displays connected.");
        for (Display display : displays) {
            Log.d(TAG, "  " + display);
        }

        if (displays.length > 1) {
            // TODO: assuming that secondary display is instrument cluster. Put this into settings?
            return displays[1];
        }
        return null;
    }

    private void runOnServiceThread(final Runnable runnable) {
        if (Looper.myLooper() == mHandler.getLooper()) {
            runnable.run();
        } else {
            mHandler.post(runnable);
        }
    }

    private static class TimeoutHandler extends Handler {
        @Override
        public void handleMessage(Message msg) {
            if (msg.what == MSG_TIMEOUT) {
                Log.e(TAG, "Renderer initialization timeout.", new RuntimeException());
            } else {
                super.handleMessage(msg);
            }
        }
    }
}
