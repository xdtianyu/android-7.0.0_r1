/*
 * Copyright (C) 2008 The Android Open Source Project
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

import android.content.Context;
import android.os.Handler;
import android.os.Looper;
import android.os.SystemClock;
import android.test.ActivityInstrumentationTestCase2;
import android.test.UiThreadTest;
import android.view.cts.GestureDetectorCtsActivity.MockOnGestureListener;
import android.view.GestureDetector;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.GestureDetector.SimpleOnGestureListener;

public class GestureDetectorTest extends
        ActivityInstrumentationTestCase2<GestureDetectorCtsActivity> {

    private static final float X_3F = 3.0f;
    private static final float Y_4F = 4.0f;

    private GestureDetector mGestureDetector;
    private GestureDetectorCtsActivity mActivity;
    private MockOnGestureListener mListener;
    private Context mContext;

    private long mDownTime;
    private long mEventTime;
    private MotionEvent mButtonPressPrimaryMotionEvent;
    private MotionEvent mButtonPressSecondaryMotionEvent;

    public GestureDetectorTest() {
        super("android.view.cts", GestureDetectorCtsActivity.class);
    }

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mActivity = getActivity();
        mGestureDetector = mActivity.getGestureDetector();
        mListener = mActivity.getListener();
        mContext = getInstrumentation().getTargetContext();
        mActivity.isDown = false;
        mActivity.isScroll = false;
        mActivity.isFling = false;
        mActivity.isSingleTapUp = false;
        mActivity.onShowPress = false;
        mActivity.onLongPress = false;
        mActivity.onDoubleTap = false;
        mActivity.onDoubleTapEvent = false;
        mActivity.onSingleTapConfirmed = false;
        mActivity.onContextClick = false;

        mDownTime = SystemClock.uptimeMillis();
        mEventTime = SystemClock.uptimeMillis();
        mButtonPressPrimaryMotionEvent = MotionEvent.obtain(mDownTime, mEventTime,
                MotionEvent.ACTION_BUTTON_PRESS, X_3F, Y_4F, 0);
        mButtonPressPrimaryMotionEvent.setActionButton(MotionEvent.BUTTON_STYLUS_PRIMARY);

        mButtonPressSecondaryMotionEvent = MotionEvent.obtain(mDownTime, mEventTime,
                MotionEvent.ACTION_BUTTON_PRESS, X_3F, Y_4F, 0);
        mButtonPressSecondaryMotionEvent.setActionButton(MotionEvent.BUTTON_SECONDARY);
    }

    @UiThreadTest
    public void testConstructor() {

        new GestureDetector(
                mContext, new SimpleOnGestureListener(), new Handler(Looper.getMainLooper()));
        new GestureDetector(mContext, new SimpleOnGestureListener());
        new GestureDetector(new SimpleOnGestureListener(), new Handler(Looper.getMainLooper()));
        new GestureDetector(new SimpleOnGestureListener());

        try {
            mGestureDetector = new GestureDetector(null);
            fail("should throw null exception");
        } catch (RuntimeException e) {
            // expected
        }
    }

    public void testLongpressEnabled() {
        mGestureDetector.setIsLongpressEnabled(true);
        assertTrue(mGestureDetector.isLongpressEnabled());
        mGestureDetector.setIsLongpressEnabled(false);
        assertFalse(mGestureDetector.isLongpressEnabled());
    }

    public void testOnSetContextClickListener() {
        mActivity.onContextClick = false;
        mGestureDetector.setContextClickListener(null);
        mGestureDetector.onGenericMotionEvent(mButtonPressPrimaryMotionEvent);
        assertFalse(mActivity.onContextClick);

        mGestureDetector.setContextClickListener(mListener);
        mGestureDetector.onGenericMotionEvent(mButtonPressPrimaryMotionEvent);
        assertTrue(mActivity.onContextClick);
        assertSame(mButtonPressPrimaryMotionEvent, mListener.getPreviousContextClickEvent());
    }

    public void testOnContextClick() {
        mActivity.onContextClick = false;
        mListener.onContextClick(mButtonPressPrimaryMotionEvent);
        assertTrue(mActivity.onContextClick);
        assertSame(mButtonPressPrimaryMotionEvent, mListener.getPreviousContextClickEvent());

        mActivity.onContextClick = false;
        mGestureDetector.onGenericMotionEvent(mButtonPressSecondaryMotionEvent);
        assertTrue(mActivity.onContextClick);
        assertSame(mButtonPressSecondaryMotionEvent, mListener.getPreviousContextClickEvent());
    }

    public void testOnGenericMotionEvent() {
        mActivity.onContextClick = false;
        mGestureDetector.onGenericMotionEvent(mButtonPressPrimaryMotionEvent);
        assertTrue(mActivity.onContextClick);
        assertSame(mButtonPressPrimaryMotionEvent, mListener.getPreviousContextClickEvent());
    }
}
