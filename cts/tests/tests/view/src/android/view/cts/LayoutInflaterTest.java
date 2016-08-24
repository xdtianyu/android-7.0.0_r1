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

package android.view.cts;

import android.view.cts.R;
import android.view.cts.util.XmlUtils;

import org.xmlpull.v1.XmlPullParser;

import android.content.ComponentName;
import android.content.Context;
import android.content.pm.ActivityInfo;
import android.content.pm.PackageManager;
import android.content.res.Resources;
import android.content.res.Resources.Theme;
import android.content.res.XmlResourceParser;
import android.test.AndroidTestCase;
import android.util.AttributeSet;
import android.util.TypedValue;
import android.util.Xml;
import android.view.Gravity;
import android.view.InflateException;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.LayoutInflater.Factory;
import android.view.LayoutInflater.Filter;
import android.widget.LinearLayout;

public class LayoutInflaterTest extends AndroidTestCase {
    private LayoutInflater mLayoutInflater;

    @SuppressWarnings("hiding")
    private Context mContext;

    private final Factory mFactory = new Factory() {
        @Override
        public View onCreateView(String name, Context context, AttributeSet attrs) {
            return null;
        }
    };
    private boolean isOnLoadClass;
    private final Filter mFilter = new Filter() {
        @SuppressWarnings({ "unchecked", "rawtypes" })
        @Override
        public boolean onLoadClass(Class clazz) {
            isOnLoadClass = true;
            return true;
        }

    };

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mContext = getContext();
        mLayoutInflater = (LayoutInflater) mContext
                .getSystemService(Context.LAYOUT_INFLATER_SERVICE);
    }

    public void testFrom() {
        mLayoutInflater = null;
        mLayoutInflater = LayoutInflater.from(mContext);
        assertNotNull(mLayoutInflater);
        mLayoutInflater = null;
        mLayoutInflater = new MockLayoutInflater(mContext);
        assertNotNull(mLayoutInflater);

        LayoutInflater layoutInflater = new MockLayoutInflater(mLayoutInflater,
                mContext);
        assertNotNull(layoutInflater);
    }

    public void testAccessLayoutInflaterProperties() {
        mLayoutInflater.setFilter(mFilter);
        assertSame(mFilter, mLayoutInflater.getFilter());
        mLayoutInflater.setFactory(mFactory);
        assertSame(mFactory, mLayoutInflater.getFactory());
        mLayoutInflater=new MockLayoutInflater(mContext);
        assertSame(mContext, mLayoutInflater.getContext());
    }

    private AttributeSet getAttrs() {
        XmlResourceParser parser = null;
        AttributeSet attrs = null;
        ActivityInfo ai = null;
        ComponentName mComponentName = new ComponentName(mContext,
                MockActivity.class);
        try {
            ai = mContext.getPackageManager().getActivityInfo(mComponentName,
                    PackageManager.GET_META_DATA);
            parser = ai.loadXmlMetaData(mContext.getPackageManager(),
                    "android.widget.layout");
            int type;
            while ((type = parser.next()) != XmlPullParser.END_DOCUMENT
                    && type != XmlPullParser.START_TAG) {
            }
            String nodeName = parser.getName();
            if (!"alias".equals(nodeName)) {
                throw new InflateException();
            }
            int outerDepth = parser.getDepth();
            while ((type = parser.next()) != XmlPullParser.END_DOCUMENT
                    && (type != XmlPullParser.END_TAG || parser.getDepth() > outerDepth)) {
                if (type == XmlPullParser.END_TAG || type == XmlPullParser.TEXT) {
                    continue;
                }
                nodeName = parser.getName();
                if ("AbsoluteLayout".equals(nodeName)) {
                    attrs = Xml.asAttributeSet(parser);
                } else {
                    XmlUtils.skipCurrentTag(parser);
                }
            }
        } catch (Exception e) {
        }
        return attrs;
    }

    public void testCreateView() {

        AttributeSet attrs = getAttrs();
        isOnLoadClass = false;
        View view = null;
        try {
            view = mLayoutInflater
                    .createView("testthrow", "com.android", attrs);
            fail("should throw exception");
        } catch (InflateException e) {
        } catch (ClassNotFoundException e) {
        }
        assertFalse(isOnLoadClass);
        assertNull(view);
        mLayoutInflater = null;
        mLayoutInflater = LayoutInflater.from(mContext);
        isOnLoadClass = false;
        mLayoutInflater.setFilter(new Filter() {
            @SuppressWarnings({ "unchecked", "rawtypes" })
            @Override
            public boolean onLoadClass(Class clazz) {
                isOnLoadClass = true;
                return false;
            }
        });
        try {
            view = mLayoutInflater.createView("MockActivity",
                    "com.android.app.", attrs);
            fail("should throw exception");
        } catch (InflateException e) {
        } catch (ClassNotFoundException e) {
        }
        assertFalse(isOnLoadClass);
        assertNull(view);

        isOnLoadClass = false;
        // allowedState is false
        try {
            view = mLayoutInflater.createView(MockActivity.class.getName(),
                    MockActivity.class.getPackage().toString(), attrs);
            fail("should throw exception");
        } catch (InflateException e) {
        } catch (ClassNotFoundException e) {
        }
        assertFalse(isOnLoadClass);
        assertNull(view);
        mLayoutInflater = null;
        mLayoutInflater = LayoutInflater.from(mContext);
        try {
            mLayoutInflater.setFilter(null);
            view = mLayoutInflater.createView("com.android.app.MockActivity",
                    null, attrs);
            assertNotNull(view);
            assertFalse(isOnLoadClass);
            mLayoutInflater = null;
            mLayoutInflater = LayoutInflater.from(mContext);
            mLayoutInflater.setFilter(null);

            view = mLayoutInflater.createView(MockActivity.class.getName(),
                    MockActivity.class.getPackage().toString(), attrs);
            assertNotNull(view);
            assertFalse(isOnLoadClass);
            mLayoutInflater.setFilter(mFilter);
            view = mLayoutInflater.createView(MockActivity.class.getName(),
                    MockActivity.class.getPackage().toString(), attrs);
            assertNotNull(view);
            assertTrue(isOnLoadClass);
            // allowedState!=null
            view = mLayoutInflater.createView(MockActivity.class.getName(),
                    MockActivity.class.getPackage().toString(), attrs);
            assertNotNull(view);
            assertTrue(isOnLoadClass);
        } catch (InflateException e) {
        } catch (ClassNotFoundException e) {
        }
    }

    public void testInflate() {
        View view = mLayoutInflater.inflate(
                android.view.cts.R.layout.inflater_layout, null);
        assertNotNull(view);
        view = null;
        try {
            view = mLayoutInflater.inflate(-1, null);
            fail("should throw exception");
        } catch (Resources.NotFoundException e) {
        }
        LinearLayout mLayout;
        mLayout = new LinearLayout(mContext);
        mLayout.setOrientation(LinearLayout.VERTICAL);
        mLayout.setHorizontalGravity(Gravity.LEFT);
        mLayout.setLayoutParams(new ViewGroup.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.MATCH_PARENT));
        assertEquals(0, mLayout.getChildCount());
        view = mLayoutInflater.inflate(R.layout.inflater_layout,
                mLayout);
        assertNotNull(view);
        assertEquals(1, mLayout.getChildCount());
    }

    public void testInflate2() {
        View view = mLayoutInflater.inflate(
                R.layout.inflater_layout, null, false);
        assertNotNull(view);
        view = null;
        try {
            view = mLayoutInflater.inflate(-1, null, false);
            fail("should throw exception");
        } catch (Resources.NotFoundException e) {

        }
        LinearLayout mLayout;
        mLayout = new LinearLayout(mContext);
        mLayout.setOrientation(LinearLayout.VERTICAL);
        mLayout.setHorizontalGravity(Gravity.LEFT);
        mLayout.setLayoutParams(new ViewGroup.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.MATCH_PARENT));
        assertEquals(0, mLayout.getChildCount());
        view = mLayoutInflater.inflate(R.layout.inflater_layout,
                mLayout, false);
        assertNotNull(view);
        assertEquals(0, mLayout.getChildCount());

        view = null;
        view = mLayoutInflater.inflate(R.layout.inflater_layout,
                mLayout, true);
        assertNotNull(view);
        assertEquals(1, mLayout.getChildCount());
    }

    public void testInflate3() {
        XmlResourceParser parser = getContext().getResources().getLayout(
                R.layout.inflater_layout);
        View view = mLayoutInflater.inflate(parser, null);
        assertNotNull(view);
        view = null;
        try {
            view = mLayoutInflater.inflate(null, null);
            fail("should throw exception");
        } catch (NullPointerException e) {
        }
        LinearLayout mLayout;
        mLayout = new LinearLayout(mContext);
        mLayout.setOrientation(LinearLayout.VERTICAL);
        mLayout.setHorizontalGravity(Gravity.LEFT);
        mLayout.setLayoutParams(new ViewGroup.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.MATCH_PARENT));
        assertEquals(0, mLayout.getChildCount());

        try {
            view = mLayoutInflater.inflate(parser, mLayout);
            fail("should throw exception");
        } catch (NullPointerException e) {
        }
        parser = getContext().getResources().getLayout(
                R.layout.inflater_layout);
        view = mLayoutInflater.inflate(parser, mLayout);
        assertNotNull(view);
        assertEquals(1, mLayout.getChildCount());
        parser = getContext().getResources().getLayout(
                R.layout.inflater_layout);
        view = mLayoutInflater.inflate(parser, mLayout);
        assertNotNull(view);
        assertEquals(2, mLayout.getChildCount());

        parser = null;
        view = null;
        parser = getParser();

        view = mLayoutInflater.inflate(parser, mLayout);
        assertNotNull(view);
        assertEquals(3, mLayout.getChildCount());
    }

    public void testInflate4() {
        XmlResourceParser parser = getContext().getResources().getLayout(
                R.layout.inflater_layout);
        View view = mLayoutInflater.inflate(parser, null, false);
        assertNotNull(view);
        view = null;
        try {
            view = mLayoutInflater.inflate(null, null, false);
            fail("should throw exception");
        } catch (NullPointerException e) {
        }
        LinearLayout mLayout;
        mLayout = new LinearLayout(mContext);
        mLayout.setOrientation(LinearLayout.VERTICAL);
        mLayout.setHorizontalGravity(Gravity.LEFT);
        mLayout.setLayoutParams(new ViewGroup.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.MATCH_PARENT));
        assertEquals(0, mLayout.getChildCount());

        try {
            view = mLayoutInflater.inflate(parser, mLayout, false);
            fail("should throw exception");
        } catch (NullPointerException e) {
        }
        parser = getContext().getResources().getLayout(
                R.layout.inflater_layout);
        view = mLayoutInflater.inflate(parser, mLayout, false);
        assertNull(view.getParent());
        assertNotNull(view);
        assertEquals(0, mLayout.getChildCount());
        parser = getContext().getResources().getLayout(
                R.layout.inflater_layout);
        assertEquals(0, mLayout.getChildCount());
        view = mLayoutInflater.inflate(parser, mLayout, true);
        assertNotNull(view);
        assertNull(view.getParent());
        assertEquals(1, mLayout.getChildCount());

        parser = null;
        parser = getParser();
        try {
            view = mLayoutInflater.inflate(parser, mLayout, false);
            fail("should throw exception");
        } catch (InflateException e) {
        }

        parser = null;
        view = null;
        parser = getParser();

        view = mLayoutInflater.inflate(parser, mLayout, true);
        assertNotNull(view);
        assertEquals(2, mLayout.getChildCount());
    }

    public void testOverrideTheme() {
        View container = mLayoutInflater.inflate(R.layout.inflater_override_theme_layout, null);
        verifyThemeType(container, "view_outer", R.id.view_outer, 1);
        verifyThemeType(container, "view_inner", R.id.view_inner, 2);
        verifyThemeType(container, "view_attr", R.id.view_attr, 3);
        verifyThemeType(container, "view_include", R.id.view_include, 4);
        verifyThemeType(container, "view_include_notheme", R.id.view_include_notheme, 5);
    }

    private void verifyThemeType(View container, String tag, int id, int type) {
        TypedValue outValue = new TypedValue();
        View view = container.findViewById(id);
        assertNotNull("Found " + tag, view);
        Theme theme = view.getContext().getTheme();
        boolean resolved = theme.resolveAttribute(R.attr.themeType, outValue, true);
        assertTrue("Resolved themeType for " + tag, resolved);
        assertEquals(tag + " has themeType " + type, type, outValue.data);
    }

    public void testInflateTags() {
        final View view = mLayoutInflater.inflate(
                android.view.cts.R.layout.inflater_layout_tags, null);
        assertNotNull(view);

        checkViewTag(view, R.id.viewlayout_root, R.id.tag_viewlayout_root, R.string.tag1);
        checkViewTag(view, R.id.mock_view, R.id.tag_mock_view, R.string.tag2);
    }

    private void checkViewTag(View parent, int viewId, int tagId, int valueResId) {
        final View target = parent.findViewById(viewId);
        assertNotNull("Found target view for " + viewId, target);

        final Object tag = target.getTag(tagId);
        assertNotNull("Tag is set", tag);
        assertTrue("Tag is a character sequence", tag instanceof CharSequence);

        final Context targetContext = target.getContext();
        final CharSequence expectedValue = targetContext.getString(valueResId);
        assertEquals(tagId + " has tag " + expectedValue, expectedValue, tag);
    }

    static class MockLayoutInflater extends LayoutInflater {

        public MockLayoutInflater(Context c) {
            super(c);
        }

        public MockLayoutInflater(LayoutInflater original, Context newContext) {
            super(original, newContext);
        }

        @Override
        public View onCreateView(String name, AttributeSet attrs)
                throws ClassNotFoundException {
            return super.onCreateView(name, attrs);
        }

        @Override
        public LayoutInflater cloneInContext(Context newContext) {
            return null;
        }
    }

    private XmlResourceParser getParser() {
        XmlResourceParser parser = null;
        ActivityInfo ai = null;
        ComponentName mComponentName = new ComponentName(mContext,
                MockActivity.class);
        try {
            ai = mContext.getPackageManager().getActivityInfo(mComponentName,
                    PackageManager.GET_META_DATA);
            parser = ai.loadXmlMetaData(mContext.getPackageManager(),
                    "android.view.merge");
        } catch (Exception e) {
        }
        return parser;
    }
}
