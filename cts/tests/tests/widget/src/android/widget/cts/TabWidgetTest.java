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
import android.content.Context;
import android.graphics.drawable.Drawable;
import android.test.ActivityInstrumentationTestCase2;
import android.test.UiThreadTest;
import android.test.suitebuilder.annotation.SmallTest;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.LinearLayout;
import android.widget.LinearLayout.LayoutParams;
import android.widget.ListView;
import android.widget.RelativeLayout;
import android.widget.TabWidget;
import android.widget.TextView;
import android.widget.cts.util.TestUtils;

/**
 * Test {@link TabWidget}.
 */
@SmallTest
public class TabWidgetTest extends ActivityInstrumentationTestCase2<TabHostCtsActivity> {
    private Activity mActivity;

    public TabWidgetTest() {
        super("android.widget.cts", TabHostCtsActivity.class);
    }

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mActivity = getActivity();
    }

    public void testConstructor() {
        new TabWidget(mActivity);

        new TabWidget(mActivity, null);

        new TabWidget(mActivity, null, 0);
    }

    public void testConstructorWithStyle() {
        TabWidget tabWidget = new TabWidget(mActivity, null, 0, R.style.TabWidgetCustomStyle);

        assertFalse(tabWidget.isStripEnabled());

        Drawable leftStripDrawable = tabWidget.getLeftStripDrawable();
        assertNotNull(leftStripDrawable);
        TestUtils.assertAllPixelsOfColor("Left strip green", leftStripDrawable,
                leftStripDrawable.getIntrinsicWidth(), leftStripDrawable.getIntrinsicHeight(),
                true, 0xFF00FF00, 1, false);

        Drawable rightStripDrawable = tabWidget.getRightStripDrawable();
        assertNotNull(rightStripDrawable);
        TestUtils.assertAllPixelsOfColor("Right strip red", rightStripDrawable,
                rightStripDrawable.getIntrinsicWidth(), rightStripDrawable.getIntrinsicHeight(),
                true, 0xFFFF0000, 1, false);
    }

    public void testInflateFromXml() {
        LayoutInflater inflater = LayoutInflater.from(mActivity);
        TabWidget tabWidget = (TabWidget) inflater.inflate(R.layout.tabhost_custom, null, false);

        assertFalse(tabWidget.isStripEnabled());

        Drawable leftStripDrawable = tabWidget.getLeftStripDrawable();
        assertNotNull(leftStripDrawable);
        TestUtils.assertAllPixelsOfColor("Left strip red", leftStripDrawable,
                leftStripDrawable.getIntrinsicWidth(), leftStripDrawable.getIntrinsicHeight(),
                true, 0xFFFF0000, 1, false);

        Drawable rightStripDrawable = tabWidget.getRightStripDrawable();
        assertNotNull(rightStripDrawable);
        TestUtils.assertAllPixelsOfColor("Right strip green", rightStripDrawable,
                rightStripDrawable.getIntrinsicWidth(), rightStripDrawable.getIntrinsicHeight(),
                true, 0xFF00FF00, 1, false);
    }

    @UiThreadTest
    public void testTabCount() {
        TabHostCtsActivity activity = getActivity();
        TabWidget tabWidget = activity.getTabWidget();

        // We have one tab added in onCreate() of our activity
        assertEquals(1, tabWidget.getTabCount());

        for (int i = 1; i < 10; i++) {
            tabWidget.addView(new TextView(mActivity));
            assertEquals(i + 1, tabWidget.getTabCount());
        }
    }

    @UiThreadTest
    public void testTabViews() {
        TabHostCtsActivity activity = getActivity();
        TabWidget tabWidget = activity.getTabWidget();

        // We have one tab added in onCreate() of our activity. We "reach" into the default tab
        // indicator layout in the same way we do in TabHost_TabSpecTest tests.
        TextView tab0 = (TextView) tabWidget.getChildTabViewAt(0).findViewById(android.R.id.title);
        assertNotNull(tab0);
        assertEquals(TabHostCtsActivity.INITIAL_TAB_LABEL, tab0.getText());

        for (int i = 1; i < 10; i++) {
            TextView toAdd = new TextView(mActivity);
            toAdd.setText("Tab #" + i);
            tabWidget.addView(toAdd);
            assertEquals(toAdd, tabWidget.getChildTabViewAt(i));
        }
    }

    public void testChildDrawableStateChanged() {
        MockTabWidget mockTabWidget = new MockTabWidget(mActivity);
        TextView tv0 = new TextView(mActivity);
        TextView tv1 = new TextView(mActivity);
        mockTabWidget.addView(tv0);
        mockTabWidget.addView(tv1);
        mockTabWidget.setCurrentTab(1);
        mockTabWidget.reset();
        mockTabWidget.childDrawableStateChanged(tv0);
        assertFalse(mockTabWidget.hasCalledInvalidate());

        mockTabWidget.reset();
        mockTabWidget.childDrawableStateChanged(tv1);
        assertTrue(mockTabWidget.hasCalledInvalidate());

        mockTabWidget.reset();
        mockTabWidget.childDrawableStateChanged(null);
        assertFalse(mockTabWidget.hasCalledInvalidate());
    }

    public void testDispatchDraw() {
        // implementation details
    }

    @UiThreadTest
    public void testSetCurrentTab() {
        TabHostCtsActivity activity = getActivity();
        TabWidget tabWidget = activity.getTabWidget();
        tabWidget.addView(new TextView(mActivity));

        assertTrue(tabWidget.getChildAt(0).isSelected());
        assertFalse(tabWidget.getChildAt(1).isSelected());
        assertTrue(tabWidget.getChildAt(0).isFocused());
        assertFalse(tabWidget.getChildAt(1).isFocused());

        tabWidget.setCurrentTab(1);
        assertFalse(tabWidget.getChildAt(0).isSelected());
        assertTrue(tabWidget.getChildAt(1).isSelected());
        assertTrue(tabWidget.getChildAt(0).isFocused());
        assertFalse(tabWidget.getChildAt(1).isFocused());
    }

    @UiThreadTest
    public void testFocusCurrentTab() {
        TabHostCtsActivity activity = getActivity();
        TabWidget tabWidget = activity.getTabWidget();
        tabWidget.addView(new TextView(mActivity));

        assertTrue(tabWidget.getChildAt(0).isSelected());
        assertFalse(tabWidget.getChildAt(1).isSelected());
        assertEquals(tabWidget.getChildAt(0), tabWidget.getFocusedChild());
        assertTrue(tabWidget.getChildAt(0).isFocused());
        assertFalse(tabWidget.getChildAt(1).isFocused());

        // normal
        tabWidget.focusCurrentTab(1);
        assertFalse(tabWidget.getChildAt(0).isSelected());
        assertTrue(tabWidget.getChildAt(1).isSelected());
        assertEquals(tabWidget.getChildAt(1), tabWidget.getFocusedChild());
        assertFalse(tabWidget.getChildAt(0).isFocused());
        assertTrue(tabWidget.getChildAt(1).isFocused());

        tabWidget.focusCurrentTab(0);
        assertTrue(tabWidget.getChildAt(0).isSelected());
        assertFalse(tabWidget.getChildAt(1).isSelected());
        assertEquals(tabWidget.getChildAt(0), tabWidget.getFocusedChild());
        assertTrue(tabWidget.getChildAt(0).isFocused());
        assertFalse(tabWidget.getChildAt(1).isFocused());

        // exceptional
        try {
            tabWidget.focusCurrentTab(-1);
            fail("Should throw NullPointerException");
        } catch (NullPointerException e) {
            // expected exception
        }

        try {
            tabWidget.focusCurrentTab(tabWidget.getChildCount() + 1);
            fail("Should throw NullPointerException");
        } catch (NullPointerException e) {
            // expected exception
        }
    }

    @UiThreadTest
    public void testSetEnabled() {
        TabHostCtsActivity activity = getActivity();
        TabWidget tabWidget = activity.getTabWidget();

        tabWidget.addView(new TextView(mActivity));
        tabWidget.addView(new TextView(mActivity));
        assertTrue(tabWidget.isEnabled());
        assertTrue(tabWidget.getChildAt(0).isEnabled());
        assertTrue(tabWidget.getChildAt(1).isEnabled());

        tabWidget.setEnabled(false);
        assertFalse(tabWidget.isEnabled());
        assertFalse(tabWidget.getChildAt(0).isEnabled());
        assertFalse(tabWidget.getChildAt(1).isEnabled());

        tabWidget.setEnabled(true);
        assertTrue(tabWidget.isEnabled());
        assertTrue(tabWidget.getChildAt(0).isEnabled());
        assertTrue(tabWidget.getChildAt(1).isEnabled());
    }

    public void testAddView() {
        MockTabWidget mockTabWidget = new MockTabWidget(mActivity);

        // normal value
        View view1 = new TextView(mActivity);
        mockTabWidget.addView(view1);
        assertSame(view1, mockTabWidget.getChildAt(0));
        LayoutParams defaultLayoutParam = mockTabWidget.generateDefaultLayoutParams();
        if (mockTabWidget.getOrientation() == LinearLayout.VERTICAL) {
            assertEquals(defaultLayoutParam.height, LayoutParams.WRAP_CONTENT);
            assertEquals(defaultLayoutParam.width, LayoutParams.MATCH_PARENT);
        } else if (mockTabWidget.getOrientation() == LinearLayout.HORIZONTAL) {
            assertEquals(defaultLayoutParam.height, LayoutParams.WRAP_CONTENT);
            assertEquals(defaultLayoutParam.width, LayoutParams.WRAP_CONTENT);
        } else {
            assertNull(defaultLayoutParam);
        }

        View view2 = new RelativeLayout(mActivity);
        mockTabWidget.addView(view2);
        assertSame(view2, mockTabWidget.getChildAt(1));

        try {
            mockTabWidget.addView(new ListView(mActivity));
            fail("did not throw RuntimeException when adding invalid view");
        } catch (RuntimeException e) {
            // issue 1695243
        }

        try {
            mockTabWidget.addView(null);
            fail("did not throw NullPointerException when child is null");
        } catch (NullPointerException e) {
            // issue 1695243
        }
    }

    @UiThreadTest
    public void testStripEnabled() {
        TabHostCtsActivity activity = getActivity();
        TabWidget tabWidget = activity.getTabWidget();

        tabWidget.setStripEnabled(true);
        assertTrue(tabWidget.isStripEnabled());

        tabWidget.setStripEnabled(false);
        assertFalse(tabWidget.isStripEnabled());
    }

    @UiThreadTest
    public void testStripDrawables() {
        TabHostCtsActivity activity = getActivity();
        TabWidget tabWidget = activity.getTabWidget();

        // Test setting left strip drawable
        tabWidget.setLeftStripDrawable(R.drawable.icon_green);
        Drawable leftStripDrawable = tabWidget.getLeftStripDrawable();
        assertNotNull(leftStripDrawable);
        TestUtils.assertAllPixelsOfColor("Left strip green", leftStripDrawable,
                leftStripDrawable.getIntrinsicWidth(), leftStripDrawable.getIntrinsicHeight(),
                true, 0xFF00FF00, 1, false);

        tabWidget.setLeftStripDrawable(activity.getResources().getDrawable(
                R.drawable.icon_red, null));
        leftStripDrawable = tabWidget.getLeftStripDrawable();
        assertNotNull(leftStripDrawable);
        TestUtils.assertAllPixelsOfColor("Left strip red", leftStripDrawable,
                leftStripDrawable.getIntrinsicWidth(), leftStripDrawable.getIntrinsicHeight(),
                true, 0xFFFF0000, 1, false);

        // Test setting right strip drawable
        tabWidget.setRightStripDrawable(R.drawable.icon_red);
        Drawable rightStripDrawable = tabWidget.getRightStripDrawable();
        assertNotNull(rightStripDrawable);
        TestUtils.assertAllPixelsOfColor("Right strip red", rightStripDrawable,
                rightStripDrawable.getIntrinsicWidth(), rightStripDrawable.getIntrinsicHeight(),
                true, 0xFFFF0000, 1, false);

        tabWidget.setRightStripDrawable(activity.getResources().getDrawable(
                R.drawable.icon_green, null));
        rightStripDrawable = tabWidget.getRightStripDrawable();
        assertNotNull(rightStripDrawable);
        TestUtils.assertAllPixelsOfColor("Left strip green", rightStripDrawable,
                rightStripDrawable.getIntrinsicWidth(), rightStripDrawable.getIntrinsicHeight(),
                true, 0xFF00FF00, 1, false);
    }

    @UiThreadTest
    public void testDividerDrawables() {
        TabHostCtsActivity activity = getActivity();
        TabWidget tabWidget = activity.getTabWidget();

        tabWidget.setDividerDrawable(R.drawable.icon_blue);
        Drawable dividerDrawable = tabWidget.getDividerDrawable();
        assertNotNull(dividerDrawable);
        TestUtils.assertAllPixelsOfColor("Divider blue", dividerDrawable,
                dividerDrawable.getIntrinsicWidth(), dividerDrawable.getIntrinsicHeight(),
                true, 0xFF0000FF, 1, false);

        tabWidget.setDividerDrawable(activity.getResources().getDrawable(
                R.drawable.icon_yellow, null));
        dividerDrawable = tabWidget.getDividerDrawable();
        assertNotNull(dividerDrawable);
        TestUtils.assertAllPixelsOfColor("Divider yellow", dividerDrawable,
                dividerDrawable.getIntrinsicWidth(), dividerDrawable.getIntrinsicHeight(),
                true, 0xFFFFFF00, 1, false);

    }

    public void testOnFocusChange() {
        // onFocusChange() is implementation details, do NOT test
    }

    public void testOnSizeChanged() {
        // implementation details
    }

    /*
     * Mock class for TabWidget to be used in test cases.
     */
    private class MockTabWidget extends TabWidget {
        private boolean mCalledInvalidate = false;

        public MockTabWidget(Context context) {
            super(context);
        }

        @Override
        protected LayoutParams generateDefaultLayoutParams() {
            return super.generateDefaultLayoutParams();
        }

        @Override
        public void invalidate() {
            super.invalidate();
            mCalledInvalidate = true;
        }

        public boolean hasCalledInvalidate() {
            return mCalledInvalidate;
        }

        public void reset() {
            mCalledInvalidate = false;
        }
    }
}
