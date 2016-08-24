/*
 * Copyright (C) 2009 The Android Open Source Project
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

import android.widget.FrameLayout;
import android.widget.cts.R;


import org.xmlpull.v1.XmlPullParser;

import android.app.Activity;
import android.content.Context;
import android.cts.util.PollingCheck;
import android.cts.util.WidgetTestUtils;
import android.graphics.Rect;
import android.test.ActivityInstrumentationTestCase2;
import android.test.UiThreadTest;
import android.util.AttributeSet;
import android.util.Xml;
import android.view.View;
import android.view.ViewGroup;
import android.view.View.MeasureSpec;
import android.widget.HorizontalScrollView;
import android.widget.TextView;
import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.verify;

/**
 * Test {@link HorizontalScrollView}.
 */
public class HorizontalScrollViewTest
        extends ActivityInstrumentationTestCase2<HorizontalScrollViewCtsActivity> {
    private static final int ITEM_WIDTH  = 250;
    private static final int ITEM_HEIGHT = 100;
    private static final int ITEM_COUNT  = 15;
    private static final int PAGE_WIDTH  = 100;
    private static final int PAGE_HEIGHT = 100;
    private static final int SCROLL_RIGHT = ITEM_WIDTH * ITEM_COUNT - PAGE_WIDTH;
    private MyHorizontalScrollView mScrollView;
    private Activity mActivity;

    public HorizontalScrollViewTest() {
        super("android.widget.cts", HorizontalScrollViewCtsActivity.class);
    }

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mActivity = getActivity();
        mScrollView = (MyHorizontalScrollView) mActivity.findViewById(R.id.horizontal_scroll_view);
    }

    public void testConstructor() {
        XmlPullParser parser = mActivity.getResources().getLayout(R.layout.horizontal_scrollview);
        AttributeSet attrs = Xml.asAttributeSet(parser);
        new HorizontalScrollView(mActivity);

        new HorizontalScrollView(mActivity, attrs);

        new HorizontalScrollView(mActivity, attrs, 0);
    }

    public void testGetMaxScrollAmount() {
        HorizontalScrollView scrollView = new HorizontalScrollView(mActivity);
        scrollView.layout(0, 0, 100, 200);
        assertEquals((100 - 0) / 2, scrollView.getMaxScrollAmount());

        scrollView.layout(0, 0, 150, 100);
        assertEquals((150 - 0) / 2, scrollView.getMaxScrollAmount());
    }

    public void testAddView() {
        HorizontalScrollView scrollView = new HorizontalScrollView(mActivity);
        TextView child0 = new TextView(mActivity);
        scrollView.addView(child0);
        assertSame(child0, scrollView.getChildAt(0));

        assertEquals(1, scrollView.getChildCount());
        TextView child1 = new TextView(mActivity);
        try {
            scrollView.addView(child1);
            fail("did not throw IllegalStateException when add more than one child");
        } catch (IllegalStateException e) {
            // expected
        }
        assertEquals(1, scrollView.getChildCount());
    }

    public void testAddViewWithIndex() {
        HorizontalScrollView scrollView = new HorizontalScrollView(mActivity);
        TextView child0 = new TextView(mActivity);
        scrollView.addView(child0, 0);
        assertSame(child0, scrollView.getChildAt(0));

        assertEquals(1, scrollView.getChildCount());
        TextView child1 = new TextView(mActivity);
        try {
            scrollView.addView(child1, 1);
            fail("did not throw IllegalStateException when add more than one child");
        } catch (IllegalStateException e) {
            // expected
        }
        assertEquals(1, scrollView.getChildCount());

        scrollView.removeAllViews();
        scrollView = new HorizontalScrollView(mActivity);
        scrollView.addView(child0, -1);
        assertSame(child0, scrollView.getChildAt(0));

        assertEquals(1, scrollView.getChildCount());
        child1 = new TextView(mActivity);
        try {
            scrollView.addView(child1, -1);
            fail("did not throw IllegalStateException when add more than one child");
        } catch (IllegalStateException e) {
            // expected
        }
        assertEquals(1, scrollView.getChildCount());

        scrollView.removeAllViews();
        scrollView = new HorizontalScrollView(mActivity);
        try {
            scrollView.addView(child0, 1);
            fail("did not throw IndexOutOfBoundsException when index is larger than 0");
        } catch (IndexOutOfBoundsException e) {
            // expected
        }
    }

    public void testAddViewWithLayoutParams() {
        HorizontalScrollView scrollView = new HorizontalScrollView(mActivity);
        TextView child0 = new TextView(mActivity);
        scrollView.addView(child0, new ViewGroup.LayoutParams(200, 100));
        assertSame(child0, scrollView.getChildAt(0));
        assertEquals(200, child0.getLayoutParams().width);
        assertEquals(100, child0.getLayoutParams().height);

        assertEquals(1, scrollView.getChildCount());
        TextView child1 = new TextView(mActivity);
        try {
            scrollView.addView(child1, new ViewGroup.LayoutParams(200, 100));
            fail("did not throw IllegalStateException when add more than one child");
        } catch (IllegalStateException e) {
            // expected
        }
        assertEquals(1, scrollView.getChildCount());

        scrollView.removeAllViews();
        scrollView = new HorizontalScrollView(mActivity);
        child0 = new TextView(mActivity);
        try {
            scrollView.addView(child0, null);
            fail("did not throw NullPointerException when LayoutParams is null.");
        } catch (NullPointerException e) {
            // expected
        }
    }

    public void testAddViewWithIndexAndLayoutParams() {
        HorizontalScrollView scrollView = new HorizontalScrollView(mActivity);
        TextView child0 = new TextView(mActivity);
        scrollView.addView(child0, 0, new ViewGroup.LayoutParams(200, 100));
        assertSame(child0, scrollView.getChildAt(0));
        assertEquals(200, child0.getLayoutParams().width);
        assertEquals(100, child0.getLayoutParams().height);

        assertEquals(1, scrollView.getChildCount());
        TextView child1 = new TextView(mActivity);
        try {
            scrollView.addView(child1, 0, new ViewGroup.LayoutParams(200, 100));
            fail("did not throw IllegalStateException when add more than one child");
        } catch (IllegalStateException e) {
            // expected
        }
        assertEquals(1, scrollView.getChildCount());

        scrollView.removeAllViews();
        scrollView = new HorizontalScrollView(mActivity);
        child0 = new TextView(mActivity);
        try {
            scrollView.addView(child0, null);
            fail("did not throw NullPointerException when LayoutParams is null.");
        } catch (NullPointerException e) {
            // expected
        }

        scrollView.removeAllViews();
        scrollView = new HorizontalScrollView(mActivity);
        scrollView.addView(child0, -1, new ViewGroup.LayoutParams(300, 150));
        assertSame(child0, scrollView.getChildAt(0));
        assertEquals(300, child0.getLayoutParams().width);
        assertEquals(150, child0.getLayoutParams().height);

        assertEquals(1, scrollView.getChildCount());
        child1 = new TextView(mActivity);
        try {
            scrollView.addView(child1, -1, new ViewGroup.LayoutParams(200, 100));
            fail("did not throw IllegalStateException when add more than one child");
        } catch (IllegalStateException e) {
            // expected
        }
        assertEquals(1, scrollView.getChildCount());

        scrollView.removeAllViews();
        scrollView = new HorizontalScrollView(mActivity);
        try {
            scrollView.addView(child0, 1, new ViewGroup.LayoutParams(200, 100));
            fail("did not throw IndexOutOfBoundsException when index is larger than 0");
        } catch (IndexOutOfBoundsException e) {
            // expected
        }
    }

    public void testAccessFillViewport() {
        HorizontalScrollView scrollView = new HorizontalScrollView(mActivity);
        assertFalse(scrollView.isFillViewport());
        scrollView.layout(0, 0, 100, 100);
        assertFalse(scrollView.isLayoutRequested());

        scrollView.setFillViewport(false);
        assertFalse(scrollView.isFillViewport());
        assertFalse(scrollView.isLayoutRequested());

        scrollView.setFillViewport(true);
        assertTrue(scrollView.isFillViewport());
        assertTrue(scrollView.isLayoutRequested());

        scrollView.layout(0, 0, 100, 100);
        assertFalse(mScrollView.isLayoutRequested());

        scrollView.setFillViewport(false);
        assertFalse(scrollView.isFillViewport());
        assertTrue(scrollView.isLayoutRequested());
    }

    public void testAccessSmoothScrollingEnabled() throws Throwable {
        assertTrue(mScrollView.isSmoothScrollingEnabled());

        // scroll immediately
        mScrollView.setSmoothScrollingEnabled(false);
        assertFalse(mScrollView.isSmoothScrollingEnabled());

        runTestOnUiThread(new Runnable() {
            public void run() {
                mScrollView.fullScroll(View.FOCUS_RIGHT);
            }
        });
        assertEquals(SCROLL_RIGHT, mScrollView.getScrollX());

        runTestOnUiThread(new Runnable() {
            public void run() {
                mScrollView.fullScroll(View.FOCUS_LEFT);
            }
        });
        assertEquals(0, mScrollView.getScrollX());

        // smooth scroll
        mScrollView.setSmoothScrollingEnabled(true);
        assertTrue(mScrollView.isSmoothScrollingEnabled());

        runTestOnUiThread(new Runnable() {
            public void run() {
                mScrollView.fullScroll(View.FOCUS_RIGHT);
            }
        });
        pollingCheckSmoothScrolling(0, SCROLL_RIGHT, 0, 0);
        assertEquals(SCROLL_RIGHT, mScrollView.getScrollX());

        runTestOnUiThread(new Runnable() {
            public void run() {
                mScrollView.fullScroll(View.FOCUS_LEFT);
            }
        });
        pollingCheckSmoothScrolling(SCROLL_RIGHT, 0, 0, 0);
        assertEquals(0, mScrollView.getScrollX());
    }

    public void testMeasureChild() {
        MyHorizontalScrollView scrollView = new MyHorizontalScrollView(mActivity);

        MyView child = new MyView(mActivity);
        child.setBackgroundDrawable(null);
        child.setPadding(0, 0, 0, 0);
        child.setMinimumWidth(30);
        child.setLayoutParams(new ViewGroup.LayoutParams(100, 100));
        child.measure(MeasureSpec.makeMeasureSpec(100, MeasureSpec.EXACTLY),
                MeasureSpec.makeMeasureSpec(100, MeasureSpec.EXACTLY));

        assertEquals(100, child.getMeasuredHeight());
        assertEquals(100, child.getMeasuredWidth());

        scrollView.measureChild(child, MeasureSpec.makeMeasureSpec(100, MeasureSpec.EXACTLY),
                MeasureSpec.makeMeasureSpec(100, MeasureSpec.EXACTLY));

        assertEquals(100, child.getMeasuredHeight());
        assertEquals(30, child.getMeasuredWidth());
    }

    public void testMeasureChildWithMargins() {
        MyHorizontalScrollView scrollView = new MyHorizontalScrollView(mActivity);

        MyView child = new MyView(mActivity);
        child.setBackgroundDrawable(null);
        child.setPadding(0, 0, 0, 0);
        child.setMinimumWidth(30);
        child.setLayoutParams(new ViewGroup.MarginLayoutParams(100, 100));
        child.measure(MeasureSpec.makeMeasureSpec(100, MeasureSpec.EXACTLY),
                MeasureSpec.makeMeasureSpec(100, MeasureSpec.EXACTLY));

        assertEquals(100, child.getMeasuredHeight());
        assertEquals(100, child.getMeasuredWidth());

        scrollView.measureChildWithMargins(child,
                MeasureSpec.makeMeasureSpec(100, MeasureSpec.EXACTLY), 5,
                MeasureSpec.makeMeasureSpec(100, MeasureSpec.EXACTLY), 5);

        assertEquals(100, child.getMeasuredHeight());
        assertEquals(30, child.getMeasuredWidth());
    }

    public void testMeasureSpecs() {
        MyView child = spy(new MyView(mActivity));
        MyHorizontalScrollView scrollView = new MyHorizontalScrollView(mActivity);
        scrollView.addView(child);

        scrollView.measureChild(child, MeasureSpec.makeMeasureSpec(100, MeasureSpec.EXACTLY),
                MeasureSpec.makeMeasureSpec(150, MeasureSpec.EXACTLY));
        verify(child).onMeasure(
                eq(MeasureSpec.makeMeasureSpec(100, MeasureSpec.UNSPECIFIED)),
                eq(MeasureSpec.makeMeasureSpec(150, MeasureSpec.EXACTLY)));
    }

    public void testMeasureSpecsWithPadding() {
        MyView child = spy(new MyView(mActivity));
        MyHorizontalScrollView scrollView = new MyHorizontalScrollView(mActivity);
        scrollView.setPadding(3, 5, 7, 11);
        scrollView.addView(child);

        scrollView.measureChild(child, MeasureSpec.makeMeasureSpec(100, MeasureSpec.EXACTLY),
                MeasureSpec.makeMeasureSpec(150, MeasureSpec.EXACTLY));
        verify(child).onMeasure(
                eq(MeasureSpec.makeMeasureSpec(90, MeasureSpec.UNSPECIFIED)),
                eq(MeasureSpec.makeMeasureSpec(134, MeasureSpec.EXACTLY)));
    }

    public void testMeasureSpecsWithMargins() {
        MyView child = spy(new MyView(mActivity));
        MyHorizontalScrollView scrollView = new MyHorizontalScrollView(mActivity);
        scrollView.addView(child);

        scrollView.measureChildWithMargins(child,
                MeasureSpec.makeMeasureSpec(100, MeasureSpec.EXACTLY), 15,
                MeasureSpec.makeMeasureSpec(150, MeasureSpec.EXACTLY), 20);
        verify(child).onMeasure(
                eq(MeasureSpec.makeMeasureSpec(85, MeasureSpec.UNSPECIFIED)),
                eq(MeasureSpec.makeMeasureSpec(130, MeasureSpec.EXACTLY)));
    }

    public void testMeasureSpecsWithMarginsAndPadding() {
        MyView child = spy(new MyView(mActivity));
        MyHorizontalScrollView scrollView = new MyHorizontalScrollView(mActivity);
        scrollView.setPadding(3, 5, 7, 11);
        scrollView.addView(child);

        scrollView.measureChildWithMargins(child,
                MeasureSpec.makeMeasureSpec(100, MeasureSpec.EXACTLY), 15,
                MeasureSpec.makeMeasureSpec(150, MeasureSpec.EXACTLY), 20);
        verify(child).onMeasure(
                eq(MeasureSpec.makeMeasureSpec(75, MeasureSpec.UNSPECIFIED)),
                eq(MeasureSpec.makeMeasureSpec(114, MeasureSpec.EXACTLY)));
    }

    public void testMeasureSpecsWithMarginsAndNoHintWidth() {
        MyView child = spy(new MyView(mActivity));
        MyHorizontalScrollView scrollView = new MyHorizontalScrollView(mActivity);
        scrollView.addView(child);

        scrollView.measureChildWithMargins(child,
                MeasureSpec.makeMeasureSpec(0, MeasureSpec.UNSPECIFIED), 15,
                MeasureSpec.makeMeasureSpec(150, MeasureSpec.EXACTLY), 20);
        verify(child).onMeasure(
                eq(MeasureSpec.makeMeasureSpec(0, MeasureSpec.UNSPECIFIED)),
                eq(MeasureSpec.makeMeasureSpec(130, MeasureSpec.EXACTLY)));
    }

    public void testFillViewport() {
        MyHorizontalScrollView scrollView = new MyHorizontalScrollView(mActivity);

        MyView child = new MyView(mActivity);
        scrollView.setFillViewport(true);
        child.setLayoutParams(new ViewGroup.LayoutParams(
                ViewGroup.LayoutParams.WRAP_CONTENT,
                ViewGroup.LayoutParams.WRAP_CONTENT
        ));

        scrollView.addView(child);
        scrollView.measure(MeasureSpec.makeMeasureSpec(150, MeasureSpec.EXACTLY),
                MeasureSpec.makeMeasureSpec(100, MeasureSpec.EXACTLY));

        assertEquals(150, child.getMeasuredWidth());
        assertEquals(100, child.getMeasuredHeight());

        scrollView.layout(0, 0, 150, 100);
        assertEquals(0, child.getLeft());
    }

    public void testFillViewportWithScrollViewPadding() {
        MyHorizontalScrollView scrollView = new MyHorizontalScrollView(mActivity);
        scrollView.setFillViewport(true);
        scrollView.setPadding(3, 10, 5, 7);

        MyView child = new MyView(mActivity);
        child.setLayoutParams(new ViewGroup.LayoutParams(10,10));
        child.setDesiredWidth(30);

        scrollView.addView(child);
        scrollView.measure(MeasureSpec.makeMeasureSpec(100, MeasureSpec.EXACTLY),
                MeasureSpec.makeMeasureSpec(150, MeasureSpec.EXACTLY));

        assertEquals(92, child.getMeasuredWidth());
        assertEquals(10, child.getMeasuredHeight());

        scrollView.layout(0, 0, 100, 150);
        assertEquals(3, child.getLeft());
    }

    public void testFillViewportWithChildMargins() {
        MyHorizontalScrollView scrollView = new MyHorizontalScrollView(mActivity);
        scrollView.setFillViewport(true);

        MyView child = new MyView(mActivity);
        FrameLayout.LayoutParams lp = new FrameLayout.LayoutParams(10, 10);
        lp.leftMargin = 3;
        lp.topMargin = 10;
        lp.rightMargin = 5;
        lp.bottomMargin = 7;
        child.setDesiredWidth(30);
        child.setLayoutParams(lp);

        scrollView.addView(child);
        scrollView.measure(MeasureSpec.makeMeasureSpec(100, MeasureSpec.EXACTLY),
                MeasureSpec.makeMeasureSpec(150, MeasureSpec.EXACTLY));

        assertEquals(92, child.getMeasuredWidth());
        assertEquals(10, child.getMeasuredHeight());

        scrollView.layout(0, 0, 100, 150);
        assertEquals(3, child.getLeft());
    }

    public void testFillViewportWithScrollViewPaddingAlreadyFills() {
        MyHorizontalScrollView scrollView = new MyHorizontalScrollView(mActivity);
        scrollView.setFillViewport(true);
        scrollView.setPadding(3, 10, 5, 7);

        MyView child = new MyView(mActivity);
        child.setDesiredWidth(175);

        scrollView.addView(child);
        scrollView.measure(MeasureSpec.makeMeasureSpec(100, MeasureSpec.EXACTLY),
                MeasureSpec.makeMeasureSpec(150, MeasureSpec.EXACTLY));


        assertEquals(175, child.getMeasuredWidth());
        assertEquals(133, child.getMeasuredHeight());

        scrollView.layout(0, 0, 100, 150);
        assertEquals(3, child.getLeft());
    }

    public void testFillViewportWithChildMarginsAlreadyFills() {
        MyHorizontalScrollView scrollView = new MyHorizontalScrollView(mActivity);
        scrollView.setFillViewport(true);
        MyView child = new MyView(mActivity);
        FrameLayout.LayoutParams lp = new FrameLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.WRAP_CONTENT);

        lp.leftMargin = 3;
        lp.topMargin = 10;
        lp.rightMargin = 5;
        lp.bottomMargin = 7;
        child.setLayoutParams(lp);
        child.setDesiredWidth(175);

        scrollView.addView(child);
        scrollView.measure(MeasureSpec.makeMeasureSpec(100, MeasureSpec.EXACTLY),
                MeasureSpec.makeMeasureSpec(150, MeasureSpec.EXACTLY));

        assertEquals(175, child.getMeasuredWidth());
        assertEquals(133, child.getMeasuredHeight());

        scrollView.layout(0, 0, 100, 150);
        assertEquals(3, child.getLeft());
    }

    @UiThreadTest
    public void testPageScroll() {
        mScrollView.setSmoothScrollingEnabled(false);
        assertEquals(0, mScrollView.getScrollX());

        assertTrue(mScrollView.pageScroll(View.FOCUS_RIGHT));
        assertEquals(PAGE_WIDTH, mScrollView.getScrollX());

        mScrollView.scrollTo(SCROLL_RIGHT, PAGE_HEIGHT);
        assertFalse(mScrollView.pageScroll(View.FOCUS_RIGHT));
        assertEquals(SCROLL_RIGHT, mScrollView.getScrollX());

        assertTrue(mScrollView.pageScroll(View.FOCUS_LEFT));
        assertEquals(SCROLL_RIGHT - PAGE_WIDTH, mScrollView.getScrollX());

        mScrollView.scrollTo(0, PAGE_HEIGHT);
        assertFalse(mScrollView.pageScroll(View.FOCUS_LEFT));
        assertEquals(0, mScrollView.getScrollX());
    }

    @UiThreadTest
    public void testFullScroll() {
        mScrollView.setSmoothScrollingEnabled(false);
        assertEquals(0, mScrollView.getScrollX());

        assertTrue(mScrollView.fullScroll(View.FOCUS_RIGHT));
        assertEquals(SCROLL_RIGHT, mScrollView.getScrollX());

        assertFalse(mScrollView.fullScroll(View.FOCUS_RIGHT));
        assertEquals(SCROLL_RIGHT, mScrollView.getScrollX());

        assertTrue(mScrollView.fullScroll(View.FOCUS_LEFT));
        assertEquals(0, mScrollView.getScrollX());

        assertFalse(mScrollView.fullScroll(View.FOCUS_LEFT));
        assertEquals(0, mScrollView.getScrollX());
    }

    @UiThreadTest
    public void testArrowScroll() {
        mScrollView.setSmoothScrollingEnabled(false);
        assertEquals(0, mScrollView.getScrollX());

        int x = mScrollView.getScrollX();
        while (SCROLL_RIGHT != x) {
            assertTrue(mScrollView.arrowScroll(View.FOCUS_RIGHT));
            assertTrue(x <= mScrollView.getScrollX());
            x = mScrollView.getScrollX();
        }

        assertFalse(mScrollView.arrowScroll(View.FOCUS_RIGHT));
        assertEquals(SCROLL_RIGHT, mScrollView.getScrollX());

        x = mScrollView.getScrollX();
        while (0 != x) {
            assertTrue(mScrollView.arrowScroll(View.FOCUS_LEFT));
            assertTrue(x >= mScrollView.getScrollX());
            x = mScrollView.getScrollX();
        }

        assertFalse(mScrollView.arrowScroll(View.FOCUS_LEFT));
        assertEquals(0, mScrollView.getScrollX());
    }

    public void testSmoothScrollBy() throws Throwable {
        assertEquals(0, mScrollView.getScrollX());
        assertEquals(0, mScrollView.getScrollY());

        runTestOnUiThread(new Runnable() {
            public void run() {
                mScrollView.smoothScrollBy(SCROLL_RIGHT, 0);
            }
        });
        pollingCheckSmoothScrolling(0, SCROLL_RIGHT, 0, 0);
        assertEquals(SCROLL_RIGHT, mScrollView.getScrollX());
        assertEquals(0, mScrollView.getScrollY());

        runTestOnUiThread(new Runnable() {
            public void run() {
                mScrollView.smoothScrollBy(-SCROLL_RIGHT, 0);
            }
        });
        pollingCheckSmoothScrolling(SCROLL_RIGHT, 0, 0, 0);
        assertEquals(0, mScrollView.getScrollX());
        assertEquals(0, mScrollView.getScrollY());
    }

    public void testSmoothScrollTo() throws Throwable {
        assertEquals(0, mScrollView.getScrollX());
        assertEquals(0, mScrollView.getScrollY());

        runTestOnUiThread(new Runnable() {
            public void run() {
                mScrollView.smoothScrollTo(SCROLL_RIGHT, 0);
            }
        });
        pollingCheckSmoothScrolling(0, SCROLL_RIGHT, 0, 0);
        assertEquals(SCROLL_RIGHT, mScrollView.getScrollX());
        assertEquals(0, mScrollView.getScrollY());

        runTestOnUiThread(new Runnable() {
            public void run() {
                mScrollView.smoothScrollTo(0, 0);
            }
        });
        pollingCheckSmoothScrolling(SCROLL_RIGHT, 0, 0, 0);
        assertEquals(0, mScrollView.getScrollX());
        assertEquals(0, mScrollView.getScrollY());
    }

    public void testComputeScrollDeltaToGetChildRectOnScreen() {
        mScrollView.setSmoothScrollingEnabled(false);
        int edge = mScrollView.getHorizontalFadingEdgeLength();

        // Rect's width is smaller than scroll view
        Rect rect = new Rect(0, 0, 0, 0);
        assertEquals(0, mScrollView.computeScrollDeltaToGetChildRectOnScreen(rect));

        rect = new Rect(edge, 0, PAGE_WIDTH, 0);
        assertEquals(0, mScrollView.computeScrollDeltaToGetChildRectOnScreen(rect));

        mScrollView.scrollTo(0, 0);
        rect = new Rect(edge + 1, 0, PAGE_WIDTH, 0);
        assertEquals(edge, mScrollView.computeScrollDeltaToGetChildRectOnScreen(rect));
    }

    public void testComputeHorizontalScrollRange() {
        assertTrue(mScrollView.getChildCount() > 0);
        assertEquals(ITEM_WIDTH * ITEM_COUNT, mScrollView.computeHorizontalScrollRange());

        MyHorizontalScrollView scrollView = new MyHorizontalScrollView(mActivity);
        assertEquals(0, scrollView.getChildCount());
        assertEquals(0, scrollView.computeHorizontalScrollRange());
    }

    @UiThreadTest
    public void testRequestChildFocus() {
        mScrollView.setSmoothScrollingEnabled(false);

        View firstChild = mScrollView.findViewById(R.id.first_horizontal_child);
        View lastChild = mScrollView.findViewById(R.id.last_horizontal_child);
        firstChild.requestFocus();

        int scrollX = mScrollView.getScrollX();
        mScrollView.requestChildFocus(lastChild, lastChild);
        // check scrolling to the child which wants focus
        assertTrue(mScrollView.getScrollX() > scrollX);

        scrollX = mScrollView.getScrollX();
        mScrollView.requestChildFocus(firstChild, firstChild);
        // check scrolling to the child which wants focus
        assertTrue(mScrollView.getScrollX() < scrollX);
    }

    @UiThreadTest
    public void testRequestChildRectangleOnScreen() {
        mScrollView.setSmoothScrollingEnabled(false);
        int edge = mScrollView.getHorizontalFadingEdgeLength();

        View child = mScrollView.findViewById(R.id.first_horizontal_child);
        final Rect originalRect = new Rect(0, 0, 10, 10);
        final Rect newRect = new Rect(ITEM_WIDTH - 10, ITEM_HEIGHT - 10, ITEM_WIDTH, ITEM_HEIGHT);

        assertFalse(mScrollView.requestChildRectangleOnScreen(child, originalRect, true));
        assertEquals(0, mScrollView.getScrollX());
        assertEquals(0, mScrollView.getScrollY());

        assertTrue(mScrollView.requestChildRectangleOnScreen(child, newRect, true));
        assertEquals(ITEM_WIDTH - mScrollView.getWidth() + edge, mScrollView.getScrollX());
        assertEquals(0, mScrollView.getScrollY());
    }

    @UiThreadTest
    public void testRequestLayout() {
        mScrollView.requestLayout();

        assertTrue(mScrollView.isLayoutRequested());
    }

    public void testFling() throws Throwable {
        mScrollView.setSmoothScrollingEnabled(true);
        assertEquals(0, mScrollView.getScrollX());

        final int velocityX = WidgetTestUtils.convertDipToPixels(getActivity(), 2000);

        // fling towards right
        runTestOnUiThread(new Runnable() {
            public void run() {
                mScrollView.fling(velocityX);
            }
        });
        pollingCheckFling(0, true);

        final int currentX = mScrollView.getScrollX();
        // fling towards left
        runTestOnUiThread(new Runnable() {
            public void run() {
                mScrollView.fling(-velocityX);
            }
        });
        pollingCheckFling(currentX, false);
    }

    @UiThreadTest
    public void testScrollTo() {
        mScrollView.setSmoothScrollingEnabled(false);

        mScrollView.scrollTo(10, 10);
        assertEquals(0, mScrollView.getScrollY());
        assertEquals(10, mScrollView.getScrollX());

        mScrollView.scrollTo(PAGE_WIDTH, PAGE_HEIGHT);
        assertEquals(0, mScrollView.getScrollY());
        assertEquals(PAGE_WIDTH, mScrollView.getScrollX());

        mScrollView.scrollTo(SCROLL_RIGHT, 0);
        assertEquals(0, mScrollView.getScrollY());
        assertEquals(SCROLL_RIGHT, mScrollView.getScrollX());

        // reach the top and left
        mScrollView.scrollTo(-10, -10);
        assertEquals(0, mScrollView.getScrollY());
        assertEquals(0, mScrollView.getScrollX());
    }

    public void testGetHorizontalFadingEdgeStrengths() {
        assertTrue(mScrollView.getChildCount() > 0);
        assertTrue(mScrollView.getLeftFadingEdgeStrength() <= 1.0f);
        assertTrue(mScrollView.getLeftFadingEdgeStrength() >= 0.0f);
        assertTrue(mScrollView.getRightFadingEdgeStrength() <= 1.0f);
        assertTrue(mScrollView.getRightFadingEdgeStrength() >= 0.0f);

        MyHorizontalScrollView myScrollView = new MyHorizontalScrollView(mActivity);
        assertEquals(0, myScrollView.getChildCount());
        assertTrue(mScrollView.getLeftFadingEdgeStrength() <= 1.0f);
        assertTrue(mScrollView.getLeftFadingEdgeStrength() >= 0.0f);
        assertTrue(mScrollView.getRightFadingEdgeStrength() <= 1.0f);
        assertTrue(mScrollView.getRightFadingEdgeStrength() >= 0.0f);
    }

    public void testOnLayout() {
        // onLayout() is implementation details, do NOT test
    }

    public void testOnMeasure() {
        // onMeasure() is implementation details, do NOT test
    }

    public void testExecuteKeyEvent() {
        // executeKeyEvent() is implementation details, do NOT test
    }

    public void testOnRequestFocusInDescendants() {
        // onRequestFocusInDescendants() is implementation details, do NOT test
    }

    public void testOnSizeChanged() {
        // onSizeChanged() is implementation details, do NOT test
    }

    public void testDispatchKeyEvent() {
        // dispatchKeyEvent() is implementation details, do NOT test
    }

    public void testOnInterceptTouchEvent() {
        // onInterceptTouchEvent() is implementation details, do NOT test
    }

    public void testOnTouchEvent() {
        // onTouchEvent() is implementation details, do NOT test
    }

    public void testComputeScroll() {
        // computeScroll() is implementation details, do NOT test
    }

    private boolean isInRange(int current, int from, int to) {
        if (from < to) {
            return current >= from && current <= to;
        }
        return current <= from && current >= to;
    }

    private void pollingCheckSmoothScrolling(final int fromX, final int toX,
            final int fromY, final int toY) {

        if (fromX == toX && fromY == toY) {
            return;
        }

        if (fromY != toY) {
            new PollingCheck() {
                @Override
                protected boolean check() {
                    return isInRange(mScrollView.getScrollY(), fromY, toY);
                }
            }.run();
        }

        if (fromX != toX) {
            new PollingCheck() {
                @Override
                protected boolean check() {
                    return isInRange(mScrollView.getScrollX(), fromX, toX);
                }
            }.run();
        }

        new PollingCheck() {
            @Override
            protected boolean check() {
                return toX == mScrollView.getScrollX() && toY == mScrollView.getScrollY();
            }
        }.run();
    }

    private void pollingCheckFling(final int startPosition, final boolean movingRight) {
        new PollingCheck() {
            @Override
            protected boolean check() {
                if (movingRight) {
                    return mScrollView.getScrollX() > startPosition;
                }
                return mScrollView.getScrollX() < startPosition;
            }
        }.run();

        new PollingCheck() {
            private int mPreviousScrollX = mScrollView.getScrollX();

            @Override
            protected boolean check() {
                if (mScrollView.getScrollX() == mPreviousScrollX) {
                    return true;
                } else {
                    mPreviousScrollX = mScrollView.getScrollX();
                    return false;
                }
            }
        }.run();
    }

    public static class MyView extends View {
        // measure in this height if set
        private Integer mDesiredWidth;
        public MyView(Context context) {
            super(context);
        }

        public void setDesiredWidth(Integer desiredWidth) {
            mDesiredWidth = desiredWidth;
        }

        @Override
        protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
            super.onMeasure(widthMeasureSpec, heightMeasureSpec);
            if (mDesiredWidth != null) {
                int mode = MeasureSpec.getMode(widthMeasureSpec);
                int size = MeasureSpec.getSize(widthMeasureSpec);
                int newWidth = size;
                if (mode == MeasureSpec.AT_MOST) {
                    newWidth = Math.max(size, mDesiredWidth);
                } else if (mode == MeasureSpec.UNSPECIFIED) {
                    newWidth = mDesiredWidth;
                }
                setMeasuredDimension(newWidth, getMeasuredHeight());
            }
        }
    }
}
