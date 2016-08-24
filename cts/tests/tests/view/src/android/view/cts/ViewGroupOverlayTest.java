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

import android.app.Instrumentation;
import android.content.Context;
import android.graphics.Color;
import android.graphics.Rect;
import android.graphics.drawable.ColorDrawable;
import android.graphics.drawable.Drawable;
import android.os.SystemClock;
import android.platform.test.annotations.Presubmit;
import android.test.ActivityInstrumentationTestCase2;
import android.test.suitebuilder.annotation.SmallTest;
import android.util.Pair;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewGroupOverlay;
import android.view.cts.util.DrawingUtils;

import java.util.ArrayList;
import java.util.List;

import static org.mockito.Mockito.*;

@SmallTest
public class ViewGroupOverlayTest extends
        ActivityInstrumentationTestCase2<ViewGroupOverlayCtsActivity> {
    private ViewGroup mViewGroupWithOverlay;
    private ViewGroupOverlay mViewGroupOverlay;
    private Context mContext;

    public ViewGroupOverlayTest() {
        super("android.view.cts", ViewGroupOverlayCtsActivity.class);
    }

    @Override
    protected void setUp() throws Exception {
        super.setUp();

        mViewGroupWithOverlay = (ViewGroup) getActivity().findViewById(R.id.viewgroup_with_overlay);
        mViewGroupOverlay = mViewGroupWithOverlay.getOverlay();
        mContext = getInstrumentation().getTargetContext();
    }

    @Presubmit
    public void testBasics() {
        DrawingUtils.assertAllPixelsOfColor("Default fill", mViewGroupWithOverlay,
                Color.WHITE, null);
        assertNotNull("Overlay is not null", mViewGroupOverlay);
    }

    public void testAddNullView() throws Throwable {
        try {
            runTestOnUiThread(() -> mViewGroupOverlay.add((View) null));
            fail("should throw IllegalArgumentException");
        } catch (IllegalArgumentException e) {
        }
    }

    public void testRemoveNullView() throws Throwable {
        try {
            runTestOnUiThread(() -> mViewGroupOverlay.remove((View) null));
            fail("should throw IllegalArgumentException");
        } catch (IllegalArgumentException e) {
        }
    }

    public void testOverlayWithOneView() throws Throwable {
        // Add one colored view to the overlay
        final View redView = new View(mContext);
        redView.setBackgroundColor(Color.RED);
        redView.layout(10, 20, 30, 40);

        runTestOnUiThread(() -> mViewGroupOverlay.add(redView));

        final List<Pair<Rect, Integer>> colorRectangles = new ArrayList<>();
        colorRectangles.add(new Pair<>(new Rect(10, 20, 30, 40), Color.RED));
        DrawingUtils.assertAllPixelsOfColor("Overlay with one red view",
                mViewGroupWithOverlay, Color.WHITE, colorRectangles);

        // Now remove that view from the overlay and test that we're back to pure white fill
        runTestOnUiThread(() -> mViewGroupOverlay.remove(redView));
        DrawingUtils.assertAllPixelsOfColor("Back to default fill", mViewGroupWithOverlay,
                Color.WHITE, null);
    }

    public void testOverlayWithNonOverlappingViews() throws Throwable {
        // Add three views to the overlay
        final View redView = new View(mContext);
        redView.setBackgroundColor(Color.RED);
        redView.layout(10, 20, 30, 40);
        final View greenView = new View(mContext);
        greenView.setBackgroundColor(Color.GREEN);
        greenView.layout(60, 30, 90, 50);
        final View blueView = new View(mContext);
        blueView.setBackgroundColor(Color.BLUE);
        blueView.layout(40, 60, 80, 90);

        runTestOnUiThread(
                () -> {
                    mViewGroupOverlay.add(redView);
                    mViewGroupOverlay.add(greenView);
                    mViewGroupOverlay.add(blueView);
                });

        final List<Pair<Rect, Integer>> colorRectangles = new ArrayList<>();
        colorRectangles.add(new Pair<>(new Rect(10, 20, 30, 40), Color.RED));
        colorRectangles.add(new Pair<>(new Rect(60, 30, 90, 50), Color.GREEN));
        colorRectangles.add(new Pair<>(new Rect(40, 60, 80, 90), Color.BLUE));
        DrawingUtils.assertAllPixelsOfColor("Overlay with three views", mViewGroupWithOverlay,
                Color.WHITE, colorRectangles);

        // Remove one of the views from the overlay
        runTestOnUiThread(() -> mViewGroupOverlay.remove(greenView));
        colorRectangles.clear();
        colorRectangles.add(new Pair<>(new Rect(10, 20, 30, 40), Color.RED));
        colorRectangles.add(new Pair<>(new Rect(40, 60, 80, 90), Color.BLUE));
        DrawingUtils.assertAllPixelsOfColor("Overlay with two views", mViewGroupWithOverlay,
                Color.WHITE, colorRectangles);

        // Clear all views from the overlay and test that we're back to pure white fill
        runTestOnUiThread(() -> mViewGroupOverlay.clear());
        DrawingUtils.assertAllPixelsOfColor("Back to default fill", mViewGroupWithOverlay,
                Color.WHITE, null);
    }


    public void testOverlayWithNonOverlappingViewAndDrawable() throws Throwable {
        // Add one view and one drawable to the overlay
        final View redView = new View(mContext);
        redView.setBackgroundColor(Color.RED);
        redView.layout(10, 20, 30, 40);
        final Drawable greenDrawable = new ColorDrawable(Color.GREEN);
        greenDrawable.setBounds(60, 30, 90, 50);

        runTestOnUiThread(
                () -> {
                    mViewGroupOverlay.add(redView);
                    mViewGroupOverlay.add(greenDrawable);
                });

        final List<Pair<Rect, Integer>> colorRectangles = new ArrayList<>();
        colorRectangles.add(new Pair<>(new Rect(10, 20, 30, 40), Color.RED));
        colorRectangles.add(new Pair<>(new Rect(60, 30, 90, 50), Color.GREEN));
        DrawingUtils.assertAllPixelsOfColor("Overlay with one view and one drawable",
                mViewGroupWithOverlay, Color.WHITE, colorRectangles);

        // Remove the view from the overlay
        runTestOnUiThread(() -> mViewGroupOverlay.remove(redView));
        colorRectangles.clear();
        colorRectangles.add(new Pair<>(new Rect(60, 30, 90, 50), Color.GREEN));
        DrawingUtils.assertAllPixelsOfColor("Overlay with one drawable", mViewGroupWithOverlay,
                Color.WHITE, colorRectangles);

        // Clear everything from the overlay and test that we're back to pure white fill
        runTestOnUiThread(() -> mViewGroupOverlay.clear());
        DrawingUtils.assertAllPixelsOfColor("Back to default fill", mViewGroupWithOverlay,
                Color.WHITE, null);
    }

    public void testOverlayWithOverlappingViews() throws Throwable {
        // Add two overlapping colored views to the overlay
        final View redView = new View(mContext);
        redView.setBackgroundColor(Color.RED);
        redView.layout(10, 20, 60, 40);
        final View greenView = new View(mContext);
        greenView.setBackgroundColor(Color.GREEN);
        greenView.layout(30, 20, 80, 40);

        runTestOnUiThread(
                () -> {
                    mViewGroupOverlay.add(redView);
                    mViewGroupOverlay.add(greenView);
                });

        // Our overlay views overlap in horizontal 30-60 range. Here we test that the
        // second view is the one that is drawn last in that range.
        final List<Pair<Rect, Integer>> colorRectangles = new ArrayList<>();
        colorRectangles.add(new Pair<>(new Rect(10, 20, 30, 40), Color.RED));
        colorRectangles.add(new Pair<>(new Rect(30, 20, 80, 40), Color.GREEN));
        DrawingUtils.assertAllPixelsOfColor("Overlay with two drawables", mViewGroupWithOverlay,
                Color.WHITE, colorRectangles);

        // Remove the second view from the overlay
        runTestOnUiThread(() -> mViewGroupOverlay.remove(greenView));
        colorRectangles.clear();
        colorRectangles.add(new Pair<>(new Rect(10, 20, 60, 40), Color.RED));
        DrawingUtils.assertAllPixelsOfColor("Overlay with one drawable", mViewGroupWithOverlay,
                Color.WHITE, colorRectangles);

        // Clear all views from the overlay and test that we're back to pure white fill
        runTestOnUiThread(() -> mViewGroupOverlay.clear());
        DrawingUtils.assertAllPixelsOfColor("Back to default fill", mViewGroupWithOverlay,
                Color.WHITE, null);
    }

    public void testOverlayWithOverlappingViewAndDrawable() throws Throwable {
        // Add two overlapping colored views to the overlay
        final Drawable redDrawable = new ColorDrawable(Color.RED);
        redDrawable.setBounds(10, 20, 60, 40);
        final View greenView = new View(mContext);
        greenView.setBackgroundColor(Color.GREEN);
        greenView.layout(30, 20, 80, 40);

        runTestOnUiThread(
                () -> {
                    mViewGroupOverlay.add(redDrawable);
                    mViewGroupOverlay.add(greenView);
                });

        // Our overlay views overlap in horizontal 30-60 range. Even though the green view was
        // added after the red drawable, *all* overlay drawables are drawn after the overlay views.
        // So in the overlap range we expect color red
        final List<Pair<Rect, Integer>> colorRectangles = new ArrayList<>();
        colorRectangles.add(new Pair<>(new Rect(10, 20, 60, 40), Color.RED));
        colorRectangles.add(new Pair<>(new Rect(60, 20, 80, 40), Color.GREEN));
        DrawingUtils.assertAllPixelsOfColor("Overlay with one view and one drawable",
                mViewGroupWithOverlay, Color.WHITE, colorRectangles);

        // Remove the drawable from the overlay
        runTestOnUiThread(() -> mViewGroupOverlay.remove(redDrawable));
        colorRectangles.clear();
        colorRectangles.add(new Pair<>(new Rect(30, 20, 80, 40), Color.GREEN));
        DrawingUtils.assertAllPixelsOfColor("Overlay with one view", mViewGroupWithOverlay,
                Color.WHITE, colorRectangles);

        // Clear all views from the overlay and test that we're back to pure white fill
        runTestOnUiThread(() -> mViewGroupOverlay.clear());
        DrawingUtils.assertAllPixelsOfColor("Back to default fill", mViewGroupWithOverlay,
                Color.WHITE, null);
    }

    public void testOverlayViewNoClicks() throws Throwable {
        // Add one colored view with mock click listener to the overlay
        final View redView = new View(mContext);
        redView.setBackgroundColor(Color.RED);
        final View.OnClickListener mockClickListener = mock(View.OnClickListener.class);
        redView.setOnClickListener(mockClickListener);
        redView.layout(10, 20, 30, 40);

        runTestOnUiThread(() -> mViewGroupOverlay.add(redView));

        // If we call performClick or dispatchTouchEvent on the view that we've added to the
        // overlay, that will invoke the listener that we've registered. But here we need to
        // test that such a view doesn't get clicks in the regular event processing pipeline
        // that handles individual events at the screen level. Use Instrumentation to emulate
        // the high-level sequence of events instead.
        final int[] viewGroupOnScreenXY = new int[2];
        mViewGroupWithOverlay.getLocationOnScreen(viewGroupOnScreenXY);

        // Compute the coordinates for emulating a tap in the center of the view we've added
        // to the overlay.
        final int emulatedTapX = viewGroupOnScreenXY[0] + redView.getLeft()
                + redView.getWidth() / 2;
        final int emulatedTapY = viewGroupOnScreenXY[1] + redView.getTop()
                + redView.getHeight() / 2;
        final Instrumentation instrumentation = getInstrumentation();

        // Inject DOWN event
        long downTime = SystemClock.uptimeMillis();
        MotionEvent eventDown = MotionEvent.obtain(
                downTime, downTime, MotionEvent.ACTION_DOWN, emulatedTapX, emulatedTapY, 1);
        instrumentation.sendPointerSync(eventDown);

        // Inject MOVE event
        long moveTime = SystemClock.uptimeMillis();
        MotionEvent eventMove = MotionEvent.obtain(
                moveTime, moveTime, MotionEvent.ACTION_MOVE, emulatedTapX, emulatedTapY, 1);
        instrumentation.sendPointerSync(eventMove);

        // Inject UP event
        long upTime = SystemClock.uptimeMillis();
        MotionEvent eventUp = MotionEvent.obtain(
                upTime, upTime, MotionEvent.ACTION_UP, emulatedTapX, emulatedTapY, 1);
        instrumentation.sendPointerSync(eventUp);

        // Wait for the system to process all events in the queue
        instrumentation.waitForIdleSync();

        // Verify that our mock listener hasn't been called
        verify(mockClickListener, never()).onClick(any(View.class));
    }

    public void testOverlayReparenting() throws Throwable {
        // Find the view that we're about to add to our overlay
        final View level2View = mViewGroupWithOverlay.findViewById(R.id.level2);
        final View level3View = level2View.findViewById(R.id.level3);

        assertTrue(level2View == level3View.getParent());

        // Set the fill of this view to red
        runTestOnUiThread(
                () -> {
                    level3View.setBackgroundColor(Color.RED);
                    mViewGroupOverlay.add(level3View);
                });

        // At this point we expect the view that was added to the overlay to have been removed
        // from its original parent
        assertFalse(level2View == level3View.getParent());

        // Check the expected visual appearance of our view group. We expect that the view that
        // was added to the overlay to maintain its relative location inside the activity.
        final List<Pair<Rect, Integer>> colorRectangles = new ArrayList<>();
        colorRectangles.add(new Pair<>(new Rect(65, 60, 85, 90), Color.RED));
        DrawingUtils.assertAllPixelsOfColor("Empty overlay before adding grandchild",
                mViewGroupWithOverlay, Color.WHITE, colorRectangles);

    }
}
