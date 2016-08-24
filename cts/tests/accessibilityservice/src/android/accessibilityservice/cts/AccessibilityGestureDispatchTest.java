/**
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except
 * in compliance with the License. You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the
 * License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing permissions and
 * limitations under the License.
 */

package android.accessibilityservice.cts;

import android.accessibilityservice.AccessibilityService;
import android.accessibilityservice.GestureDescription;
import android.content.pm.PackageManager;
import android.content.res.Resources;
import android.graphics.Matrix;
import android.graphics.Path;
import android.graphics.Rect;
import android.os.Bundle;
import android.os.SystemClock;
import android.test.ActivityInstrumentationTestCase2;
import android.util.DisplayMetrics;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewConfiguration;
import android.widget.TextView;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.atomic.AtomicBoolean;

/**
 * Verify that gestures dispatched from an accessibility service show up in the current UI
 */
public class AccessibilityGestureDispatchTest extends
        ActivityInstrumentationTestCase2<AccessibilityGestureDispatchTest.GestureDispatchActivity> {
    private static final int GESTURE_COMPLETION_TIMEOUT = 5000; // millis
    private static final int MOTION_EVENT_TIMEOUT = 1000; // millis

    final List<MotionEvent> mMotionEvents = new ArrayList<>();
    StubGestureAccessibilityService mService;
    MyTouchListener mMyTouchListener = new MyTouchListener();
    MyGestureCallback mCallback;
    TextView mFullScreenTextView;
    Rect mViewBounds = new Rect();
    boolean mGotUpEvent;
    // Without a touch screen, there's no point in testing this feature
    boolean mHasTouchScreen;
    boolean mHasMultiTouch;

    public AccessibilityGestureDispatchTest() {
        super(GestureDispatchActivity.class);
    }

    @Override
    public void setUp() throws Exception {
        super.setUp();

        PackageManager pm = getInstrumentation().getContext().getPackageManager();
        mHasTouchScreen = pm.hasSystemFeature(PackageManager.FEATURE_TOUCHSCREEN)
                || pm.hasSystemFeature(PackageManager.FEATURE_FAKETOUCH);
        if (!mHasTouchScreen) {
            return;
        }

        mHasMultiTouch = pm.hasSystemFeature(PackageManager.FEATURE_TOUCHSCREEN_MULTITOUCH)
                || pm.hasSystemFeature(PackageManager.FEATURE_FAKETOUCH_MULTITOUCH_DISTINCT);

        mFullScreenTextView =
                (TextView) getActivity().findViewById(R.id.full_screen_text_view);
        getInstrumentation().runOnMainSync(() -> {
            mFullScreenTextView.getGlobalVisibleRect(mViewBounds);
            mFullScreenTextView.setOnTouchListener(mMyTouchListener);
        });

        mService = StubGestureAccessibilityService.enableSelf(this);

        mMotionEvents.clear();
        mCallback = new MyGestureCallback();
        mGotUpEvent = false;
    }

    @Override
    public void tearDown() throws Exception {
        if (!mHasTouchScreen) {
            return;
        }

        mService.runOnServiceSync(() -> mService.disableSelf());
        super.tearDown();
    }

    public void testClickAt_producesDownThenUp() throws InterruptedException {
        if (!mHasTouchScreen) {
            return;
        }

        final int clickXInsideView = 10;
        final int clickYInsideView = 20;
        int clickX = clickXInsideView + mViewBounds.left;
        int clickY = clickYInsideView + mViewBounds.top;
        GestureDescription click = createClick(clickX, clickY);
        mService.runOnServiceSync(() -> mService.doDispatchGesture(click, mCallback, null));
        mCallback.assertGestureCompletes(GESTURE_COMPLETION_TIMEOUT);
        waitForMotionEvents(2);

        assertEquals(2, mMotionEvents.size());
        MotionEvent clickDown = mMotionEvents.get(0);
        MotionEvent clickUp = mMotionEvents.get(1);

        assertEquals(MotionEvent.ACTION_DOWN, clickDown.getActionMasked());
        assertEquals(0, clickDown.getActionIndex());
        assertEquals(0, clickDown.getDeviceId());
        assertEquals(0, clickDown.getEdgeFlags());
        assertEquals(1F, clickDown.getXPrecision());
        assertEquals(1F, clickDown.getYPrecision());
        assertEquals(1, clickDown.getPointerCount());
        assertEquals(1F, clickDown.getPressure());
        assertEquals((float) clickXInsideView, clickDown.getX());
        assertEquals((float) clickYInsideView, clickDown.getY());
        assertEquals(clickDown.getDownTime(), clickDown.getEventTime());

        assertEquals(MotionEvent.ACTION_UP, clickUp.getActionMasked());
        assertEquals(clickDown.getDownTime(), clickUp.getDownTime());
        assertEquals(ViewConfiguration.getTapTimeout(),
                clickUp.getEventTime() - clickUp.getDownTime());
        assertTrue(clickDown.getEventTime() + ViewConfiguration.getLongPressTimeout()
                > clickUp.getEventTime());
        assertEquals((float) clickXInsideView, clickUp.getX());
        assertEquals((float) clickYInsideView, clickUp.getY());
    }

    public void testLongClickAt_producesEventsWithLongClickTiming() throws InterruptedException {
        if (!mHasTouchScreen) {
            return;
        }

        final int clickXInsideView = 10;
        final int clickYInsideView = 20;
        int clickX = clickXInsideView + mViewBounds.left;
        int clickY = clickYInsideView + mViewBounds.top;
        GestureDescription longClick = createLongClick(clickX, clickY);
        mService.runOnServiceSync(() -> mService.doDispatchGesture(longClick, mCallback, null));
        mCallback.assertGestureCompletes(
                ViewConfiguration.getLongPressTimeout() + GESTURE_COMPLETION_TIMEOUT);

        waitForMotionEvents(2);
        MotionEvent clickDown = mMotionEvents.get(0);
        MotionEvent clickUp = mMotionEvents.get(1);

        assertEquals(MotionEvent.ACTION_DOWN, clickDown.getActionMasked());

        assertEquals((float) clickXInsideView, clickDown.getX());
        assertEquals((float) clickYInsideView, clickDown.getY());

        assertEquals(MotionEvent.ACTION_UP, clickUp.getActionMasked());
        assertTrue(clickDown.getEventTime() + ViewConfiguration.getLongPressTimeout()
                <= clickUp.getEventTime());
        assertEquals(clickDown.getDownTime(), clickUp.getDownTime());
        assertEquals((float) clickXInsideView, clickUp.getX());
        assertEquals((float) clickYInsideView, clickUp.getY());
    }

    public void testSwipe_shouldContainPointsInALine() throws InterruptedException {
        if (!mHasTouchScreen) {
            return;
        }

        int startXInsideView = 10;
        int startYInsideView = 20;
        int endXInsideView = 20;
        int endYInsideView = 40;
        int startX = startXInsideView + mViewBounds.left;
        int startY = startYInsideView + mViewBounds.top;
        int endX = endXInsideView + mViewBounds.left;
        int endY = endYInsideView + mViewBounds.top;
        int gestureTime = 500;
        float swipeTolerance = 2.0f;

        GestureDescription swipe = createSwipe(startX, startY, endX, endY, gestureTime);
        mService.runOnServiceSync(() -> mService.doDispatchGesture(swipe, mCallback, null));
        mCallback.assertGestureCompletes(gestureTime + GESTURE_COMPLETION_TIMEOUT);
        waitForUpEvent();
        int numEvents = mMotionEvents.size();

        MotionEvent downEvent = mMotionEvents.get(0);
        assertEquals(MotionEvent.ACTION_DOWN, downEvent.getActionMasked());
        assertEquals(startXInsideView, (int) downEvent.getX());
        assertEquals(startYInsideView, (int) downEvent.getY());

        MotionEvent upEvent = mMotionEvents.get(numEvents - 1);
        assertEquals(MotionEvent.ACTION_UP, upEvent.getActionMasked());
        assertEquals(endXInsideView, (int) upEvent.getX());
        assertEquals(endYInsideView, (int) upEvent.getY());
        assertEquals(gestureTime, upEvent.getEventTime() - downEvent.getEventTime());

        long lastEventTime = downEvent.getEventTime();
        for (int i = 1; i < numEvents - 1; i++) {
            MotionEvent moveEvent = mMotionEvents.get(i);
            assertEquals(MotionEvent.ACTION_MOVE, moveEvent.getActionMasked());
            assertTrue(moveEvent.getEventTime() >= lastEventTime);
            float fractionOfSwipe =
                    ((float) (moveEvent.getEventTime() - downEvent.getEventTime())) / gestureTime;
            float fractionX = ((float) (endXInsideView - startXInsideView)) * fractionOfSwipe;
            float fractionY = ((float) (endYInsideView - startYInsideView)) * fractionOfSwipe;
            assertEquals(startXInsideView + fractionX, moveEvent.getX(), swipeTolerance);
            assertEquals(startYInsideView + fractionY, moveEvent.getY(), swipeTolerance);
            lastEventTime = moveEvent.getEventTime();
        }
    }

    public void testSlowSwipe_shouldNotContainMovesForTinyMovement() throws InterruptedException {
        if (!mHasTouchScreen) {
            return;
        }

        int startXInsideView = 10;
        int startYInsideView = 20;
        int endXInsideView = 11;
        int endYInsideView = 22;
        int startX = startXInsideView + mViewBounds.left;
        int startY = startYInsideView + mViewBounds.top;
        int endX = endXInsideView + mViewBounds.left;
        int endY = endYInsideView + mViewBounds.top;
        int gestureTime = 1000;

        GestureDescription swipe = createSwipe(startX, startY, endX, endY, gestureTime);
        mService.runOnServiceSync(() -> mService.doDispatchGesture(swipe, mCallback, null));
        mCallback.assertGestureCompletes(gestureTime + GESTURE_COMPLETION_TIMEOUT);
        waitForUpEvent();

        assertEquals(5, mMotionEvents.size());

        assertEquals(MotionEvent.ACTION_DOWN, mMotionEvents.get(0).getActionMasked());
        assertEquals(MotionEvent.ACTION_MOVE, mMotionEvents.get(1).getActionMasked());
        assertEquals(MotionEvent.ACTION_MOVE, mMotionEvents.get(2).getActionMasked());
        assertEquals(MotionEvent.ACTION_MOVE, mMotionEvents.get(3).getActionMasked());
        assertEquals(MotionEvent.ACTION_UP, mMotionEvents.get(4).getActionMasked());

        assertEquals(startXInsideView, (int) mMotionEvents.get(0).getX());
        assertEquals(startXInsideView, (int) mMotionEvents.get(1).getX());
        assertEquals(startXInsideView + 1, (int) mMotionEvents.get(2).getX());
        assertEquals(startXInsideView + 1, (int) mMotionEvents.get(3).getX());
        assertEquals(startXInsideView + 1, (int) mMotionEvents.get(4).getX());

        assertEquals(startYInsideView, (int) mMotionEvents.get(0).getY());
        assertEquals(startYInsideView + 1, (int) mMotionEvents.get(1).getY());
        assertEquals(startYInsideView + 1, (int) mMotionEvents.get(2).getY());
        assertEquals(startYInsideView + 2, (int) mMotionEvents.get(3).getY());
        assertEquals(startYInsideView + 2, (int) mMotionEvents.get(4).getY());
    }

    public void testAngledPinch_looksReasonable() throws InterruptedException {
        if (!(mHasTouchScreen && mHasMultiTouch)) {
            return;
        }

        int centerXInsideView = 50;
        int centerYInsideView = 60;
        int centerX = centerXInsideView + mViewBounds.left;
        int centerY = centerYInsideView + mViewBounds.top;
        int startSpacing = 100;
        int endSpacing = 50;
        int gestureTime = 500;
        float pinchTolerance = 2.0f;

        GestureDescription pinch = createPinch(centerX, centerY, startSpacing,
                endSpacing, 45.0F, gestureTime);
        mService.runOnServiceSync(() -> mService.doDispatchGesture(pinch, mCallback, null));
        mCallback.assertGestureCompletes(gestureTime + GESTURE_COMPLETION_TIMEOUT);
        waitForUpEvent();
        int numEvents = mMotionEvents.size();

        // First two events are the initial down and the pointer down
        assertEquals(MotionEvent.ACTION_DOWN, mMotionEvents.get(0).getActionMasked());
        assertEquals(MotionEvent.ACTION_POINTER_DOWN, mMotionEvents.get(1).getActionMasked());

        // The second event must have two pointers at the initial spacing along a 45 degree angle
        MotionEvent firstEventWithTwoPointers = mMotionEvents.get(1);
        assertEquals(2, firstEventWithTwoPointers.getPointerCount());
        MotionEvent.PointerCoords coords0 = new MotionEvent.PointerCoords();
        MotionEvent.PointerCoords coords1 = new MotionEvent.PointerCoords();
        firstEventWithTwoPointers.getPointerCoords(0, coords0);
        firstEventWithTwoPointers.getPointerCoords(1, coords1);
        // Verify center point
        assertEquals((float) centerXInsideView, (coords0.x + coords1.x) / 2, pinchTolerance);
        assertEquals((float) centerYInsideView, (coords0.y + coords1.y) / 2, pinchTolerance);
        // Verify angle
        assertEquals(coords0.x - centerXInsideView, coords0.y - centerYInsideView, pinchTolerance);
        assertEquals(coords1.x - centerXInsideView, coords1.y - centerYInsideView, pinchTolerance);
        // Verify spacing
        assertEquals(startSpacing, distance(coords0, coords1), pinchTolerance);

        // The last two events are the pointer up and the final up
        assertEquals(MotionEvent.ACTION_UP, mMotionEvents.get(numEvents - 1).getActionMasked());

        MotionEvent lastEventWithTwoPointers = mMotionEvents.get(numEvents - 2);
        assertEquals(MotionEvent.ACTION_POINTER_UP, lastEventWithTwoPointers.getActionMasked());
        lastEventWithTwoPointers.getPointerCoords(0, coords0);
        lastEventWithTwoPointers.getPointerCoords(1, coords1);
        // Verify center point
        assertEquals((float) centerXInsideView, (coords0.x + coords1.x) / 2, pinchTolerance);
        assertEquals((float) centerYInsideView, (coords0.y + coords1.y) / 2, pinchTolerance);
        // Verify angle
        assertEquals(coords0.x - centerXInsideView, coords0.y - centerYInsideView, pinchTolerance);
        assertEquals(coords1.x - centerXInsideView, coords1.y - centerYInsideView, pinchTolerance);
        // Verify spacing
        assertEquals(endSpacing, distance(coords0, coords1), pinchTolerance);

        float lastSpacing = startSpacing;
        for (int i = 2; i < numEvents - 2; i++) {
            MotionEvent eventInMiddle = mMotionEvents.get(i);
            assertEquals(MotionEvent.ACTION_MOVE, eventInMiddle.getActionMasked());
            eventInMiddle.getPointerCoords(0, coords0);
            eventInMiddle.getPointerCoords(1, coords1);
            // Verify center point
            assertEquals((float) centerXInsideView, (coords0.x + coords1.x) / 2, pinchTolerance);
            assertEquals((float) centerYInsideView, (coords0.y + coords1.y) / 2, pinchTolerance);
            // Verify angle
            assertEquals(coords0.x - centerXInsideView, coords0.y - centerYInsideView,
                    pinchTolerance);
            assertEquals(coords1.x - centerXInsideView, coords1.y - centerYInsideView,
                    pinchTolerance);
            float spacing = distance(coords0, coords1);
            assertTrue(spacing <= lastSpacing + pinchTolerance);
            assertTrue(spacing >= endSpacing - pinchTolerance);
            lastSpacing = spacing;
        }
    }

    public void testClickWhenMagnified_matchesActualTouch() throws InterruptedException {
        if (!mHasTouchScreen) {
            return;
        }

        final int clickXInsideView = 10;
        final int clickYInsideView = 20;
        int clickX = clickXInsideView + mViewBounds.left;
        int clickY = clickYInsideView + mViewBounds.top;
        final float TOUCH_TOLERANCE = 2.0f;

        StubMagnificationAccessibilityService magnificationService =
                StubMagnificationAccessibilityService.enableSelf(this);
        android.accessibilityservice.AccessibilityService.MagnificationController
                magnificationController = magnificationService.getMagnificationController();
        final Resources res = getInstrumentation().getTargetContext().getResources();
        final DisplayMetrics metrics = res.getDisplayMetrics();
        try {
            // Magnify screen by 2x from upper left corner
            final AtomicBoolean setScale = new AtomicBoolean();
            final float magnificationFactor = 2.0f;
            // Center to have (0,0) in the upper-left corner
            final float centerX = metrics.widthPixels / (2.0f * magnificationFactor) - 1.0f;
            final float centerY = metrics.heightPixels / (2.0f * magnificationFactor) - 1.0f;
            magnificationService.runOnServiceSync(() -> {
                        setScale.set(magnificationController.setScale(magnificationFactor, false));
                        // Make sure the upper right corner is on the screen
                        magnificationController.setCenter(centerX, centerY, false);
                    });
            assertTrue("Failed to set scale", setScale.get());

            GestureDescription click = createClick((int) (clickX * magnificationFactor),
                    (int) (clickY * magnificationFactor));
            mService.runOnServiceSync(() -> mService.doDispatchGesture(click, mCallback, null));
            mCallback.assertGestureCompletes(GESTURE_COMPLETION_TIMEOUT);
            waitForMotionEvents(3);
        } finally {
            // Reset magnification
            final AtomicBoolean result = new AtomicBoolean();
            magnificationService.runOnServiceSync(() ->
                    result.set(magnificationController.reset(false)));
            magnificationService.runOnServiceSync(() -> magnificationService.disableSelf());
            assertTrue("Failed to reset", result.get());
        }

        assertEquals(2, mMotionEvents.size());
        MotionEvent clickDown = mMotionEvents.get(0);
        MotionEvent clickUp = mMotionEvents.get(1);

        assertEquals(MotionEvent.ACTION_DOWN, clickDown.getActionMasked());
        assertEquals((float) clickXInsideView, clickDown.getX(), TOUCH_TOLERANCE);
        assertEquals((float) clickYInsideView, clickDown.getY(), TOUCH_TOLERANCE);
        assertEquals(clickDown.getDownTime(), clickDown.getEventTime());

        assertEquals(MotionEvent.ACTION_UP, clickUp.getActionMasked());
        assertEquals((float) clickXInsideView, clickUp.getX(), TOUCH_TOLERANCE);
        assertEquals((float) clickYInsideView, clickUp.getY(), TOUCH_TOLERANCE);
    }


    public static class GestureDispatchActivity extends AccessibilityTestActivity {
        public GestureDispatchActivity() {
            super();
        }

        @Override
        public void onCreate(Bundle savedInstanceState) {
            super.onCreate(savedInstanceState);
            setContentView(R.layout.full_screen_frame_layout);
        }
    }

    public static class MyGestureCallback extends AccessibilityService.GestureResultCallback {
        private boolean mCompleted;
        private boolean mCancelled;

        @Override
        public synchronized void onCompleted(GestureDescription gestureDescription) {
            mCompleted = true;
            notifyAll();
        }

        @Override
        public synchronized void onCancelled(GestureDescription gestureDescription) {
            mCancelled = true;
            notifyAll();
        }

        public synchronized void assertGestureCompletes(long timeout) {
            if (mCompleted) {
                return;
            }
            try {
                wait(timeout);
            } catch (InterruptedException e) {
                throw new RuntimeException(e);
            }
            assertTrue("Gesture did not complete.", mCompleted);
        }
    }

    private void waitForMotionEvents(int numEventsExpected) throws InterruptedException {
        synchronized (mMotionEvents) {
            long endMillis = SystemClock.uptimeMillis() + MOTION_EVENT_TIMEOUT;
            while ((mMotionEvents.size() < numEventsExpected)
                    && (SystemClock.uptimeMillis() < endMillis)) {
                mMotionEvents.wait(endMillis - SystemClock.uptimeMillis());
            }
        }
    }

    private void waitForUpEvent() throws InterruptedException {
        synchronized (mMotionEvents) {
            long endMillis = SystemClock.uptimeMillis() + MOTION_EVENT_TIMEOUT;
            while (!mGotUpEvent && (SystemClock.uptimeMillis() < endMillis)) {
                mMotionEvents.wait(endMillis - SystemClock.uptimeMillis());
            }
        }
    }

    private float distance(MotionEvent.PointerCoords point1, MotionEvent.PointerCoords point2) {
        return (float) Math.hypot((double) (point1.x - point2.x), (double) (point1.y - point2.y));
    }

    private class MyTouchListener implements View.OnTouchListener {
        @Override
        public boolean onTouch(View view, MotionEvent motionEvent) {
            synchronized (mMotionEvents) {
                if (motionEvent.getActionMasked() == MotionEvent.ACTION_UP) {
                    mGotUpEvent = true;
                }
                mMotionEvents.add(MotionEvent.obtain(motionEvent));
                mMotionEvents.notifyAll();
                return true;
            }
        }
    }

    private GestureDescription createClick(int x, int y) {
        Path clickPath = new Path();
        clickPath.moveTo(x, y);
        GestureDescription.StrokeDescription clickStroke =
                new GestureDescription.StrokeDescription(clickPath, 0, ViewConfiguration.getTapTimeout());
        GestureDescription.Builder clickBuilder = new GestureDescription.Builder();
        clickBuilder.addStroke(clickStroke);
        return clickBuilder.build();
    }

    private GestureDescription createLongClick(int x, int y) {
        Path clickPath = new Path();
        clickPath.moveTo(x, y);
        int longPressTime = ViewConfiguration.getLongPressTimeout();

        GestureDescription.StrokeDescription longClickStroke =
                new GestureDescription.StrokeDescription(clickPath, 0, longPressTime + (longPressTime / 2));
        GestureDescription.Builder longClickBuilder = new GestureDescription.Builder();
        longClickBuilder.addStroke(longClickStroke);
        return longClickBuilder.build();
    }

    private GestureDescription createSwipe(
            int startX, int startY, int endX, int endY, long duration) {
        Path swipePath = new Path();
        swipePath.moveTo(startX, startY);
        swipePath.lineTo(endX, endY);

        GestureDescription.StrokeDescription swipeStroke = new GestureDescription.StrokeDescription(swipePath, 0, duration);
        GestureDescription.Builder swipeBuilder = new GestureDescription.Builder();
        swipeBuilder.addStroke(swipeStroke);
        return swipeBuilder.build();
    }

    private GestureDescription createPinch(int centerX, int centerY, int startSpacing,
            int endSpacing, float orientation, long duration) {
        if ((startSpacing < 0) || (endSpacing < 0)) {
            throw new IllegalArgumentException("Pinch spacing cannot be negative");
        }
        float[] startPoint1 = new float[2];
        float[] endPoint1 = new float[2];
        float[] startPoint2 = new float[2];
        float[] endPoint2 = new float[2];

        /* Build points for a horizontal gesture centered at the origin */
        startPoint1[0] = startSpacing / 2;
        startPoint1[1] = 0;
        endPoint1[0] = endSpacing / 2;
        endPoint1[1] = 0;
        startPoint2[0] = -startSpacing / 2;
        startPoint2[1] = 0;
        endPoint2[0] = -endSpacing / 2;
        endPoint2[1] = 0;

        /* Rotate and translate the points */
        Matrix matrix = new Matrix();
        matrix.setRotate(orientation);
        matrix.postTranslate(centerX, centerY);
        matrix.mapPoints(startPoint1);
        matrix.mapPoints(endPoint1);
        matrix.mapPoints(startPoint2);
        matrix.mapPoints(endPoint2);

        Path path1 = new Path();
        path1.moveTo(startPoint1[0], startPoint1[1]);
        path1.lineTo(endPoint1[0], endPoint1[1]);
        Path path2 = new Path();
        path2.moveTo(startPoint2[0], startPoint2[1]);
        path2.lineTo(endPoint2[0], endPoint2[1]);

        GestureDescription.StrokeDescription path1Stroke = new GestureDescription.StrokeDescription(path1, 0, duration);
        GestureDescription.StrokeDescription path2Stroke = new GestureDescription.StrokeDescription(path2, 0, duration);
        GestureDescription.Builder swipeBuilder = new GestureDescription.Builder();
        swipeBuilder.addStroke(path1Stroke);
        swipeBuilder.addStroke(path2Stroke);
        return swipeBuilder.build();
    }
}
