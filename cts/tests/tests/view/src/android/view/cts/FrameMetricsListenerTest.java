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

package android.view.cts;

import android.view.cts.R;

import android.app.Activity;
import android.app.Instrumentation;
import android.cts.util.PollingCheck;
import android.os.Looper;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.SystemClock;
import android.test.ActivityInstrumentationTestCase2;
import android.util.Log;
import android.view.FrameMetrics;
import android.view.View;
import android.view.Window;
import android.widget.ScrollView;

import java.lang.Thread;
import java.lang.Exception;
import java.lang.System;
import java.util.ArrayList;
import java.util.concurrent.atomic.AtomicInteger;

public class FrameMetricsListenerTest extends ActivityInstrumentationTestCase2<MockActivity> {

    private Instrumentation mInstrumentation;
    private Window.OnFrameMetricsAvailableListener mFrameMetricsListener;
    private Activity mActivity;

    public FrameMetricsListenerTest() {
        super(MockActivity.class);
    }

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mActivity = getActivity();
        mInstrumentation = getInstrumentation();
    }

    private void layout(final int layoutId) {
        mInstrumentation.runOnMainSync(new Runnable() {
            @Override
            public void run() {
                mActivity.setContentView(layoutId);
            }
        });
        mInstrumentation.waitForIdleSync();
    }

    public void testReceiveData() throws Throwable {
        layout(R.layout.scrollview_layout);
        final ScrollView scrollView = (ScrollView) mActivity.findViewById(R.id.scroll_view);
        final ArrayList<FrameMetrics> data = new ArrayList<>();
        final Handler handler = new Handler(Looper.getMainLooper());
        final Window myWindow = mActivity.getWindow();
        final Window.OnFrameMetricsAvailableListener listener =
            new Window.OnFrameMetricsAvailableListener() {
               @Override
               public void onFrameMetricsAvailable(Window window, FrameMetrics frameMetrics,
                       int dropCount) {
                   assertEquals(myWindow, window);
                   assertEquals(0, dropCount);
                   data.add(new FrameMetrics(frameMetrics));
               }
            };
        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
                mActivity.getWindow().addOnFrameMetricsAvailableListener(listener, handler);
            }
        });

        mInstrumentation.waitForIdleSync();

        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
                scrollView.fling(-100);
            }
        });

        mInstrumentation.waitForIdleSync();
        new PollingCheck() {
            @Override
            protected boolean check() {
                return data.size() != 0;
            }
        }.run();

        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
                mActivity.getWindow().removeOnFrameMetricsAvailableListener(listener);
                data.clear();
            }
        });

        mInstrumentation.waitForIdleSync();

        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
                scrollView.fling(100);
                assertEquals(0, data.size());
            }
        });

        mInstrumentation.waitForIdleSync();
    }

    public void testMultipleListeners() throws Throwable {
        layout(R.layout.scrollview_layout);
        final ScrollView scrollView = (ScrollView) mActivity.findViewById(R.id.scroll_view);
        final ArrayList<FrameMetrics> data1 = new ArrayList<>();
        final Handler handler = new Handler(Looper.getMainLooper());
        final Window myWindow = mActivity.getWindow();

        final Window.OnFrameMetricsAvailableListener frameMetricsListener1 =
            new Window.OnFrameMetricsAvailableListener() {
               @Override
               public void onFrameMetricsAvailable(Window window, FrameMetrics frameMetrics,
                       int dropCount) {
                   assertEquals(myWindow, window);
                   assertEquals(0, dropCount);
                   data1.add(new FrameMetrics(frameMetrics));
               }
            };
        final ArrayList<FrameMetrics> data2 = new ArrayList<>();
        final Window.OnFrameMetricsAvailableListener frameMetricsListener2 =
            new Window.OnFrameMetricsAvailableListener() {
               @Override
               public void onFrameMetricsAvailable(Window window, FrameMetrics frameMetrics,
                       int dropCount) {
                   assertEquals(myWindow, window);
                   assertEquals(0, dropCount);
                   data2.add(new FrameMetrics(frameMetrics));
               }
            };
        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
                mActivity.getWindow().addOnFrameMetricsAvailableListener(
                        frameMetricsListener1, handler);
                mActivity.getWindow().addOnFrameMetricsAvailableListener(
                        frameMetricsListener2, handler);
            }
        });

        mInstrumentation.waitForIdleSync();

        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
                scrollView.fling(-100);
            }
        });

        mInstrumentation.waitForIdleSync();
        new PollingCheck() {
            @Override
            protected boolean check() {
                return data1.size() != 0 && data1.size() == data2.size();
            }
        }.run();

        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
                mActivity.getWindow().removeOnFrameMetricsAvailableListener(frameMetricsListener1);
                mActivity.getWindow().removeOnFrameMetricsAvailableListener(frameMetricsListener2);
            }
        });
    }

    public void testDropCount() throws Throwable {
        layout(R.layout.scrollview_layout);
        final ScrollView scrollView = (ScrollView) mActivity.findViewById(R.id.scroll_view);

        final Window window = mActivity.getWindow();
        final AtomicInteger framesDropped = new AtomicInteger();
        final AtomicInteger frameCount = new AtomicInteger();

        final HandlerThread thread = new HandlerThread("Listener");
        thread.start();
        final Handler handler = new Handler(thread.getLooper());
        final Window.OnFrameMetricsAvailableListener frameMetricsListener =
            new Window.OnFrameMetricsAvailableListener() {
               @Override
               public void onFrameMetricsAvailable(Window window, FrameMetrics frameMetrics,
                       int dropCount) {
                    try {
                        Thread.sleep(100);
                        framesDropped.addAndGet(dropCount);
                    } catch (Exception e) { }
               }
            };

        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
                mActivity.getWindow().addOnFrameMetricsAvailableListener(frameMetricsListener, handler);
            }
        });

        mInstrumentation.waitForIdleSync();

        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
                scrollView.fling(-100);
            }
        });

        mInstrumentation.waitForIdleSync();
        new PollingCheck() {
            @Override
            protected boolean check() {
                return framesDropped.get() > 0;
            }
        }.run();

        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
                mActivity.getWindow().removeOnFrameMetricsAvailableListener(frameMetricsListener);
            }
        });
    }
}


