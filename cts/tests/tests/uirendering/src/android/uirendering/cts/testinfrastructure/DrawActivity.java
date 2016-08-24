/*
 * Copyright (C) 2014 The Android Open Source Project
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
package android.uirendering.cts.testinfrastructure;

import android.annotation.Nullable;
import android.app.Activity;
import android.content.res.Configuration;
import android.graphics.Point;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.view.View;
import android.view.ViewStub;
import android.view.ViewTreeObserver;
import android.webkit.WebView;

import android.uirendering.cts.R;

/**
 * A generic activity that uses a view specified by the user.
 */
public class DrawActivity extends Activity {
    private final static long TIME_OUT_MS = 10000;
    private final Point mLock = new Point();
    public static final int MIN_NUMBER_OF_DRAWS = 20;

    private Handler mHandler;
    private View mView;
    private View mViewWrapper;
    private boolean mOnTv;

    public void onCreate(Bundle bundle){
        super.onCreate(bundle);
        getWindow().getDecorView().setSystemUiVisibility(
                View.SYSTEM_UI_FLAG_HIDE_NAVIGATION | View.SYSTEM_UI_FLAG_FULLSCREEN);
        mHandler = new RenderSpecHandler();
        int uiMode = getResources().getConfiguration().uiMode;
        mOnTv = (uiMode & Configuration.UI_MODE_TYPE_MASK) == Configuration.UI_MODE_TYPE_TELEVISION;
    }

    public boolean getOnTv() {
        return mOnTv;
    }

    public Point enqueueRenderSpecAndWait(int layoutId, CanvasClient canvasClient, String webViewUrl,
            @Nullable ViewInitializer viewInitializer, boolean useHardware) {
        ((RenderSpecHandler) mHandler).setViewInitializer(viewInitializer);
        int arg2 = (useHardware ? View.LAYER_TYPE_NONE : View.LAYER_TYPE_SOFTWARE);
        if (canvasClient != null) {
            mHandler.obtainMessage(RenderSpecHandler.CANVAS_MSG, 0, arg2, canvasClient).sendToTarget();
        } else if (webViewUrl != null) {
            mHandler.obtainMessage(RenderSpecHandler.WEB_VIEW_MSG, 0, arg2, webViewUrl).sendToTarget();
        } else {
            mHandler.obtainMessage(RenderSpecHandler.LAYOUT_MSG, layoutId, arg2).sendToTarget();
        }

        Point point = new Point();
        synchronized (mLock) {
            try {
                mLock.wait(TIME_OUT_MS);
                point.set(mLock.x, mLock.y);
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }
        return point;
    }

    private ViewInitializer mViewInitializer;

    private class RenderSpecHandler extends Handler {
        public static final int LAYOUT_MSG = 1;
        public static final int CANVAS_MSG = 2;
        public static final int WEB_VIEW_MSG = 3;


        public void setViewInitializer(ViewInitializer viewInitializer) {
            mViewInitializer = viewInitializer;
        }

        public void handleMessage(Message message) {
            int drawCountDelay = 0;
            setContentView(R.layout.test_container);
            ViewStub stub = (ViewStub) findViewById(R.id.test_content_stub);
            mViewWrapper = findViewById(R.id.test_content_wrapper);
            switch (message.what) {
                case LAYOUT_MSG: {
                    stub.setLayoutResource(message.arg1);
                    mView = stub.inflate();
                } break;

                case CANVAS_MSG: {
                    stub.setLayoutResource(R.layout.test_content_canvasclientview);
                    mView = stub.inflate();
                    ((CanvasClientView) mView).setCanvasClient((CanvasClient) (message.obj));
                } break;

                case WEB_VIEW_MSG: {
                    stub.setLayoutResource(R.layout.test_content_webview);
                    mView = stub.inflate();
                    ((WebView) mView).loadUrl((String) message.obj);
                    ((WebView) mView).setInitialScale(100);
                    drawCountDelay = 10;
                } break;
            }

            if (mView == null) {
                throw new IllegalStateException("failed to inflate test content");
            }

            if (mViewInitializer != null) {
                mViewInitializer.initializeView(mView);
            }

            // set layer on wrapper parent of view, so view initializer
            // can control layer type of View under test.
            mViewWrapper.setLayerType(message.arg2, null);

            DrawCounterListener onDrawListener = new DrawCounterListener(drawCountDelay);

            mView.getViewTreeObserver().addOnPreDrawListener(onDrawListener);

            mView.postInvalidate();
        }
    }

    @Override
    protected void onPause() {
        super.onPause();
        if (mViewInitializer != null) {
            mViewInitializer.teardownView();
        }
    }

    private class DrawCounterListener implements ViewTreeObserver.OnPreDrawListener {
        private int mCurrentDraws = 0;
        private int mExtraDraws;

        public DrawCounterListener(int extraDraws) {
            mExtraDraws = extraDraws;
        }

        @Override
        public boolean onPreDraw() {
            mCurrentDraws++;
            if (mCurrentDraws < MIN_NUMBER_OF_DRAWS + mExtraDraws) {
                mView.postInvalidate();
            } else {
                synchronized (mLock) {
                    mLock.set(mViewWrapper.getLeft(), mViewWrapper.getTop());
                    mLock.notify();
                }
                mView.getViewTreeObserver().removeOnPreDrawListener(this);
            }
            return true;
        }
    }
}
