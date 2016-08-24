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

package android.widget.cts;

import android.app.Activity;
import android.app.Instrumentation;
import android.content.Context;
import android.graphics.Color;
import android.graphics.Rect;
import android.graphics.drawable.ColorDrawable;
import android.graphics.drawable.Drawable;
import android.os.SystemClock;
import android.test.ActivityInstrumentationTestCase2;
import android.test.UiThreadTest;
import android.transition.Transition;
import android.transition.Transition.TransitionListener;
import android.transition.TransitionValues;
import android.util.AttributeSet;
import android.view.Display;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.OnTouchListener;
import android.view.ViewGroup;
import android.view.ViewGroup.LayoutParams;
import android.view.WindowManager;
import android.widget.ImageView;
import android.widget.PopupWindow;
import android.widget.PopupWindow.OnDismissListener;
import android.widget.TextView;
import android.widget.cts.R;

import static org.mockito.Mockito.any;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

public class PopupWindowTest extends
        ActivityInstrumentationTestCase2<PopupWindowCtsActivity> {
    private Instrumentation mInstrumentation;
    private Activity mActivity;
    /** The popup window. */
    private PopupWindow mPopupWindow;

    /**
     * Instantiates a new popup window test.
     */
    public PopupWindowTest() {
        super("android.widget.cts", PopupWindowCtsActivity.class);
    }

    /*
     * (non-Javadoc)
     *
     * @see android.test.ActivityInstrumentationTestCase#setUp()
     */
    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mInstrumentation = getInstrumentation();
        mActivity = getActivity();
    }

    public void testConstructor() {
        new PopupWindow(mActivity);

        new PopupWindow(mActivity, null);

        new PopupWindow(mActivity, null, android.R.attr.popupWindowStyle);

        mPopupWindow = new PopupWindow();
        assertEquals(0, mPopupWindow.getWidth());
        assertEquals(0, mPopupWindow.getHeight());

        mPopupWindow = new PopupWindow(50, 50);
        assertEquals(50, mPopupWindow.getWidth());
        assertEquals(50, mPopupWindow.getHeight());

        mPopupWindow = new PopupWindow(-1, -1);
        assertEquals(-1, mPopupWindow.getWidth());
        assertEquals(-1, mPopupWindow.getHeight());

        TextView contentView = new TextView(mActivity);
        mPopupWindow = new PopupWindow(contentView);
        assertSame(contentView, mPopupWindow.getContentView());

        mPopupWindow = new PopupWindow(contentView, 0, 0);
        assertEquals(0, mPopupWindow.getWidth());
        assertEquals(0, mPopupWindow.getHeight());
        assertSame(contentView, mPopupWindow.getContentView());

        mPopupWindow = new PopupWindow(contentView, 50, 50);
        assertEquals(50, mPopupWindow.getWidth());
        assertEquals(50, mPopupWindow.getHeight());
        assertSame(contentView, mPopupWindow.getContentView());

        mPopupWindow = new PopupWindow(contentView, -1, -1);
        assertEquals(-1, mPopupWindow.getWidth());
        assertEquals(-1, mPopupWindow.getHeight());
        assertSame(contentView, mPopupWindow.getContentView());

        mPopupWindow = new PopupWindow(contentView, 0, 0, true);
        assertEquals(0, mPopupWindow.getWidth());
        assertEquals(0, mPopupWindow.getHeight());
        assertSame(contentView, mPopupWindow.getContentView());
        assertTrue(mPopupWindow.isFocusable());

        mPopupWindow = new PopupWindow(contentView, 50, 50, false);
        assertEquals(50, mPopupWindow.getWidth());
        assertEquals(50, mPopupWindow.getHeight());
        assertSame(contentView, mPopupWindow.getContentView());
        assertFalse(mPopupWindow.isFocusable());

        mPopupWindow = new PopupWindow(contentView, -1, -1, true);
        assertEquals(-1, mPopupWindow.getWidth());
        assertEquals(-1, mPopupWindow.getHeight());
        assertSame(contentView, mPopupWindow.getContentView());
        assertTrue(mPopupWindow.isFocusable());
    }

    public void testAccessEnterExitTransitions() {
        PopupWindow w;

        w = new PopupWindow(mActivity, null, 0, 0);
        assertNull(w.getEnterTransition());
        assertNull(w.getExitTransition());

        w = new PopupWindow(mActivity, null, 0, R.style.PopupWindow_NullTransitions);
        assertNull(w.getEnterTransition());
        assertNull(w.getExitTransition());

        w = new PopupWindow(mActivity, null, 0, R.style.PopupWindow_CustomTransitions);
        assertTrue(w.getEnterTransition() instanceof CustomTransition);
        assertTrue(w.getExitTransition() instanceof CustomTransition);

        Transition enterTransition = new CustomTransition();
        Transition exitTransition = new CustomTransition();
        w = new PopupWindow(mActivity, null, 0, 0);
        w.setEnterTransition(enterTransition);
        w.setExitTransition(exitTransition);
        assertEquals(enterTransition, w.getEnterTransition());
        assertEquals(exitTransition, w.getExitTransition());

        w.setEnterTransition(null);
        w.setExitTransition(null);
        assertNull(w.getEnterTransition());
        assertNull(w.getExitTransition());
    }

    public static class CustomTransition extends Transition {
        public CustomTransition() {
        }

        // This constructor is needed for reflection-based creation of a transition when
        // the transition is defined in layout XML via attribute.
        @SuppressWarnings("unused")
        public CustomTransition(Context context, AttributeSet attrs) {
            super(context, attrs);
        }

        @Override
        public void captureStartValues(TransitionValues transitionValues) {}

        @Override
        public void captureEndValues(TransitionValues transitionValues) {}
    }

    public void testAccessBackground() {
        mPopupWindow = new PopupWindow(mActivity);

        Drawable drawable = new ColorDrawable();
        mPopupWindow.setBackgroundDrawable(drawable);
        assertSame(drawable, mPopupWindow.getBackground());

        mPopupWindow.setBackgroundDrawable(null);
        assertNull(mPopupWindow.getBackground());
    }

    public void testAccessAnimationStyle() {
        mPopupWindow = new PopupWindow(mActivity);
        // default is -1
        assertEquals(-1, mPopupWindow.getAnimationStyle());

        mPopupWindow.setAnimationStyle(android.R.style.Animation_Toast);
        assertEquals(android.R.style.Animation_Toast,
                mPopupWindow.getAnimationStyle());

        // abnormal values
        mPopupWindow.setAnimationStyle(-100);
        assertEquals(-100, mPopupWindow.getAnimationStyle());
    }

    public void testAccessContentView() {
        mPopupWindow = new PopupWindow(mActivity);
        assertNull(mPopupWindow.getContentView());

        View view = new TextView(mActivity);
        mPopupWindow.setContentView(view);
        assertSame(view, mPopupWindow.getContentView());

        mPopupWindow.setContentView(null);
        assertNull(mPopupWindow.getContentView());

        // can not set the content if the old content is shown
        mPopupWindow.setContentView(view);
        assertFalse(mPopupWindow.isShowing());
        showPopup();
        ImageView img = new ImageView(mActivity);
        assertTrue(mPopupWindow.isShowing());
        mPopupWindow.setContentView(img);
        assertSame(view, mPopupWindow.getContentView());
        dismissPopup();
    }

    public void testAccessFocusable() {
        mPopupWindow = new PopupWindow(mActivity);
        assertFalse(mPopupWindow.isFocusable());

        mPopupWindow.setFocusable(true);
        assertTrue(mPopupWindow.isFocusable());

        mPopupWindow.setFocusable(false);
        assertFalse(mPopupWindow.isFocusable());
    }

    public void testAccessHeight() {
        mPopupWindow = new PopupWindow(mActivity);
        assertEquals(WindowManager.LayoutParams.WRAP_CONTENT, mPopupWindow.getHeight());

        int height = getDisplay().getHeight() / 2;
        mPopupWindow.setHeight(height);
        assertEquals(height, mPopupWindow.getHeight());

        height = getDisplay().getHeight();
        mPopupWindow.setHeight(height);
        assertEquals(height, mPopupWindow.getHeight());

        mPopupWindow.setHeight(0);
        assertEquals(0, mPopupWindow.getHeight());

        height = getDisplay().getHeight() * 2;
        mPopupWindow.setHeight(height);
        assertEquals(height, mPopupWindow.getHeight());

        height = -getDisplay().getHeight() / 2;
        mPopupWindow.setHeight(height);
        assertEquals(height, mPopupWindow.getHeight());
    }

    /**
     * Gets the display.
     *
     * @return the display
     */
    private Display getDisplay() {
        WindowManager wm = (WindowManager) mActivity.getSystemService(Context.WINDOW_SERVICE);
        return wm.getDefaultDisplay();
    }

    public void testAccessWidth() {
        mPopupWindow = new PopupWindow(mActivity);
        assertEquals(WindowManager.LayoutParams.WRAP_CONTENT, mPopupWindow.getWidth());

        int width = getDisplay().getWidth() / 2;
        mPopupWindow.setWidth(width);
        assertEquals(width, mPopupWindow.getWidth());

        width = getDisplay().getWidth();
        mPopupWindow.setWidth(width);
        assertEquals(width, mPopupWindow.getWidth());

        mPopupWindow.setWidth(0);
        assertEquals(0, mPopupWindow.getWidth());

        width = getDisplay().getWidth() * 2;
        mPopupWindow.setWidth(width);
        assertEquals(width, mPopupWindow.getWidth());

        width = - getDisplay().getWidth() / 2;
        mPopupWindow.setWidth(width);
        assertEquals(width, mPopupWindow.getWidth());
    }

    private static final int TOP = 0x00;
    private static final int BOTTOM = 0x01;

    private static final int LEFT = 0x00;
    private static final int RIGHT = 0x01;

    private static final int GREATER_THAN = 1;
    private static final int LESS_THAN = -1;
    private static final int EQUAL_TO = 0;

    public void testShowAsDropDown() {
        final PopupWindow popup = createPopupWindow(createPopupContent(50, 50));
        popup.setClipToScreenEnabled(false);
        popup.setOverlapAnchor(false);
        popup.setAnimationStyle(0);
        popup.setExitTransition(null);
        popup.setEnterTransition(null);

        assertPosition(popup, R.id.anchor_upper_left,
                LEFT, EQUAL_TO, LEFT, TOP, EQUAL_TO, BOTTOM);
        assertPosition(popup, R.id.anchor_upper,
                LEFT, EQUAL_TO, LEFT, TOP, EQUAL_TO, BOTTOM);
        assertPosition(popup, R.id.anchor_upper_right,
                RIGHT, EQUAL_TO, RIGHT, TOP, EQUAL_TO, BOTTOM);

        assertPosition(popup, R.id.anchor_middle_left,
                LEFT, EQUAL_TO, LEFT, TOP, EQUAL_TO, BOTTOM);
        assertPosition(popup, R.id.anchor_middle,
                LEFT, EQUAL_TO, LEFT, TOP, EQUAL_TO, BOTTOM);
        assertPosition(popup, R.id.anchor_middle_right,
                RIGHT, EQUAL_TO, RIGHT, TOP, EQUAL_TO, BOTTOM);

        assertPosition(popup, R.id.anchor_lower_left,
                LEFT, EQUAL_TO, LEFT, BOTTOM, EQUAL_TO, TOP);
        assertPosition(popup, R.id.anchor_lower,
                LEFT, EQUAL_TO, LEFT, BOTTOM, EQUAL_TO, TOP);
        assertPosition(popup, R.id.anchor_lower_right,
                RIGHT, EQUAL_TO, RIGHT, BOTTOM, EQUAL_TO, TOP);
    }

    public void testShowAsDropDown_ClipToScreen() {
        final PopupWindow popup = createPopupWindow(createPopupContent(50, 50));
        popup.setClipToScreenEnabled(true);
        popup.setOverlapAnchor(false);
        popup.setAnimationStyle(0);
        popup.setExitTransition(null);
        popup.setEnterTransition(null);

        assertPosition(popup, R.id.anchor_upper_left,
                LEFT, EQUAL_TO, LEFT, TOP, EQUAL_TO, BOTTOM);
        assertPosition(popup, R.id.anchor_upper,
                LEFT, EQUAL_TO, LEFT, TOP, EQUAL_TO, BOTTOM);
        assertPosition(popup, R.id.anchor_upper_right,
                RIGHT, EQUAL_TO, RIGHT, TOP, EQUAL_TO, BOTTOM);

        assertPosition(popup, R.id.anchor_middle_left,
                LEFT, EQUAL_TO, LEFT, TOP, EQUAL_TO, BOTTOM);
        assertPosition(popup, R.id.anchor_middle,
                LEFT, EQUAL_TO, LEFT, TOP, EQUAL_TO, BOTTOM);
        assertPosition(popup, R.id.anchor_middle_right,
                RIGHT, EQUAL_TO, RIGHT, TOP, EQUAL_TO, BOTTOM);

        assertPosition(popup, R.id.anchor_lower_left,
                LEFT, EQUAL_TO, LEFT, BOTTOM, EQUAL_TO, TOP);
        assertPosition(popup, R.id.anchor_lower,
                LEFT, EQUAL_TO, LEFT, BOTTOM, EQUAL_TO, TOP);
        assertPosition(popup, R.id.anchor_lower_right,
                RIGHT, EQUAL_TO, RIGHT, BOTTOM, EQUAL_TO, TOP);
    }

    public void testShowAsDropDown_ClipToScreen_Overlap() {
        final PopupWindow popup = createPopupWindow(createPopupContent(50, 50));
        popup.setClipToScreenEnabled(true);
        popup.setOverlapAnchor(true);
        popup.setAnimationStyle(0);
        popup.setExitTransition(null);
        popup.setEnterTransition(null);

        assertPosition(popup, R.id.anchor_upper_left,
                LEFT, EQUAL_TO, LEFT, TOP, EQUAL_TO, TOP);
        assertPosition(popup, R.id.anchor_upper,
                LEFT, EQUAL_TO, LEFT, TOP, EQUAL_TO, TOP);
        assertPosition(popup, R.id.anchor_upper_right,
                RIGHT, EQUAL_TO, RIGHT, TOP, EQUAL_TO, TOP);

        assertPosition(popup, R.id.anchor_middle_left,
                LEFT, EQUAL_TO, LEFT, TOP, EQUAL_TO, TOP);
        assertPosition(popup, R.id.anchor_middle,
                LEFT, EQUAL_TO, LEFT, TOP, EQUAL_TO, TOP);
        assertPosition(popup, R.id.anchor_middle_right,
                RIGHT, EQUAL_TO, RIGHT, TOP, EQUAL_TO, TOP);

        assertPosition(popup, R.id.anchor_lower_left,
                LEFT, EQUAL_TO, LEFT, BOTTOM, EQUAL_TO, TOP);
        assertPosition(popup, R.id.anchor_lower,
                LEFT, EQUAL_TO, LEFT, BOTTOM, EQUAL_TO, TOP);
        assertPosition(popup, R.id.anchor_lower_right,
                RIGHT, EQUAL_TO, RIGHT, BOTTOM, EQUAL_TO, TOP);
    }

    public void testShowAsDropDown_ClipToScreen_Overlap_Offset() {
        final PopupWindow popup = createPopupWindow(createPopupContent(50, 50));
        popup.setClipToScreenEnabled(true);
        popup.setOverlapAnchor(true);
        popup.setAnimationStyle(0);
        popup.setExitTransition(null);
        popup.setEnterTransition(null);

        final int offsetX = mActivity.findViewById(R.id.anchor_upper).getWidth() / 2;
        final int offsetY = mActivity.findViewById(R.id.anchor_upper).getHeight() / 2;
        final int gravity = Gravity.TOP | Gravity.START;

        assertPosition(popup, R.id.anchor_upper_left,
                LEFT, GREATER_THAN, LEFT, TOP, GREATER_THAN, TOP,
                offsetX, offsetY, gravity);
        assertPosition(popup, R.id.anchor_upper,
                LEFT, GREATER_THAN, LEFT, TOP, GREATER_THAN, TOP,
                offsetX, offsetY, gravity);
        assertPosition(popup, R.id.anchor_upper_right,
                RIGHT, EQUAL_TO, RIGHT, TOP, GREATER_THAN, TOP,
                offsetX, offsetY, gravity);

        assertPosition(popup, R.id.anchor_middle_left,
                LEFT, GREATER_THAN, LEFT, TOP, GREATER_THAN, TOP,
                offsetX, offsetY, gravity);
        assertPosition(popup, R.id.anchor_middle,
                LEFT, GREATER_THAN, LEFT, TOP, GREATER_THAN, TOP,
                offsetX, offsetY, gravity);
        assertPosition(popup, R.id.anchor_middle_right,
                RIGHT, EQUAL_TO, RIGHT, TOP, GREATER_THAN, TOP,
                offsetX, offsetY, gravity);

        assertPosition(popup, R.id.anchor_lower_left,
                LEFT, GREATER_THAN, LEFT, BOTTOM, LESS_THAN, BOTTOM,
                offsetX, offsetY, gravity);
        assertPosition(popup, R.id.anchor_lower,
                LEFT, GREATER_THAN, LEFT, BOTTOM, LESS_THAN, BOTTOM,
                offsetX, offsetY, gravity);
        assertPosition(popup, R.id.anchor_lower_right,
                RIGHT, EQUAL_TO, RIGHT, BOTTOM, LESS_THAN, BOTTOM,
                offsetX, offsetY, gravity);
    }

    public void testShowAsDropDown_ClipToScreen_TooBig() {
        final View rootView = mActivity.findViewById(R.id.anchor_upper_left).getRootView();
        final int width = rootView.getWidth() * 2;
        final int height = rootView.getHeight() * 2;

        final PopupWindow popup = createPopupWindow(createPopupContent(width, height));
        popup.setWidth(width);
        popup.setHeight(height);

        popup.setClipToScreenEnabled(true);
        popup.setOverlapAnchor(false);
        popup.setAnimationStyle(0);
        popup.setExitTransition(null);
        popup.setEnterTransition(null);

        assertPosition(popup, R.id.anchor_upper_left,
                LEFT, EQUAL_TO, LEFT, TOP, LESS_THAN, TOP);
        assertPosition(popup, R.id.anchor_upper,
                LEFT, LESS_THAN, LEFT, TOP, LESS_THAN, TOP);
        assertPosition(popup, R.id.anchor_upper_right,
                RIGHT, EQUAL_TO, RIGHT, TOP, LESS_THAN, TOP);

        assertPosition(popup, R.id.anchor_middle_left,
                LEFT, EQUAL_TO, LEFT, TOP, LESS_THAN, TOP);
        assertPosition(popup, R.id.anchor_middle,
                LEFT, LESS_THAN, LEFT, TOP, LESS_THAN, TOP);
        assertPosition(popup, R.id.anchor_middle_right,
                RIGHT, EQUAL_TO, RIGHT, TOP, LESS_THAN, TOP);

        assertPosition(popup, R.id.anchor_lower_left,
                LEFT, EQUAL_TO, LEFT, BOTTOM, EQUAL_TO, BOTTOM);
        assertPosition(popup, R.id.anchor_lower,
                LEFT, LESS_THAN, LEFT, BOTTOM, EQUAL_TO, BOTTOM);
        assertPosition(popup, R.id.anchor_lower_right,
                RIGHT, EQUAL_TO, RIGHT, BOTTOM, EQUAL_TO, BOTTOM);
    }

    private void assertPosition(PopupWindow popup, int anchorId,
            int contentEdgeX, int operatorX, int anchorEdgeX,
            int contentEdgeY, int operatorY, int anchorEdgeY) {
        assertPosition(popup, anchorId,
                contentEdgeX, operatorX, anchorEdgeX,
                contentEdgeY, operatorY, anchorEdgeY,
                0, 0, Gravity.TOP | Gravity.START);
    }

    private void assertPosition(PopupWindow popup, int anchorId,
            int contentEdgeX, int operatorX, int anchorEdgeX,
            int contentEdgeY, int operatorY, int anchorEdgeY,
            int offsetX, int offsetY, int gravity) {
        final View content = popup.getContentView();
        final View anchor = mActivity.findViewById(anchorId);

        getInstrumentation().runOnMainSync(() -> popup.showAsDropDown(
                anchor, offsetX, offsetY, gravity));
        getInstrumentation().waitForIdleSync();

        assertTrue(popup.isShowing());
        assertPositionX(content, contentEdgeX, operatorX, anchor, anchorEdgeX);
        assertPositionY(content, contentEdgeY, operatorY, anchor, anchorEdgeY);

        // Make sure it fits in the display frame.
        final Rect displayFrame = new Rect();
        anchor.getWindowVisibleDisplayFrame(displayFrame);
        final Rect contentFrame = new Rect();
        content.getBoundsOnScreen(contentFrame);
        assertTrue("Content (" + contentFrame + ") extends outside display (" + displayFrame + ")",
                displayFrame.contains(contentFrame));

        getInstrumentation().runOnMainSync(() -> popup.dismiss());
        getInstrumentation().waitForIdleSync();

        assertFalse(popup.isShowing());
    }

    public static void assertPositionY(View content, int contentEdge, int flags,
            View anchor, int anchorEdge) {
        final int[] anchorOnScreenXY = new int[2];
        anchor.getLocationOnScreen(anchorOnScreenXY);
        int anchorY = anchorOnScreenXY[1];
        if ((anchorEdge & BOTTOM) == BOTTOM) {
            anchorY += anchor.getHeight();
        }

        final int[] contentOnScreenXY = new int[2];
        content.getLocationOnScreen(contentOnScreenXY);
        int contentY = contentOnScreenXY[1];
        if ((contentEdge & BOTTOM) == BOTTOM) {
            contentY += content.getHeight();
        }

        assertComparison(contentY, flags, anchorY);
    }

    private static void assertPositionX(View content, int contentEdge, int flags,
            View anchor, int anchorEdge) {
        final int[] anchorOnScreenXY = new int[2];
        anchor.getLocationOnScreen(anchorOnScreenXY);
        int anchorX = anchorOnScreenXY[0];
        if ((anchorEdge & RIGHT) == RIGHT) {
            anchorX += anchor.getWidth();
        }

        final int[] contentOnScreenXY = new int[2];
        content.getLocationOnScreen(contentOnScreenXY);
        int contentX = contentOnScreenXY[0];
        if ((contentEdge & RIGHT) == RIGHT) {
            contentX += content.getWidth();
        }

        assertComparison(contentX, flags, anchorX);
    }

    private static void assertComparison(int left, int operator, int right) {
        switch (operator) {
            case GREATER_THAN:
                assertTrue(left + " <= " + right, left > right);
                break;
            case LESS_THAN:
                assertTrue(left + " >= " + right, left < right);
                break;
            case EQUAL_TO:
                assertTrue(left + " != " + right, left == right);
                break;
        }
    }

    public void testShowAtLocation() {
        int[] popupContentViewInWindowXY = new int[2];
        int[] popupContentViewOnScreenXY = new int[2];

        mPopupWindow = createPopupWindow(createPopupContent(50, 50));
        // Do not attach within the decor; we will be measuring location
        // with regard to screen coordinates.
        mPopupWindow.setAttachedInDecor(false);
        final View upperAnchor = mActivity.findViewById(R.id.anchor_upper);

        final int xOff = 10;
        final int yOff = 21;
        assertFalse(mPopupWindow.isShowing());
        mPopupWindow.getContentView().getLocationInWindow(popupContentViewInWindowXY);
        assertEquals(0, popupContentViewInWindowXY[0]);
        assertEquals(0, popupContentViewInWindowXY[1]);

        mInstrumentation.runOnMainSync(
                () -> mPopupWindow.showAtLocation(upperAnchor, Gravity.NO_GRAVITY, xOff, yOff));
        mInstrumentation.waitForIdleSync();

        assertTrue(mPopupWindow.isShowing());
        mPopupWindow.getContentView().getLocationInWindow(popupContentViewInWindowXY);
        mPopupWindow.getContentView().getLocationOnScreen(popupContentViewOnScreenXY);
        assertTrue(popupContentViewInWindowXY[0] >= 0);
        assertTrue(popupContentViewInWindowXY[1] >= 0);
        assertEquals(popupContentViewInWindowXY[0] + xOff, popupContentViewOnScreenXY[0]);
        assertEquals(popupContentViewInWindowXY[1] + yOff, popupContentViewOnScreenXY[1]);

        dismissPopup();
    }

    public void testShowAsDropDownWithOffsets() {
        int[] anchorXY = new int[2];
        int[] viewOnScreenXY = new int[2];
        int[] viewInWindowXY = new int[2];

        mPopupWindow = createPopupWindow(createPopupContent(50, 50));
        final View upperAnchor = mActivity.findViewById(R.id.anchor_upper);
        upperAnchor.getLocationOnScreen(anchorXY);
        int height = upperAnchor.getHeight();

        final int xOff = 11;
        final int yOff = 12;

        mInstrumentation.runOnMainSync(() -> mPopupWindow.showAsDropDown(upperAnchor, xOff, yOff));
        mInstrumentation.waitForIdleSync();

        mPopupWindow.getContentView().getLocationOnScreen(viewOnScreenXY);
        mPopupWindow.getContentView().getLocationInWindow(viewInWindowXY);
        assertEquals(anchorXY[0] + xOff + viewInWindowXY[0], viewOnScreenXY[0]);
        assertEquals(anchorXY[1] + height + yOff + viewInWindowXY[1], viewOnScreenXY[1]);

        dismissPopup();
    }

    public void testOverlapAnchor() {
        int[] anchorXY = new int[2];
        int[] viewOnScreenXY = new int[2];
        int[] viewInWindowXY = new int[2];

        mPopupWindow = createPopupWindow(createPopupContent(50, 50));
        final View upperAnchor = mActivity.findViewById(R.id.anchor_upper);
        upperAnchor.getLocationOnScreen(anchorXY);

        assertFalse(mPopupWindow.getOverlapAnchor());
        mPopupWindow.setOverlapAnchor(true);
        assertTrue(mPopupWindow.getOverlapAnchor());

        mInstrumentation.runOnMainSync(() -> mPopupWindow.showAsDropDown(upperAnchor, 0, 0));
        mInstrumentation.waitForIdleSync();

        mPopupWindow.getContentView().getLocationOnScreen(viewOnScreenXY);
        mPopupWindow.getContentView().getLocationInWindow(viewInWindowXY);
        assertEquals(anchorXY[0] + viewInWindowXY[0], viewOnScreenXY[0]);
        assertEquals(anchorXY[1] + viewInWindowXY[1], viewOnScreenXY[1]);
    }

    public void testAccessWindowLayoutType() {
        mPopupWindow = createPopupWindow(createPopupContent(50, 50));
        assertEquals(WindowManager.LayoutParams.TYPE_APPLICATION_PANEL,
                mPopupWindow.getWindowLayoutType());
        mPopupWindow.setWindowLayoutType(WindowManager.LayoutParams.TYPE_APPLICATION_SUB_PANEL);
        assertEquals(WindowManager.LayoutParams.TYPE_APPLICATION_SUB_PANEL,
                mPopupWindow.getWindowLayoutType());
    }

    public void testGetMaxAvailableHeight() {
        mPopupWindow = createPopupWindow(createPopupContent(50, 50));

        View anchorView = mActivity.findViewById(R.id.anchor_upper);
        int avaliable = getDisplay().getHeight() - anchorView.getHeight();
        int maxAvailableHeight = mPopupWindow.getMaxAvailableHeight(anchorView);
        assertTrue(maxAvailableHeight > 0);
        assertTrue(maxAvailableHeight <= avaliable);
        int maxAvailableHeightWithOffset = mPopupWindow.getMaxAvailableHeight(anchorView, 2);
        assertEquals(maxAvailableHeight - 2, maxAvailableHeightWithOffset);
        maxAvailableHeightWithOffset =
                mPopupWindow.getMaxAvailableHeight(anchorView, maxAvailableHeight);
        assertTrue(maxAvailableHeightWithOffset > 0);
        assertTrue(maxAvailableHeightWithOffset <= avaliable);
        maxAvailableHeightWithOffset =
                mPopupWindow.getMaxAvailableHeight(anchorView, maxAvailableHeight / 2 - 1);
        assertTrue(maxAvailableHeightWithOffset > 0);
        assertTrue(maxAvailableHeightWithOffset <= avaliable);
        maxAvailableHeightWithOffset = mPopupWindow.getMaxAvailableHeight(anchorView, -1);
        assertTrue(maxAvailableHeightWithOffset > 0);
        assertTrue(maxAvailableHeightWithOffset <= avaliable);

        anchorView = mActivity.findViewById(R.id.anchor_lower);
        // On some devices the view might actually have larger size than the physical display
        // due to chin and content will be laid out as if outside of the display. We need to use
        // larger from the display height and the main view height.
        avaliable = Math.max(getDisplay().getHeight(),
                mActivity.findViewById(android.R.id.content).getHeight()) - anchorView.getHeight();
        maxAvailableHeight = mPopupWindow.getMaxAvailableHeight(anchorView);
        assertTrue(maxAvailableHeight > 0);
        assertTrue(maxAvailableHeight <= avaliable);

        anchorView = mActivity.findViewById(R.id.anchor_middle_left);
        avaliable = getDisplay().getHeight() - anchorView.getHeight()
                - mActivity.findViewById(R.id.anchor_upper).getHeight();
        maxAvailableHeight = mPopupWindow.getMaxAvailableHeight(anchorView);
        assertTrue(maxAvailableHeight > 0);
        assertTrue(maxAvailableHeight <= avaliable);
    }

    @UiThreadTest
    public void testDismiss() {
        mPopupWindow = createPopupWindow(createPopupContent(50, 50));
        assertFalse(mPopupWindow.isShowing());
        View anchorView = mActivity.findViewById(R.id.anchor_upper);
        mPopupWindow.showAsDropDown(anchorView);

        mPopupWindow.dismiss();
        assertFalse(mPopupWindow.isShowing());

        mPopupWindow.dismiss();
        assertFalse(mPopupWindow.isShowing());
    }

    public void testSetOnDismissListener() {
        mPopupWindow = new PopupWindow(new TextView(mActivity));
        mPopupWindow.setOnDismissListener(null);

        OnDismissListener onDismissListener = mock(OnDismissListener.class);
        mPopupWindow.setOnDismissListener(onDismissListener);
        showPopup();
        dismissPopup();
        verify(onDismissListener, times(1)).onDismiss();

        showPopup();
        dismissPopup();
        verify(onDismissListener, times(2)).onDismiss();

        mPopupWindow.setOnDismissListener(null);
        showPopup();
        dismissPopup();
        verify(onDismissListener, times(2)).onDismiss();
    }

    public void testUpdate() {
        mPopupWindow = createPopupWindow(createPopupContent(50, 50));
        mPopupWindow.setBackgroundDrawable(null);
        showPopup();

        mPopupWindow.setIgnoreCheekPress();
        mPopupWindow.setFocusable(true);
        mPopupWindow.setTouchable(false);
        mPopupWindow.setInputMethodMode(PopupWindow.INPUT_METHOD_NOT_NEEDED);
        mPopupWindow.setClippingEnabled(false);
        mPopupWindow.setOutsideTouchable(true);

        WindowManager.LayoutParams p = (WindowManager.LayoutParams)
                mPopupWindow.getContentView().getRootView().getLayoutParams();

        assertEquals(0, WindowManager.LayoutParams.FLAG_IGNORE_CHEEK_PRESSES & p.flags);
        assertEquals(WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE,
                WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE & p.flags);
        assertEquals(0, WindowManager.LayoutParams.FLAG_NOT_TOUCHABLE & p.flags);
        assertEquals(0, WindowManager.LayoutParams.FLAG_WATCH_OUTSIDE_TOUCH & p.flags);
        assertEquals(0, WindowManager.LayoutParams.FLAG_LAYOUT_NO_LIMITS & p.flags);
        assertEquals(0, WindowManager.LayoutParams.FLAG_ALT_FOCUSABLE_IM & p.flags);

        mInstrumentation.runOnMainSync(() -> mPopupWindow.update());
        mInstrumentation.waitForIdleSync();

        assertEquals(WindowManager.LayoutParams.FLAG_IGNORE_CHEEK_PRESSES,
                WindowManager.LayoutParams.FLAG_IGNORE_CHEEK_PRESSES & p.flags);
        assertEquals(0, WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE & p.flags);
        assertEquals(WindowManager.LayoutParams.FLAG_NOT_TOUCHABLE,
                WindowManager.LayoutParams.FLAG_NOT_TOUCHABLE & p.flags);
        assertEquals(WindowManager.LayoutParams.FLAG_WATCH_OUTSIDE_TOUCH,
                WindowManager.LayoutParams.FLAG_WATCH_OUTSIDE_TOUCH & p.flags);
        assertEquals(WindowManager.LayoutParams.FLAG_LAYOUT_NO_LIMITS,
                WindowManager.LayoutParams.FLAG_LAYOUT_NO_LIMITS & p.flags);
        assertEquals(WindowManager.LayoutParams.FLAG_ALT_FOCUSABLE_IM,
                WindowManager.LayoutParams.FLAG_ALT_FOCUSABLE_IM & p.flags);
    }

    public void testEnterExitTransition() {
        TransitionListener enterListener = mock(TransitionListener.class);
        Transition enterTransition = new BaseTransition();
        enterTransition.addListener(enterListener);

        TransitionListener exitListener = mock(TransitionListener.class);
        Transition exitTransition = new BaseTransition();
        exitTransition.addListener(exitListener);

        OnDismissListener dismissListener = mock(OnDismissListener.class);

        mPopupWindow = createPopupWindow(createPopupContent(50, 50));
        mPopupWindow.setEnterTransition(enterTransition);
        mPopupWindow.setExitTransition(exitTransition);
        mPopupWindow.setOnDismissListener(dismissListener);
        verify(enterListener, never()).onTransitionStart(any(Transition.class));
        verify(exitListener, never()).onTransitionStart(any(Transition.class));
        verify(dismissListener, never()).onDismiss();

        final View anchorView = mActivity.findViewById(R.id.anchor_upper);
        mInstrumentation.runOnMainSync(() -> mPopupWindow.showAsDropDown(anchorView, 0, 0));
        mInstrumentation.waitForIdleSync();
        verify(enterListener, times(1)).onTransitionStart(any(Transition.class));
        verify(exitListener, never()).onTransitionStart(any(Transition.class));
        verify(dismissListener, never()).onDismiss();

        mInstrumentation.runOnMainSync(() -> mPopupWindow.dismiss());
        mInstrumentation.waitForIdleSync();
        verify(enterListener, times(1)).onTransitionStart(any(Transition.class));
        verify(exitListener, times(1)).onTransitionStart(any(Transition.class));
        verify(dismissListener, times(1)).onDismiss();
    }

    public void testUpdatePositionAndDimension() {
        int[] fstXY = new int[2];
        int[] sndXY = new int[2];
        int[] viewInWindowXY = new int[2];

        mInstrumentation.runOnMainSync(() -> {
            mPopupWindow = createPopupWindow(createPopupContent(50, 50));
            // Do not attach within the decor; we will be measuring location
            // with regard to screen coordinates.
            mPopupWindow.setAttachedInDecor(false);
        });

        mInstrumentation.waitForIdleSync();
        // Do not update if it is not shown
        assertFalse(mPopupWindow.isShowing());
        assertEquals(100, mPopupWindow.getWidth());
        assertEquals(100, mPopupWindow.getHeight());

        showPopup();
        mPopupWindow.getContentView().getLocationInWindow(viewInWindowXY);

        // update if it is not shown
        mInstrumentation.runOnMainSync(() -> mPopupWindow.update(20, 50, 50, 50));

        mInstrumentation.waitForIdleSync();
        assertTrue(mPopupWindow.isShowing());
        assertEquals(50, mPopupWindow.getWidth());
        assertEquals(50, mPopupWindow.getHeight());

        mPopupWindow.getContentView().getLocationOnScreen(fstXY);
        assertEquals(viewInWindowXY[0] + 20, fstXY[0]);
        assertEquals(viewInWindowXY[1] + 50, fstXY[1]);

        // ignore if width or height is -1
        mInstrumentation.runOnMainSync(() -> mPopupWindow.update(4, 0, -1, -1, true));
        mInstrumentation.waitForIdleSync();

        assertTrue(mPopupWindow.isShowing());
        assertEquals(50, mPopupWindow.getWidth());
        assertEquals(50, mPopupWindow.getHeight());

        mPopupWindow.getContentView().getLocationOnScreen(sndXY);
        assertEquals(viewInWindowXY[0] + 4, sndXY[0]);
        assertEquals(viewInWindowXY[1], sndXY[1]);

        dismissPopup();
    }

    public void testUpdateDimensionAndAlignAnchorView() {
        mInstrumentation.runOnMainSync(
                () -> mPopupWindow = createPopupWindow(createPopupContent(50, 50)));
        mInstrumentation.waitForIdleSync();

        final View anchorView = mActivity.findViewById(R.id.anchor_upper);
        mPopupWindow.update(anchorView, 50, 50);
        // Do not update if it is not shown
        assertFalse(mPopupWindow.isShowing());
        assertEquals(100, mPopupWindow.getWidth());
        assertEquals(100, mPopupWindow.getHeight());

        mInstrumentation.runOnMainSync(() -> mPopupWindow.showAsDropDown(anchorView));
        mInstrumentation.waitForIdleSync();
        // update if it is shown
        mInstrumentation.runOnMainSync(() -> mPopupWindow.update(anchorView, 50, 50));
        mInstrumentation.waitForIdleSync();
        assertTrue(mPopupWindow.isShowing());
        assertEquals(50, mPopupWindow.getWidth());
        assertEquals(50, mPopupWindow.getHeight());

        // ignore if width or height is -1
        mInstrumentation.runOnMainSync(() -> mPopupWindow.update(anchorView, -1, -1));
        mInstrumentation.waitForIdleSync();
        assertTrue(mPopupWindow.isShowing());
        assertEquals(50, mPopupWindow.getWidth());
        assertEquals(50, mPopupWindow.getHeight());

        mInstrumentation.runOnMainSync(() -> mPopupWindow.dismiss());
        mInstrumentation.waitForIdleSync();
    }

    public void testUpdateDimensionAndAlignAnchorViewWithOffsets() {
        int[] anchorXY = new int[2];
        int[] viewInWindowOff = new int[2];
        int[] viewXY = new int[2];

        mPopupWindow = createPopupWindow(createPopupContent(50, 50));
        final View anchorView = mActivity.findViewById(R.id.anchor_upper);
        // Do not update if it is not shown
        assertFalse(mPopupWindow.isShowing());
        assertEquals(100, mPopupWindow.getWidth());
        assertEquals(100, mPopupWindow.getHeight());

        showPopup();
        anchorView.getLocationOnScreen(anchorXY);
        mPopupWindow.getContentView().getLocationInWindow(viewInWindowOff);

        // update if it is not shown
        mInstrumentation.runOnMainSync(() -> mPopupWindow.update(anchorView, 20, 50, 50, 50));

        mInstrumentation.waitForIdleSync();

        assertTrue(mPopupWindow.isShowing());
        assertEquals(50, mPopupWindow.getWidth());
        assertEquals(50, mPopupWindow.getHeight());

        mPopupWindow.getContentView().getLocationOnScreen(viewXY);

        // The popup should appear below and to right with an offset.
        assertEquals(anchorXY[0] + 20 + viewInWindowOff[0], viewXY[0]);
        assertEquals(anchorXY[1] + anchorView.getHeight() + 50 + viewInWindowOff[1], viewXY[1]);

        // ignore width and height but change location
        mInstrumentation.runOnMainSync(() -> mPopupWindow.update(anchorView, 10, 50, -1, -1));
        mInstrumentation.waitForIdleSync();

        assertTrue(mPopupWindow.isShowing());
        assertEquals(50, mPopupWindow.getWidth());
        assertEquals(50, mPopupWindow.getHeight());

        mPopupWindow.getContentView().getLocationOnScreen(viewXY);

        // The popup should appear below and to right with an offset.
        assertEquals(anchorXY[0] + 10 + viewInWindowOff[0], viewXY[0]);
        assertEquals(anchorXY[1] + anchorView.getHeight() + 50 + viewInWindowOff[1], viewXY[1]);

        final View anotherView = mActivity.findViewById(R.id.anchor_middle_left);
        mInstrumentation.runOnMainSync(() -> mPopupWindow.update(anotherView, 0, 0, 60, 60));
        mInstrumentation.waitForIdleSync();

        assertTrue(mPopupWindow.isShowing());
        assertEquals(60, mPopupWindow.getWidth());
        assertEquals(60, mPopupWindow.getHeight());

        int[] newXY = new int[2];
        anotherView.getLocationOnScreen(newXY);
        mPopupWindow.getContentView().getLocationOnScreen(viewXY);

        // The popup should appear below and to the right.
        assertEquals(newXY[0] + viewInWindowOff[0], viewXY[0]);
        assertEquals(newXY[1] + anotherView.getHeight() + viewInWindowOff[1], viewXY[1]);

        dismissPopup();
    }

    public void testAccessInputMethodMode() {
        mPopupWindow = new PopupWindow(mActivity);
        assertEquals(0, mPopupWindow.getInputMethodMode());

        mPopupWindow.setInputMethodMode(PopupWindow.INPUT_METHOD_FROM_FOCUSABLE);
        assertEquals(PopupWindow.INPUT_METHOD_FROM_FOCUSABLE, mPopupWindow.getInputMethodMode());

        mPopupWindow.setInputMethodMode(PopupWindow.INPUT_METHOD_NEEDED);
        assertEquals(PopupWindow.INPUT_METHOD_NEEDED, mPopupWindow.getInputMethodMode());

        mPopupWindow.setInputMethodMode(PopupWindow.INPUT_METHOD_NOT_NEEDED);
        assertEquals(PopupWindow.INPUT_METHOD_NOT_NEEDED, mPopupWindow.getInputMethodMode());

        mPopupWindow.setInputMethodMode(-1);
        assertEquals(-1, mPopupWindow.getInputMethodMode());
    }

    public void testAccessClippingEnabled() {
        mPopupWindow = new PopupWindow(mActivity);
        assertTrue(mPopupWindow.isClippingEnabled());

        mPopupWindow.setClippingEnabled(false);
        assertFalse(mPopupWindow.isClippingEnabled());
    }

    public void testAccessOutsideTouchable() {
        mPopupWindow = new PopupWindow(mActivity);
        assertFalse(mPopupWindow.isOutsideTouchable());

        mPopupWindow.setOutsideTouchable(true);
        assertTrue(mPopupWindow.isOutsideTouchable());
    }

    public void testAccessTouchable() {
        mPopupWindow = new PopupWindow(mActivity);
        assertTrue(mPopupWindow.isTouchable());

        mPopupWindow.setTouchable(false);
        assertFalse(mPopupWindow.isTouchable());
    }

    public void testIsAboveAnchor() {
        mInstrumentation.runOnMainSync(() -> mPopupWindow = createPopupWindow(createPopupContent(50,
                50)));
        mInstrumentation.waitForIdleSync();
        final View upperAnchor = mActivity.findViewById(R.id.anchor_upper);

        mInstrumentation.runOnMainSync(() -> mPopupWindow.showAsDropDown(upperAnchor));
        mInstrumentation.waitForIdleSync();
        assertFalse(mPopupWindow.isAboveAnchor());
        dismissPopup();

        mPopupWindow = createPopupWindow(createPopupContent(50, 50));
        final View lowerAnchor = mActivity.findViewById(R.id.anchor_lower);

        mInstrumentation.runOnMainSync(() -> mPopupWindow.showAsDropDown(lowerAnchor, 0, 0));
        mInstrumentation.waitForIdleSync();
        assertTrue(mPopupWindow.isAboveAnchor());
        dismissPopup();
    }

    public void testSetTouchInterceptor() {
        mPopupWindow = new PopupWindow(new TextView(mActivity));

        OnTouchListener onTouchListener = mock(OnTouchListener.class);
        when(onTouchListener.onTouch(any(View.class), any(MotionEvent.class))).thenReturn(true);

        mPopupWindow.setTouchInterceptor(onTouchListener);
        mPopupWindow.setFocusable(true);
        mPopupWindow.setOutsideTouchable(true);
        Drawable drawable = new ColorDrawable();
        mPopupWindow.setBackgroundDrawable(drawable);
        showPopup();

        int[] xy = new int[2];
        mPopupWindow.getContentView().getLocationOnScreen(xy);
        final int viewWidth = mPopupWindow.getContentView().getWidth();
        final int viewHeight = mPopupWindow.getContentView().getHeight();
        final float x = xy[0] + (viewWidth / 2.0f);
        float y = xy[1] + (viewHeight / 2.0f);

        long downTime = SystemClock.uptimeMillis();
        long eventTime = SystemClock.uptimeMillis();
        MotionEvent event = MotionEvent.obtain(downTime, eventTime,
                MotionEvent.ACTION_DOWN, x, y, 0);
        getInstrumentation().sendPointerSync(event);
        verify(onTouchListener, times(1)).onTouch(any(View.class), any(MotionEvent.class));

        downTime = SystemClock.uptimeMillis();
        eventTime = SystemClock.uptimeMillis();
        event = MotionEvent.obtain(downTime, eventTime, MotionEvent.ACTION_UP, x, y, 0);
        getInstrumentation().sendPointerSync(event);
        verify(onTouchListener, times(2)).onTouch(any(View.class), any(MotionEvent.class));

        mPopupWindow.setTouchInterceptor(null);
        downTime = SystemClock.uptimeMillis();
        eventTime = SystemClock.uptimeMillis();
        event = MotionEvent.obtain(downTime, eventTime, MotionEvent.ACTION_DOWN, x, y, 0);
        getInstrumentation().sendPointerSync(event);
        verify(onTouchListener, times(2)).onTouch(any(View.class), any(MotionEvent.class));
    }

    public void testSetWindowLayoutMode() {
        mPopupWindow = new PopupWindow(new TextView(mActivity));
        showPopup();

        ViewGroup.LayoutParams p = mPopupWindow.getContentView().getRootView().getLayoutParams();
        assertEquals(0, p.width);
        assertEquals(0, p.height);

        mPopupWindow.setWindowLayoutMode(LayoutParams.WRAP_CONTENT, LayoutParams.MATCH_PARENT);
        mInstrumentation.runOnMainSync(() -> mPopupWindow.update(20, 50, 50, 50));

        assertEquals(LayoutParams.WRAP_CONTENT, p.width);
        assertEquals(LayoutParams.MATCH_PARENT, p.height);
    }

    private static class BaseTransition extends Transition {
        @Override
        public void captureStartValues(TransitionValues transitionValues) {}

        @Override
        public void captureEndValues(TransitionValues transitionValues) {}
    }

    private View createPopupContent(int width, int height) {
        final View popupView = new View(mActivity);
        popupView.setLayoutParams(new ViewGroup.LayoutParams(width, height));
        popupView.setBackgroundColor(Color.MAGENTA);

        return popupView;
    }

    private PopupWindow createPopupWindow() {
        PopupWindow window = new PopupWindow(mActivity);
        window.setWidth(100);
        window.setHeight(100);
        window.setBackgroundDrawable(new ColorDrawable(Color.YELLOW));
        return window;
    }

    private PopupWindow createPopupWindow(View content) {
        PopupWindow window = createPopupWindow();
        window.setContentView(content);
        return window;
    }

    private void showPopup() {
        mInstrumentation.runOnMainSync(() -> {
            if (mPopupWindow == null || mPopupWindow.isShowing()) {
                return;
            }
            View anchor = mActivity.findViewById(R.id.anchor_upper);
            mPopupWindow.showAsDropDown(anchor);
            assertTrue(mPopupWindow.isShowing());
        });
        mInstrumentation.waitForIdleSync();
    }

    private void dismissPopup() {
        mInstrumentation.runOnMainSync(() -> {
            if (mPopupWindow == null || !mPopupWindow.isShowing()) {
                return;
            }
            mPopupWindow.dismiss();
        });
        mInstrumentation.waitForIdleSync();
    }
}
