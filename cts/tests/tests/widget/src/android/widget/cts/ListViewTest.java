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

import android.widget.BaseAdapter;
import android.widget.LinearLayout;
import android.widget.cts.R;

import org.xmlpull.v1.XmlPullParser;

import android.app.Activity;
import android.app.Instrumentation;
import android.content.Context;
import android.cts.util.PollingCheck;
import android.graphics.Canvas;
import android.graphics.Rect;
import android.graphics.drawable.Drawable;
import android.test.ActivityInstrumentationTestCase2;
import android.test.UiThreadTest;
import android.test.suitebuilder.annotation.MediumTest;
import android.util.AttributeSet;
import android.util.Pair;
import android.util.SparseBooleanArray;
import android.util.Xml;
import android.view.KeyEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.animation.LayoutAnimationController;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.ArrayAdapter;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.cts.util.ViewTestUtils;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

import junit.framework.Assert;

import static org.mockito.Mockito.*;

public class ListViewTest extends ActivityInstrumentationTestCase2<ListViewCtsActivity> {
    private final String[] mCountryList = new String[] {
        "Argentina", "Australia", "China", "France", "Germany", "Italy", "Japan", "United States"
    };
    private final String[] mNameList = new String[] {
        "Jacky", "David", "Kevin", "Michael", "Andy"
    };
    private final String[] mEmptyList = new String[0];

    private ListView mListView;
    private Activity mActivity;
    private Instrumentation mInstrumentation;
    private AttributeSet mAttributeSet;
    private ArrayAdapter<String> mAdapter_countries;
    private ArrayAdapter<String> mAdapter_names;
    private ArrayAdapter<String> mAdapter_empty;

    public ListViewTest() {
        super("android.widget.cts", ListViewCtsActivity.class);
    }

    protected void setUp() throws Exception {
        super.setUp();

        mActivity = getActivity();
        mInstrumentation = getInstrumentation();
        XmlPullParser parser = mActivity.getResources().getXml(R.layout.listview_layout);
        mAttributeSet = Xml.asAttributeSet(parser);

        mAdapter_countries = new ArrayAdapter<String>(mActivity,
                android.R.layout.simple_list_item_1, mCountryList);
        mAdapter_names = new ArrayAdapter<String>(mActivity, android.R.layout.simple_list_item_1,
                mNameList);
        mAdapter_empty = new ArrayAdapter<String>(mActivity, android.R.layout.simple_list_item_1,
                mEmptyList);

        mListView = (ListView) mActivity.findViewById(R.id.listview_default);
    }

    public void testConstructor() {
        new ListView(mActivity);
        new ListView(mActivity, mAttributeSet);
        new ListView(mActivity, mAttributeSet, 0);

        try {
            new ListView(null);
            fail("There should be a NullPointerException thrown out. ");
        } catch (NullPointerException e) {
            // expected, test success.
        }

        try {
            new ListView(null, null);
            fail("There should be a NullPointerException thrown out. ");
        } catch (NullPointerException e) {
            // expected, test success.
        }

        try {
            new ListView(null, null, -1);
            fail("There should be a NullPointerException thrown out. ");
        } catch (NullPointerException e) {
            // expected, test success.
        }
    }

    public void testGetMaxScrollAmount() {
        setAdapter(mAdapter_empty);
        int scrollAmount = mListView.getMaxScrollAmount();
        assertEquals(0, scrollAmount);

        setAdapter(mAdapter_names);
        scrollAmount = mListView.getMaxScrollAmount();
        assertTrue(scrollAmount > 0);
    }

    private void setAdapter(final ArrayAdapter<String> adapter) {
        ViewTestUtils.runOnMainAndDrawSync(mInstrumentation, mListView,
                () -> mListView.setAdapter(adapter));
    }

    public void testAccessDividerHeight() {
        ViewTestUtils.runOnMainAndDrawSync(mInstrumentation, mListView,
                () -> mListView.setAdapter(mAdapter_countries));

        Drawable d = mListView.getDivider();
        final Rect r = d.getBounds();
        new PollingCheck() {
            @Override
            protected boolean check() {
                return r.bottom - r.top > 0;
            }
        }.run();

        ViewTestUtils.runOnMainAndDrawSync(mInstrumentation, mListView,
                () -> mListView.setDividerHeight(20));

        assertEquals(20, mListView.getDividerHeight());
        assertEquals(20, r.bottom - r.top);

        ViewTestUtils.runOnMainAndDrawSync(mInstrumentation, mListView,
                () -> mListView.setDividerHeight(10));

        assertEquals(10, mListView.getDividerHeight());
        assertEquals(10, r.bottom - r.top);
    }

    public void testAccessItemsCanFocus() {
        mListView.setItemsCanFocus(true);
        assertTrue(mListView.getItemsCanFocus());

        mListView.setItemsCanFocus(false);
        assertFalse(mListView.getItemsCanFocus());

        // TODO: how to check?
    }

    public void testAccessAdapter() {
        ViewTestUtils.runOnMainAndDrawSync(mInstrumentation, mListView,
                () -> mListView.setAdapter(mAdapter_countries));

        assertSame(mAdapter_countries, mListView.getAdapter());
        assertEquals(mCountryList.length, mListView.getCount());

        ViewTestUtils.runOnMainAndDrawSync(mInstrumentation, mListView,
                () -> mListView.setAdapter(mAdapter_names));

        assertSame(mAdapter_names, mListView.getAdapter());
        assertEquals(mNameList.length, mListView.getCount());
    }

    @UiThreadTest
    public void testAccessItemChecked() {
        // NONE mode
        mListView.setChoiceMode(ListView.CHOICE_MODE_NONE);
        assertEquals(ListView.CHOICE_MODE_NONE, mListView.getChoiceMode());

        mListView.setItemChecked(1, true);
        assertEquals(ListView.INVALID_POSITION, mListView.getCheckedItemPosition());
        assertFalse(mListView.isItemChecked(1));

        // SINGLE mode
        mListView.setChoiceMode(ListView.CHOICE_MODE_SINGLE);
        assertEquals(ListView.CHOICE_MODE_SINGLE, mListView.getChoiceMode());

        mListView.setItemChecked(2, true);
        assertEquals(2, mListView.getCheckedItemPosition());
        assertTrue(mListView.isItemChecked(2));

        mListView.setItemChecked(3, true);
        assertEquals(3, mListView.getCheckedItemPosition());
        assertTrue(mListView.isItemChecked(3));
        assertFalse(mListView.isItemChecked(2));

        // test attempt to uncheck a item that wasn't checked to begin with
        mListView.setItemChecked(4, false);
        // item three should still be checked
        assertEquals(3, mListView.getCheckedItemPosition());
        assertFalse(mListView.isItemChecked(4));
        assertTrue(mListView.isItemChecked(3));
        assertFalse(mListView.isItemChecked(2));

        mListView.setItemChecked(4, true);
        assertTrue(mListView.isItemChecked(4));
        mListView.clearChoices();
        assertEquals(ListView.INVALID_POSITION, mListView.getCheckedItemPosition());
        assertFalse(mListView.isItemChecked(4));

        // MULTIPLE mode
        mListView.setChoiceMode(ListView.CHOICE_MODE_MULTIPLE);
        assertEquals(ListView.CHOICE_MODE_MULTIPLE, mListView.getChoiceMode());

        mListView.setItemChecked(1, true);
        assertEquals(ListView.INVALID_POSITION, mListView.getCheckedItemPosition());
        SparseBooleanArray array = mListView.getCheckedItemPositions();
        assertTrue(array.get(1));
        assertFalse(array.get(2));
        assertTrue(mListView.isItemChecked(1));
        assertFalse(mListView.isItemChecked(2));

        mListView.setItemChecked(2, true);
        mListView.setItemChecked(3, false);
        mListView.setItemChecked(4, true);

        assertTrue(array.get(1));
        assertTrue(array.get(2));
        assertFalse(array.get(3));
        assertTrue(array.get(4));
        assertTrue(mListView.isItemChecked(1));
        assertTrue(mListView.isItemChecked(2));
        assertFalse(mListView.isItemChecked(3));
        assertTrue(mListView.isItemChecked(4));

        mListView.clearChoices();
        assertFalse(array.get(1));
        assertFalse(array.get(2));
        assertFalse(array.get(3));
        assertFalse(array.get(4));
        assertFalse(mListView.isItemChecked(1));
        assertFalse(mListView.isItemChecked(2));
        assertFalse(mListView.isItemChecked(3));
        assertFalse(mListView.isItemChecked(4));
    }

    public void testAccessFooterView() {
        final TextView footerView1 = new TextView(mActivity);
        footerView1.setText("footerview1");
        final TextView footerView2 = new TextView(mActivity);
        footerView2.setText("footerview2");

        mInstrumentation.runOnMainSync(() -> mListView.setFooterDividersEnabled(true));
        assertEquals(0, mListView.getFooterViewsCount());

        mInstrumentation.runOnMainSync(() -> mListView.addFooterView(footerView1, null, true));
        assertEquals(1, mListView.getFooterViewsCount());

        mInstrumentation.runOnMainSync(() -> mListView.addFooterView(footerView2));

        mInstrumentation.waitForIdleSync();
        assertEquals(2, mListView.getFooterViewsCount());

        ViewTestUtils.runOnMainAndDrawSync(mInstrumentation, mListView,
                () -> mListView.setAdapter(mAdapter_countries));

        ViewTestUtils.runOnMainAndDrawSync(mInstrumentation, mListView,
                () -> mListView.removeFooterView(footerView1));
        assertEquals(1, mListView.getFooterViewsCount());

        ViewTestUtils.runOnMainAndDrawSync(mInstrumentation, mListView,
                () -> mListView.removeFooterView(footerView2));
        assertEquals(0, mListView.getFooterViewsCount());
    }

    public void testAccessHeaderView() {
        final TextView headerView1 = (TextView) mActivity.findViewById(R.id.headerview1);
        final TextView headerView2 = (TextView) mActivity.findViewById(R.id.headerview2);

        mInstrumentation.runOnMainSync(() -> mListView.setHeaderDividersEnabled(true));
        assertEquals(0, mListView.getHeaderViewsCount());

        mInstrumentation.runOnMainSync(() -> mListView.addHeaderView(headerView2, null, true));
        assertEquals(1, mListView.getHeaderViewsCount());

        mInstrumentation.runOnMainSync(() -> mListView.addHeaderView(headerView1));
        assertEquals(2, mListView.getHeaderViewsCount());
    }

    public void testHeaderFooterType() throws Throwable {
        final TextView headerView = new TextView(getActivity());
        final List<Pair<View, View>> mismatch = new ArrayList<Pair<View, View>>();
        final ArrayAdapter adapter = new ArrayAdapter<String>(mActivity,
                android.R.layout.simple_list_item_1, mNameList) {
            @Override
            public int getItemViewType(int position) {
                return position == 0 ? AdapterView.ITEM_VIEW_TYPE_HEADER_OR_FOOTER :
                        super.getItemViewType(position - 1);
            }

            @Override
            public View getView(int position, View convertView, ViewGroup parent) {
                if (position == 0) {
                    if (convertView != null && convertView != headerView) {
                        mismatch.add(new Pair<View, View>(headerView, convertView));
                    }
                    return headerView;
                } else {
                    return super.getView(position - 1, convertView, parent);
                }
            }

            @Override
            public int getCount() {
                return super.getCount() + 1;
            }
        };
        ViewTestUtils.runOnMainAndDrawSync(mInstrumentation, mListView,
                () -> mListView.setAdapter(adapter));

        ViewTestUtils.runOnMainAndDrawSync(mInstrumentation, mListView,
                () -> adapter.notifyDataSetChanged());

        assertEquals(0, mismatch.size());
    }

    public void testAccessDivider() {
        ViewTestUtils.runOnMainAndDrawSync(mInstrumentation, mListView,
                () -> mListView.setAdapter(mAdapter_countries));

        Drawable defaultDrawable = mListView.getDivider();
        final Rect r = defaultDrawable.getBounds();
        new PollingCheck() {
            @Override
            protected boolean check() {
                return r.bottom - r.top > 0;
            }
        }.run();

        final Drawable d = mActivity.getResources().getDrawable(R.drawable.scenery);

        ViewTestUtils.runOnMainAndDrawSync(mInstrumentation, mListView,
                () -> mListView.setDivider(d));
        assertSame(d, mListView.getDivider());
        assertEquals(d.getBounds().height(), mListView.getDividerHeight());

        ViewTestUtils.runOnMainAndDrawSync(mInstrumentation, mListView,
                () -> mListView.setDividerHeight(10));
        assertEquals(10, mListView.getDividerHeight());
        assertEquals(10, d.getBounds().height());
    }

    public void testSetSelection() {
        ViewTestUtils.runOnMainAndDrawSync(mInstrumentation, mListView,
                () -> mListView.setAdapter(mAdapter_countries));

        ViewTestUtils.runOnMainAndDrawSync(mInstrumentation, mListView,
                () -> mListView.setSelection(1));
        String item = (String) mListView.getSelectedItem();
        assertEquals(mCountryList[1], item);

        ViewTestUtils.runOnMainAndDrawSync(mInstrumentation, mListView,
                () -> mListView.setSelectionFromTop(5, 0));
        item = (String) mListView.getSelectedItem();
        assertEquals(mCountryList[5], item);

        ViewTestUtils.runOnMainAndDrawSync(mInstrumentation, mListView,
                () -> mListView.setSelectionAfterHeaderView());
        item = (String) mListView.getSelectedItem();
        assertEquals(mCountryList[0], item);
    }

    public void testOnKeyUpDown() {
        // implementation details, do NOT test
    }

    public void testPerformItemClick() {
        ViewTestUtils.runOnMainAndDrawSync(mInstrumentation, mListView,
                () -> mListView.setAdapter(mAdapter_countries));

        mListView.setChoiceMode(ListView.CHOICE_MODE_SINGLE);
        ViewTestUtils.runOnMainAndDrawSync(mInstrumentation, mListView,
                () -> mListView.setSelection(2));

        final TextView child = (TextView) mAdapter_countries.getView(2, null, mListView);
        assertNotNull(child);
        assertEquals(mCountryList[2], child.getText().toString());
        final long itemID = mAdapter_countries.getItemId(2);
        assertEquals(2, itemID);

        mInstrumentation.runOnMainSync(() -> mListView.performItemClick(child, 2, itemID));
        mInstrumentation.waitForIdleSync();

        OnItemClickListener onClickListener = mock(OnItemClickListener.class);
        mListView.setOnItemClickListener(onClickListener);
        verify(onClickListener, never()).onItemClick(any(AdapterView.class), any(View.class),
                anyInt(), anyLong());

        mInstrumentation.runOnMainSync(() -> mListView.performItemClick(child, 2, itemID));
        mInstrumentation.waitForIdleSync();

        verify(onClickListener, times(1)).onItemClick(mListView, child, 2, 2L);
        verifyNoMoreInteractions(onClickListener);
    }

    public void testSaveAndRestoreInstanceState() {
        // implementation details, do NOT test
    }

    public void testDispatchKeyEvent() {
        ViewTestUtils.runOnMainAndDrawSync(mInstrumentation, mListView,
                () -> {
                    mListView.setAdapter(mAdapter_countries);
                    mListView.requestFocus();
                });
        assertTrue(mListView.hasFocus());

        ViewTestUtils.runOnMainAndDrawSync(mInstrumentation, mListView,
                () -> mListView.setSelection(1));
        String item = (String) mListView.getSelectedItem();
        assertEquals(mCountryList[1], item);

        ViewTestUtils.runOnMainAndDrawSync(mInstrumentation, mListView,
                () ->  {
                    KeyEvent keyEvent = new KeyEvent(KeyEvent.ACTION_DOWN, KeyEvent.KEYCODE_A);
                    mListView.dispatchKeyEvent(keyEvent);
                });

        ViewTestUtils.runOnMainAndDrawSync(mInstrumentation, mListView,
                () -> {
                    KeyEvent keyEvent = new KeyEvent(KeyEvent.ACTION_DOWN,
                            KeyEvent.KEYCODE_DPAD_DOWN);
                    mListView.dispatchKeyEvent(keyEvent);
                    mListView.dispatchKeyEvent(keyEvent);
                    mListView.dispatchKeyEvent(keyEvent);
                });
        item = (String)mListView.getSelectedItem();
        assertEquals(mCountryList[4], item);
    }

    public void testRequestChildRectangleOnScreen() {
        ViewTestUtils.runOnMainAndDrawSync(mInstrumentation, mListView,
                () -> mListView.setAdapter(mAdapter_countries));

        TextView child = (TextView) mAdapter_countries.getView(0, null, mListView);
        assertNotNull(child);
        assertEquals(mCountryList[0], child.getText().toString());

        Rect rect = new Rect(0, 0, 10, 10);
        assertFalse(mListView.requestChildRectangleOnScreen(child, rect, false));

        // TODO: how to check?
    }

    public void testOnTouchEvent() {
        // implementation details, do NOT test
    }

    @UiThreadTest
    public void testCanAnimate() {
        MyListView listView = new MyListView(mActivity, mAttributeSet);

        assertFalse(listView.canAnimate());
        listView.setAdapter(mAdapter_countries);
        assertFalse(listView.canAnimate());

        LayoutAnimationController controller = new LayoutAnimationController(
                mActivity, mAttributeSet);
        listView.setLayoutAnimation(controller);

        assertTrue(listView.canAnimate());
    }

    @UiThreadTest
    public void testDispatchDraw() {
        // implementation details, do NOT test
    }

    @UiThreadTest
    public void testFindViewTraversal() {
        MyListView listView = new MyListView(mActivity, mAttributeSet);
        TextView headerView = (TextView) mActivity.findViewById(R.id.headerview1);

        assertNull(listView.findViewTraversal(R.id.headerview1));

        listView.addHeaderView(headerView);
        assertNotNull(listView.findViewTraversal(R.id.headerview1));
        assertSame(headerView, listView.findViewTraversal(R.id.headerview1));
    }

    @UiThreadTest
    public void testFindViewWithTagTraversal() {
        MyListView listView = new MyListView(mActivity, mAttributeSet);
        TextView headerView = (TextView) mActivity.findViewById(R.id.headerview1);

        assertNull(listView.findViewWithTagTraversal("header"));

        headerView.setTag("header");
        listView.addHeaderView(headerView);
        assertNotNull(listView.findViewWithTagTraversal("header"));
        assertSame(headerView, listView.findViewWithTagTraversal("header"));
    }

    public void testLayoutChildren() {
        // TODO: how to test?
    }

    public void testOnFinishInflate() {
        // implementation details, do NOT test
    }

    public void testOnFocusChanged() {
        // implementation details, do NOT test
    }

    public void testOnMeasure() {
        // implementation details, do NOT test
    }

    /**
     * MyListView for test
     */
    private static class MyListView extends ListView {
        public MyListView(Context context, AttributeSet attrs) {
            super(context, attrs);
        }

        @Override
        protected boolean canAnimate() {
            return super.canAnimate();
        }

        @Override
        protected void dispatchDraw(Canvas canvas) {
            super.dispatchDraw(canvas);
        }

        @Override
        protected View findViewTraversal(int id) {
            return super.findViewTraversal(id);
        }

        @Override
        protected View findViewWithTagTraversal(Object tag) {
            return super.findViewWithTagTraversal(tag);
        }

        @Override
        protected void layoutChildren() {
            super.layoutChildren();
        }
    }

    /**
     * The following functions are merged from frameworktest.
     */
    @MediumTest
    public void testRequestLayoutCallsMeasure() throws Exception {
        ListView listView = new ListView(mActivity);
        List<String> items = new ArrayList<>();
        items.add("hello");
        Adapter<String> adapter = new Adapter<String>(mActivity, 0, items);
        listView.setAdapter(adapter);

        int measureSpec = View.MeasureSpec.makeMeasureSpec(100, View.MeasureSpec.EXACTLY);

        adapter.notifyDataSetChanged();
        listView.measure(measureSpec, measureSpec);
        listView.layout(0, 0, 100, 100);

        MockView childView = (MockView) listView.getChildAt(0);

        childView.requestLayout();
        childView.onMeasureCalled = false;
        listView.measure(measureSpec, measureSpec);
        listView.layout(0, 0, 100, 100);
        Assert.assertTrue(childView.onMeasureCalled);
    }

    @MediumTest
    public void testNoSelectableItems() throws Exception {
        ListView listView = new ListView(mActivity);
        // We use a header as the unselectable item to remain after the selectable one is removed.
        listView.addHeaderView(new View(mActivity), null, false);
        List<String> items = new ArrayList<>();
        items.add("hello");
        Adapter<String> adapter = new Adapter<String>(mActivity, 0, items);
        listView.setAdapter(adapter);

        listView.setSelection(1);

        int measureSpec = View.MeasureSpec.makeMeasureSpec(100, View.MeasureSpec.EXACTLY);

        adapter.notifyDataSetChanged();
        listView.measure(measureSpec, measureSpec);
        listView.layout(0, 0, 100, 100);

        items.remove(0);

        adapter.notifyDataSetChanged();
        listView.measure(measureSpec, measureSpec);
        listView.layout(0, 0, 100, 100);
    }

    @MediumTest
    public void testFullDetachHeaderViewOnScroll() {
        final AttachDetachAwareView header = new AttachDetachAwareView(mActivity);
        ViewTestUtils.runOnMainAndDrawSync(getInstrumentation(), mListView, () -> {
            mListView.setAdapter(new DummyAdapter(1000));
            mListView.addHeaderView(header);
        });
        assertEquals("test sanity", 1, header.mOnAttachCount);
        assertEquals("test sanity", 0, header.mOnDetachCount);
        ViewTestUtils.runOnMainAndDrawSync(getInstrumentation(), mListView, () -> {
            mListView.scrollListBy(mListView.getHeight() * 3);
        });
        assertNull("test sanity, header should be removed", header.getParent());
        assertEquals("header view should be detached", 1, header.mOnDetachCount);
        assertFalse(header.isTemporarilyDetached());
    }

    @MediumTest
    public void testFullDetachHeaderViewOnRelayout() {
        final AttachDetachAwareView header = new AttachDetachAwareView(mActivity);
        ViewTestUtils.runOnMainAndDrawSync(getInstrumentation(), mListView, () -> {
            mListView.setAdapter(new DummyAdapter(1000));
            mListView.addHeaderView(header);
        });
        assertEquals("test sanity", 1, header.mOnAttachCount);
        assertEquals("test sanity", 0, header.mOnDetachCount);
        ViewTestUtils.runOnMainAndDrawSync(getInstrumentation(), mListView, () -> {
            mListView.setSelection(800);
        });
        assertNull("test sanity, header should be removed", header.getParent());
        assertEquals("header view should be detached", 1, header.mOnDetachCount);
        assertFalse(header.isTemporarilyDetached());
    }

    @MediumTest
    public void testFullDetachHeaderViewOnScrollForFocus() {
        final AttachDetachAwareView header = new AttachDetachAwareView(mActivity);
        ViewTestUtils.runOnMainAndDrawSync(getInstrumentation(), mListView, () -> {
            mListView.setAdapter(new DummyAdapter(1000));
            mListView.addHeaderView(header);
        });
        assertEquals("test sanity", 1, header.mOnAttachCount);
        assertEquals("test sanity", 0, header.mOnDetachCount);
        while(header.getParent() != null) {
            assertEquals("header view should NOT be detached", 0, header.mOnDetachCount);
            sendKeys(KeyEvent.KEYCODE_DPAD_DOWN);
            ViewTestUtils.runOnMainAndDrawSync(getInstrumentation(), mListView, null);
        }
        assertEquals("header view should be detached", 1, header.mOnDetachCount);
        assertFalse(header.isTemporarilyDetached());
    }

    @MediumTest
    public void testFullyDetachUnusedViewOnScroll() {
        final AttachDetachAwareView theView = new AttachDetachAwareView(mActivity);
        ViewTestUtils.runOnMainAndDrawSync(getInstrumentation(), mListView, () -> {
            mListView.setAdapter(new DummyAdapter(1000, theView));
        });
        assertEquals("test sanity", 1, theView.mOnAttachCount);
        assertEquals("test sanity", 0, theView.mOnDetachCount);
        ViewTestUtils.runOnMainAndDrawSync(getInstrumentation(), mListView, () -> {
            mListView.scrollListBy(mListView.getHeight() * 2);
        });
        assertNull("test sanity, unused view should be removed", theView.getParent());
        assertEquals("unused view should be detached", 1, theView.mOnDetachCount);
        assertFalse(theView.isTemporarilyDetached());
        ViewTestUtils.runOnMainAndDrawSync(getInstrumentation(), mListView, () -> {
            mListView.scrollListBy(-mListView.getHeight() * 2);
            // listview limits scroll to 1 page which is why we call it twice here.
            mListView.scrollListBy(-mListView.getHeight() * 2);
        });
        assertNotNull("test sanity, view should be re-added", theView.getParent());
        assertEquals("view should receive another attach call", 2, theView.mOnAttachCount);
        assertEquals("view should not receive a detach call", 1, theView.mOnDetachCount);
        assertFalse(theView.isTemporarilyDetached());
    }

    @MediumTest
    public void testFullyDetachUnusedViewOnReLayout() {
        final AttachDetachAwareView theView = new AttachDetachAwareView(mActivity);
        ViewTestUtils.runOnMainAndDrawSync(getInstrumentation(), mListView, () -> {
            mListView.setAdapter(new DummyAdapter(1000, theView));
        });
        assertEquals("test sanity", 1, theView.mOnAttachCount);
        assertEquals("test sanity", 0, theView.mOnDetachCount);
        ViewTestUtils.runOnMainAndDrawSync(getInstrumentation(), mListView, () -> {
            mListView.setSelection(800);
        });
        assertNull("test sanity, unused view should be removed", theView.getParent());
        assertEquals("unused view should be detached", 1, theView.mOnDetachCount);
        assertFalse(theView.isTemporarilyDetached());
        ViewTestUtils.runOnMainAndDrawSync(getInstrumentation(), mListView, () -> {
            mListView.setSelection(0);
        });
        assertNotNull("test sanity, view should be re-added", theView.getParent());
        assertEquals("view should receive another attach call", 2, theView.mOnAttachCount);
        assertEquals("view should not receive a detach call", 1, theView.mOnDetachCount);
        assertFalse(theView.isTemporarilyDetached());
    }

    @MediumTest
    public void testFullyDetachUnusedViewOnScrollForFocus() {
        final AttachDetachAwareView theView = new AttachDetachAwareView(mActivity);
        ViewTestUtils.runOnMainAndDrawSync(getInstrumentation(), mListView, () -> {
            mListView.setAdapter(new DummyAdapter(1000, theView));
        });
        assertEquals("test sanity", 1, theView.mOnAttachCount);
        assertEquals("test sanity", 0, theView.mOnDetachCount);
        while(theView.getParent() != null) {
            assertEquals("the view should NOT be detached", 0, theView.mOnDetachCount);
            sendKeys(KeyEvent.KEYCODE_DPAD_DOWN);
            ViewTestUtils.runOnMainAndDrawSync(getInstrumentation(), mListView, null);
        }
        assertEquals("the view should be detached", 1, theView.mOnDetachCount);
        assertFalse(theView.isTemporarilyDetached());
        while(theView.getParent() == null) {
            sendKeys(KeyEvent.KEYCODE_DPAD_UP);
            ViewTestUtils.runOnMainAndDrawSync(getInstrumentation(), mListView, null);
        }
        assertEquals("the view should be re-attached", 2, theView.mOnAttachCount);
        assertEquals("the view should not recieve another detach", 1, theView.mOnDetachCount);
        assertFalse(theView.isTemporarilyDetached());
    }

    @MediumTest
    public void testSetPadding() {
        View view = new View(mActivity);
        view.setLayoutParams(new ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.WRAP_CONTENT));
        view.setMinimumHeight(30);
        final DummyAdapter adapter = new DummyAdapter(2, view);
        ViewTestUtils.runOnMainAndDrawSync(getInstrumentation(), mListView, () -> {
            mListView.setLayoutParams(new LinearLayout.LayoutParams(200, 100));
            mListView.setAdapter(adapter);
        });
        assertEquals("test sanity", 200, mListView.getWidth());
        assertEquals(200, view.getWidth());
        ViewTestUtils.runOnMainAndDrawSync(getInstrumentation(), mListView, () -> {
            mListView.setPadding(10, 0, 5, 0);
            assertTrue(view.isLayoutRequested());
        });
        assertEquals(185, view.getWidth());
        assertFalse(view.isLayoutRequested());
        ViewTestUtils.runOnMainAndDrawSync(getInstrumentation(), mListView, () -> {
            mListView.setPadding(10, 0, 5, 0);
            assertFalse(view.isLayoutRequested());
        });

    }

    private class MockView extends View {

        public boolean onMeasureCalled = false;

        public MockView(Context context) {
            super(context);
        }

        @Override
        protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
            super.onMeasure(widthMeasureSpec, heightMeasureSpec);
            onMeasureCalled = true;
        }
    }

    private class Adapter<T> extends ArrayAdapter<T> {

        public Adapter(Context context, int resource, List<T> objects) {
            super(context, resource, objects);
        }

        @Override
        public View getView(int position, View convertView, ViewGroup parent) {
            return new MockView(getContext());
        }
    }

    @MediumTest
    public void testRequestLayoutWithTemporaryDetach() throws Exception {
        ListView listView = new ListView(mActivity);
        List<String> items = new ArrayList<>();
        items.add("0");
        items.add("1");
        items.add("2");
        final TemporarilyDetachableMockViewAdapter<String> adapter =
                new TemporarilyDetachableMockViewAdapter<>(
                        mActivity, android.R.layout.simple_list_item_1, items);
        mInstrumentation.runOnMainSync(() -> {
            listView.setAdapter(adapter);
            mActivity.setContentView(listView);
        });
        mInstrumentation.waitForIdleSync();

        assertEquals(items.size(), listView.getCount());
        final TemporarilyDetachableMockView childView0 =
                (TemporarilyDetachableMockView) listView.getChildAt(0);
        final TemporarilyDetachableMockView childView1 =
                (TemporarilyDetachableMockView) listView.getChildAt(1);
        final TemporarilyDetachableMockView childView2 =
                (TemporarilyDetachableMockView) listView.getChildAt(2);
        assertNotNull(childView0);
        assertNotNull(childView1);
        assertNotNull(childView2);

        // Make sure that the childView1 has focus.
        ViewTestUtils.runOnMainAndDrawSync(mInstrumentation, childView1, childView1::requestFocus);
        assertTrue(childView1.isFocused());

        // Make sure that ListView#requestLayout() is optimized when nothing is changed.
        ViewTestUtils.runOnMainAndDrawSync(mInstrumentation, listView, listView::requestLayout);
        assertEquals(childView0, listView.getChildAt(0));
        assertEquals(childView1, listView.getChildAt(1));
        assertEquals(childView2, listView.getChildAt(2));
    }

    private class TemporarilyDetachableMockView extends View {

        private boolean mIsDispatchingStartTemporaryDetach = false;
        private boolean mIsDispatchingFinishTemporaryDetach = false;

        public TemporarilyDetachableMockView(Context context) {
            super(context);
        }

        @Override
        public void dispatchStartTemporaryDetach() {
            mIsDispatchingStartTemporaryDetach = true;
            super.dispatchStartTemporaryDetach();
            mIsDispatchingStartTemporaryDetach = false;
        }

        @Override
        public void dispatchFinishTemporaryDetach() {
            mIsDispatchingFinishTemporaryDetach = true;
            super.dispatchFinishTemporaryDetach();
            mIsDispatchingFinishTemporaryDetach = false;
        }

        @Override
        public void onStartTemporaryDetach() {
            super.onStartTemporaryDetach();
            if (!mIsDispatchingStartTemporaryDetach) {
                throw new IllegalStateException("#onStartTemporaryDetach() must be indirectly"
                        + " called via #dispatchStartTemporaryDetach()");
            }
        }

        @Override
        public void onFinishTemporaryDetach() {
            super.onFinishTemporaryDetach();
            if (!mIsDispatchingFinishTemporaryDetach) {
                throw new IllegalStateException("#onStartTemporaryDetach() must be indirectly"
                        + " called via #dispatchFinishTemporaryDetach()");
            }
        }
    }

    private class TemporarilyDetachableMockViewAdapter<T> extends ArrayAdapter<T> {
        ArrayList<TemporarilyDetachableMockView> views = new ArrayList<>();

        public TemporarilyDetachableMockViewAdapter(Context context, int textViewResourceId,
                List<T> objects) {
            super(context, textViewResourceId, objects);
            for (int i = 0; i < objects.size(); i++) {
                views.add(new TemporarilyDetachableMockView(context));
                views.get(i).setFocusable(true);
            }
        }

        @Override
        public int getCount() {
            return views.size();
        }

        @Override
        public long getItemId(int position) {
            return position;
        }

        @Override
        public View getView(int position, View convertView, ViewGroup parent) {
            return views.get(position);
        }
    }

    public void testTransientStateUnstableIds() throws Exception {
        final ListView listView = mListView;
        final ArrayList<String> items = new ArrayList<String>(Arrays.asList(mCountryList));
        final ArrayAdapter<String> adapter = new ArrayAdapter<String>(mActivity,
                android.R.layout.simple_list_item_1, items);

        ViewTestUtils.runOnMainAndDrawSync(mInstrumentation, listView,
                () -> listView.setAdapter(adapter));

        final View oldItem = listView.getChildAt(2);
        final CharSequence oldText = ((TextView) oldItem.findViewById(android.R.id.text1))
                .getText();
        oldItem.setHasTransientState(true);

        ViewTestUtils.runOnMainAndDrawSync(mInstrumentation, listView,
                () -> {
                    adapter.remove(adapter.getItem(0));
                    adapter.notifyDataSetChanged();
                });

        final View newItem = listView.getChildAt(2);
        final CharSequence newText = ((TextView) newItem.findViewById(android.R.id.text1))
                .getText();

        Assert.assertFalse(oldText.equals(newText));
    }

    public void testTransientStateStableIds() throws Exception {
        final ListView listView = mListView;
        final ArrayList<String> items = new ArrayList<String>(Arrays.asList(mCountryList));
        final StableArrayAdapter<String> adapter = new StableArrayAdapter<String>(mActivity,
                android.R.layout.simple_list_item_1, items);

        ViewTestUtils.runOnMainAndDrawSync(mInstrumentation, mListView,
                () -> listView.setAdapter(adapter));

        final Object tag = new Object();
        final View oldItem = listView.getChildAt(2);
        final CharSequence oldText = ((TextView) oldItem.findViewById(android.R.id.text1))
                .getText();
        oldItem.setHasTransientState(true);
        oldItem.setTag(tag);

        ViewTestUtils.runOnMainAndDrawSync(mInstrumentation, listView,
                () -> {
                    adapter.remove(adapter.getItem(0));
                    adapter.notifyDataSetChanged();
                });

        final View newItem = listView.getChildAt(1);
        final CharSequence newText = ((TextView) newItem.findViewById(android.R.id.text1))
                .getText();

        Assert.assertTrue(newItem.hasTransientState());
        Assert.assertEquals(oldText, newText);
        Assert.assertEquals(tag, newItem.getTag());
    }

    private static class StableArrayAdapter<T> extends ArrayAdapter<T> {
        public StableArrayAdapter(Context context, int resource, List<T> objects) {
            super(context, resource, objects);
        }

        @Override
        public long getItemId(int position) {
            return getItem(position).hashCode();
        }

        @Override
        public boolean hasStableIds() {
            return true;
        }
    }
}
