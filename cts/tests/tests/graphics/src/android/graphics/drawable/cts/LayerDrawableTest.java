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

package android.graphics.drawable.cts;

import org.xmlpull.v1.XmlPullParserException;

import android.content.res.Resources;
import android.content.res.Resources.Theme;
import android.content.res.XmlResourceParser;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.ColorFilter;
import android.graphics.PixelFormat;
import android.graphics.Rect;
import android.graphics.cts.R;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.ColorDrawable;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.Drawable.ConstantState;
import android.graphics.drawable.GradientDrawable;
import android.graphics.drawable.LayerDrawable;
import android.graphics.drawable.RotateDrawable;
import android.graphics.drawable.ShapeDrawable;
import android.graphics.drawable.StateListDrawable;
import android.test.AndroidTestCase;
import android.util.AttributeSet;
import android.util.StateSet;
import android.util.Xml;
import android.view.Gravity;
import android.view.View;

import java.io.IOException;

public class LayerDrawableTest extends AndroidTestCase {

    @SuppressWarnings("deprecation")
    public void testConstructor() {
        Drawable bitmapDrawable = new BitmapDrawable();
        Drawable colorDrawable  = new ColorDrawable(Color.BLUE);
        Drawable[] array = new Drawable[] { bitmapDrawable, colorDrawable };
        LayerDrawable layerDrawable = new LayerDrawable(array);
        assertEquals(array.length, layerDrawable.getNumberOfLayers());
        assertSame(bitmapDrawable, layerDrawable.getDrawable(0));
        assertSame(colorDrawable, layerDrawable.getDrawable(1));

        array = new Drawable[0];
        layerDrawable = new LayerDrawable(array);
        assertEquals(0, layerDrawable.getNumberOfLayers());

        try {
            new LayerDrawable(null);
            fail("Should throw IllegalArgumentException");
        } catch (IllegalArgumentException e) {
        }
    }

    public void testInflate() throws XmlPullParserException, IOException {
        Drawable[] array = new Drawable[0];
        LayerDrawable layerDrawable = new LayerDrawable(array);

        XmlResourceParser parser = mContext.getResources().getXml(R.xml.layerdrawable);
        AttributeSet attrs = DrawableTestUtils.getAttributeSet(parser, "layer-list_full");

        layerDrawable.inflate(mContext.getResources(), parser, attrs);

        assertEquals(4, layerDrawable.getNumberOfLayers());
        assertEquals(ColorDrawable.class, layerDrawable.getDrawable(0).getClass());
        assertEquals(0x88, layerDrawable.getDrawable(0).getAlpha());
        assertEquals(View.NO_ID, layerDrawable.getId(0));
        assertEquals(BitmapDrawable.class, layerDrawable.getDrawable(1).getClass());
        assertEquals(View.NO_ID, layerDrawable.getId(1));
        assertEquals(RotateDrawable.class, layerDrawable.getDrawable(2).getClass());
        assertEquals(View.NO_ID, layerDrawable.getId(2));
        assertEquals(GradientDrawable.class, layerDrawable.getDrawable(3).getClass());
        assertEquals(R.id.background, layerDrawable.getId(3));

        layerDrawable = new LayerDrawable(array);
        parser = mContext.getResources().getXml(R.xml.layerdrawable);
        attrs = DrawableTestUtils.getAttributeSet(parser, "layer-list_empty");
        layerDrawable.inflate(mContext.getResources(), parser, attrs);
        assertEquals(0, layerDrawable.getNumberOfLayers());

        parser = mContext.getResources().getXml(R.xml.layerdrawable);
        attrs = DrawableTestUtils.getAttributeSet(parser, "layer-list_exception");
        try {
            layerDrawable.inflate(mContext.getResources(), parser, attrs);
            fail("Should throw XmlPullParserException if neither 'drawable' attribute" +
                    " nor child tag defining a drawable in <item> tag.");
        } catch (XmlPullParserException e) {
        }

        try {
            layerDrawable.inflate(null, parser, attrs);
            fail("Should throw NullPointerException if resource is null");
        } catch (NullPointerException e) {
        }

        try {
            layerDrawable.inflate(mContext.getResources(), null, attrs);
            fail("Should throw NullPointerException if parser is null");
        } catch (NullPointerException e) {
        }

        try {
            layerDrawable.inflate(mContext.getResources(), parser, null);
            fail("Should throw NullPointerException if attribute set is null");
        } catch (NullPointerException e) {
        }
    }

    @SuppressWarnings("deprecation")
    public void testFindDrawableByLayerId() {
        Drawable bitmapDrawable = new BitmapDrawable();
        Drawable colorDrawable  = new ColorDrawable(Color.BLUE);
        Drawable[] array = new Drawable[] { bitmapDrawable, colorDrawable };
        LayerDrawable layerDrawable = new LayerDrawable(array);

        layerDrawable.setId(0, 10);
        layerDrawable.setId(1, 20);
        assertSame(bitmapDrawable, layerDrawable.findDrawableByLayerId(10));
        assertSame(colorDrawable, layerDrawable.findDrawableByLayerId(20));
        assertNull(layerDrawable.findDrawableByLayerId(30));

        layerDrawable.setId(0, Integer.MIN_VALUE);
        layerDrawable.setId(1, Integer.MAX_VALUE);
        assertSame(bitmapDrawable, layerDrawable.findDrawableByLayerId(Integer.MIN_VALUE));
        assertSame(colorDrawable, layerDrawable.findDrawableByLayerId(Integer.MAX_VALUE));

        layerDrawable.setId(0, 10);
        layerDrawable.setId(1, 10);
        assertSame(colorDrawable, layerDrawable.findDrawableByLayerId(10));
        assertNull(layerDrawable.findDrawableByLayerId(30));
    }

    @SuppressWarnings("deprecation")
    public void testAccessId() {
        Drawable[] array = new Drawable[] { new BitmapDrawable(), new ColorDrawable(Color.BLUE) };
        LayerDrawable layerDrawable = new LayerDrawable(array);

        layerDrawable.setId(0, 10);
        layerDrawable.setId(1, 20);
        assertEquals(10, layerDrawable.getId(0));
        assertEquals(20, layerDrawable.getId(1));

        layerDrawable.setId(0, Integer.MIN_VALUE);
        layerDrawable.setId(1, Integer.MAX_VALUE);
        assertEquals(Integer.MIN_VALUE, layerDrawable.getId(0));
        assertEquals(Integer.MAX_VALUE, layerDrawable.getId(1));

        try {
            layerDrawable.setId(-1, 20);
            fail("Should throw IndexOutOfBoundsException");
        } catch (IndexOutOfBoundsException e) {
        }

        try {
            layerDrawable.setId(layerDrawable.getNumberOfLayers(), 20);
            fail("Should throw IndexOutOfBoundsException");
        } catch (IndexOutOfBoundsException e) {
        }

        try {
            layerDrawable.getId(-1);
            fail("Should throw IndexOutOfBoundsException");
        } catch (IndexOutOfBoundsException e) {
        }

        try {
            layerDrawable.getId(layerDrawable.getNumberOfLayers());
            fail("Should throw IndexOutOfBoundsException");
        } catch (IndexOutOfBoundsException e) {
        }
    }

    @SuppressWarnings("deprecation")
    public void testGetNumberOfLayers() {
        Drawable[] array = new Drawable[] { new BitmapDrawable(), new ColorDrawable(Color.BLUE) };
        LayerDrawable layerDrawable = new LayerDrawable(array);
        assertEquals(2, layerDrawable.getNumberOfLayers());

        array = new Drawable[5];
        for (int i = 0; i < 5; i++) {
            array[i] = new BitmapDrawable();
        }
        layerDrawable = new LayerDrawable(array);
        assertEquals(5, layerDrawable.getNumberOfLayers());

        array = new Drawable[0];
        layerDrawable = new LayerDrawable(array);
        assertEquals(0, layerDrawable.getNumberOfLayers());
    }

    @SuppressWarnings("deprecation")
    public void testAccessDrawable() {
        Drawable bitmapDrawable = new BitmapDrawable();
        Drawable colorDrawable  = new ColorDrawable(Color.BLUE);
        Drawable[] array = new Drawable[] { bitmapDrawable, colorDrawable };
        LayerDrawable layerDrawable = new LayerDrawable(array);
        assertSame(bitmapDrawable, layerDrawable.getDrawable(0));
        assertSame(colorDrawable, layerDrawable.getDrawable(1));

        layerDrawable.setId(0, 10);
        layerDrawable.setId(1, 20);
        Drawable d1 = new ColorDrawable(Color.GREEN);
        Drawable d2 = new BitmapDrawable();
        layerDrawable.setDrawableByLayerId(10, d1);
        layerDrawable.setDrawableByLayerId(20, d2);
        assertEquals(d1, layerDrawable.getDrawable(0));
        assertEquals(d2, layerDrawable.getDrawable(1));

        assertFalse(layerDrawable.setDrawableByLayerId(30, d1));

        try {
            layerDrawable.getDrawable(layerDrawable.getNumberOfLayers());
            fail("Should throw IndexOutOfBoundsException");
        } catch (IndexOutOfBoundsException e) {
        }

        try {
            layerDrawable.getDrawable(-1);
            fail("Should throw IndexOutOfBoundsException");
        } catch (IndexOutOfBoundsException e) {
        }
    }

    @SuppressWarnings("deprecation")
    public void testSetDrawableByLayerId() {
        Drawable layer1A  = new ColorDrawable(Color.RED);
        Drawable layer2A  = new ColorDrawable(Color.BLUE);
        LayerDrawable layerDrawable = new LayerDrawable(new Drawable[] { layer1A, layer2A });
        layerDrawable.setId(0, 10);
        layerDrawable.setId(1, 20);

        Drawable layer1B = new ColorDrawable(Color.GREEN);
        layer1B.setLevel(10000);
        Drawable layer2B = new ColorDrawable(Color.YELLOW);
        layer2B.setLevel(5000);
        layerDrawable.setDrawableByLayerId(10, layer1B);
        layerDrawable.setDrawableByLayerId(20, layer2B);

        assertEquals("Level is unchanged after setDrawableByLayerId()",
                10000, layerDrawable.findDrawableByLayerId(10).getLevel());
        assertEquals("Level is unchanged after setDrawableByLayerId()",
                5000, layerDrawable.findDrawableByLayerId(20).getLevel());
    }

    @SuppressWarnings("deprecation")
    public void testSetLayerInset() {
        MockDrawable firstLayer = new MockDrawable();
        firstLayer.setIntrinsicSize(10, 10);
        MockDrawable secondLayer = new MockDrawable();
        secondLayer.setIntrinsicSize(-1, -1);

        Drawable[] array = new Drawable[] { firstLayer, secondLayer };
        LayerDrawable layerDrawable = new LayerDrawable(array);

        // set inset for layer 0
        int left = 10;
        int top = 20;
        int right = 30;
        int bottom = 40;
        layerDrawable.setLayerInset(0, left, top, right, bottom);
        assertEquals(layerDrawable.getDrawable(0).getIntrinsicWidth() + left + right,
                layerDrawable.getIntrinsicWidth());
        assertEquals(layerDrawable.getDrawable(0).getIntrinsicHeight() + top + bottom,
                layerDrawable.getIntrinsicHeight());

        // The drawable at index 0 has no intrinsic width or height, so it
        // won't be counted for the overall intrinsic width or height.
        layerDrawable.setLayerInset(1, 10, 10, 10, 10);
        assertEquals(layerDrawable.getDrawable(0).getIntrinsicWidth() + left + right,
                layerDrawable.getIntrinsicWidth());
        assertEquals(layerDrawable.getDrawable(0).getIntrinsicHeight() + top + bottom,
                layerDrawable.getIntrinsicHeight());

        try {
            layerDrawable.setLayerInset(-1, left, top, right, bottom);
            fail("Should throw IndexOutOfBoundsException");
        } catch (IndexOutOfBoundsException e) {
        }
    }

    @SuppressWarnings("deprecation")
    public void testInvalidateDrawable() {
        Drawable[] array = new Drawable[0];
        LayerDrawable layerDrawable = new LayerDrawable(array);

        MockCallback cb = new MockCallback();
        layerDrawable.setCallback(cb);
        layerDrawable.invalidateDrawable(null);
        assertTrue(cb.hasCalledInvalidate());

        cb.reset();
        layerDrawable.invalidateDrawable(new BitmapDrawable());
        assertTrue(cb.hasCalledInvalidate());

        cb.reset();
        layerDrawable.setCallback(null);
        layerDrawable.invalidateDrawable(null);
        assertFalse(cb.hasCalledInvalidate());
    }

    @SuppressWarnings("deprecation")
    public void testScheduleDrawable() {
        Drawable[] array = new Drawable[0];
        LayerDrawable layerDrawable = new LayerDrawable(array);

        MockCallback cb = new MockCallback();
        layerDrawable.setCallback(cb);
        layerDrawable.scheduleDrawable(null, null, 0);
        assertTrue(cb.hasCalledSchedule());

        cb.reset();
        layerDrawable.scheduleDrawable(new BitmapDrawable(), new Runnable() {
            @Override
            public void run() {
            }
        }, 1000L);
        assertTrue(cb.hasCalledSchedule());

        cb.reset();
        layerDrawable.setCallback(null);
        layerDrawable.scheduleDrawable(null, null, 0);
        assertFalse(cb.hasCalledSchedule());
    }

    @SuppressWarnings("deprecation")
    public void testUnscheduleDrawable() {
        Drawable[] array = new Drawable[0];
        LayerDrawable layerDrawable = new LayerDrawable(array);

        MockCallback cb = new MockCallback();
        layerDrawable.setCallback(cb);
        layerDrawable.unscheduleDrawable(null, null);
        assertTrue(cb.hasCalledUnschedule());

        cb.reset();
        layerDrawable.unscheduleDrawable(new BitmapDrawable(), new Runnable() {
            @Override
            public void run() {
            }
        });
        assertTrue(cb.hasCalledUnschedule());

        cb.reset();
        layerDrawable.setCallback(null);
        layerDrawable.unscheduleDrawable(null, null);
        assertFalse(cb.hasCalledUnschedule());
    }

    private static class MockCallback implements Drawable.Callback {
        private boolean mCalledInvalidate;
        private boolean mCalledSchedule;
        private boolean mCalledUnschedule;

        @Override
        public void invalidateDrawable(Drawable who) {
            mCalledInvalidate = true;
        }

        @Override
        public void scheduleDrawable(Drawable who, Runnable what, long when) {
            mCalledSchedule = true;
        }

        @Override
        public void unscheduleDrawable(Drawable who, Runnable what) {
            mCalledUnschedule = true;
        }

        public boolean hasCalledInvalidate() {
            return mCalledInvalidate;
        }

        public boolean hasCalledSchedule() {
            return mCalledSchedule;
        }

        public boolean hasCalledUnschedule() {
            return mCalledUnschedule;
        }

        public int getResolvedLayoutDirection(Drawable who) {
            return 0;
        }

        public void reset() {
            mCalledInvalidate = false;
            mCalledSchedule = false;
            mCalledUnschedule = false;
        }
    }

    public void testDraw() {
        MockDrawable mockDrawable1 = new MockDrawable();
        MockDrawable mockDrawable2 = new MockDrawable();
        Drawable[] array = new Drawable[] { mockDrawable1, mockDrawable2 };
        LayerDrawable layerDrawable = new LayerDrawable(array);

        // this method will call each child's draw().
        layerDrawable.draw(new Canvas());
        assertTrue(mockDrawable1.hasCalledDraw());
        assertTrue(mockDrawable2.hasCalledDraw());

        mockDrawable1.reset();
        mockDrawable2.reset();
        layerDrawable.draw(null);
        assertTrue(mockDrawable1.hasCalledDraw());
        assertTrue(mockDrawable2.hasCalledDraw());
    }

    @SuppressWarnings("deprecation")
    public void testGetChangingConfigurations() {
        final int superConfig = 1;
        final int bitmapDrawableConfig = 2;
        final int colorDrawableConfig = 4;
        final int childConfig = bitmapDrawableConfig | colorDrawableConfig;

        BitmapDrawable bitmapDrawable = new BitmapDrawable();
        bitmapDrawable.setChangingConfigurations(bitmapDrawableConfig);
        ColorDrawable colorDrawable = new ColorDrawable(Color.BLUE);
        colorDrawable.setChangingConfigurations(colorDrawableConfig);
        Drawable[] array = new Drawable[] { bitmapDrawable, colorDrawable };
        LayerDrawable layerDrawable = new LayerDrawable(array);

        assertEquals(childConfig, layerDrawable.getChangingConfigurations());

        layerDrawable.setChangingConfigurations(superConfig);
        assertEquals(superConfig | childConfig, layerDrawable.getChangingConfigurations());
    }

    public void testAccessPadding() {
        Drawable[] array = new Drawable[] { new ShapeDrawable(), new ShapeDrawable() };
        LayerDrawable layerDrawable = new LayerDrawable(array);

        Rect rc = new Rect();
        layerDrawable.getPadding(rc);
        assertEquals(0, rc.left);
        assertEquals(0, rc.top);
        assertEquals(0, rc.right);
        assertEquals(0, rc.bottom);

        Rect padding0 = new Rect(10, 20, 30, 40);
        ((ShapeDrawable) layerDrawable.getDrawable(0)).setPadding(padding0);
        layerDrawable.getPadding(rc);
        assertEquals(padding0.left, rc.left);
        assertEquals(padding0.top, rc.top);
        assertEquals(padding0.right, rc.right);
        assertEquals(padding0.bottom, rc.bottom);

        Rect padding1 = new Rect(20, 30, 40, 50);
        ((ShapeDrawable) layerDrawable.getDrawable(1)).setPadding(padding1);
        layerDrawable.getPadding(rc);
        assertEquals(padding0.left + padding1.left, rc.left);
        assertEquals(padding0.top + padding1.top, rc.top);
        assertEquals(padding0.right + padding1.right, rc.right);
        assertEquals(padding0.bottom + padding1.bottom, rc.bottom);
    }

    public void testAccessPaddingMode() {
        Rect padding = new Rect();
        Drawable dr0 = new IntrinsicSizeDrawable(20, 30, new Rect(1, 2, 3, 4));
        Drawable dr1 = new IntrinsicSizeDrawable(30, 40, new Rect(9, 8, 7, 6));
        LayerDrawable layerDrawable = new LayerDrawable(new Drawable[] { dr0, dr1 });

        assertEquals("Default padding mode is NEST",
                LayerDrawable.PADDING_MODE_NEST, layerDrawable.getPaddingMode());
        assertEquals(34, layerDrawable.getIntrinsicWidth());
        assertEquals(46, layerDrawable.getIntrinsicHeight());
        assertTrue(layerDrawable.getPadding(padding));
        assertEquals(new Rect(10, 10, 10, 10), padding);

        layerDrawable.setPaddingMode(LayerDrawable.PADDING_MODE_STACK);
        assertEquals(LayerDrawable.PADDING_MODE_STACK, layerDrawable.getPaddingMode());
        assertEquals(30, layerDrawable.getIntrinsicWidth());
        assertEquals(40, layerDrawable.getIntrinsicHeight());
        assertTrue(layerDrawable.getPadding(padding));
        assertEquals(new Rect(9, 8, 7, 6), padding);
    }

    @SuppressWarnings("deprecation")
    public void testSetVisible() {
        Drawable[] array = new Drawable[] { new BitmapDrawable(), new ColorDrawable(Color.BLUE) };
        LayerDrawable layerDrawable = new LayerDrawable(array);

        assertTrue(layerDrawable.setVisible(false, true));
        assertFalse(layerDrawable.isVisible());
        assertFalse(layerDrawable.getDrawable(0).isVisible());
        assertFalse(layerDrawable.getDrawable(1).isVisible());

        assertFalse(layerDrawable.setVisible(false, false));

        assertTrue(layerDrawable.setVisible(true, false));
        assertTrue(layerDrawable.isVisible());
        assertTrue(layerDrawable.getDrawable(0).isVisible());
        assertTrue(layerDrawable.getDrawable(1).isVisible());
    }

    public void testSetDither() {
        MockDrawable mockDrawable1 = new MockDrawable();
        MockDrawable mockDrawable2 = new MockDrawable();
        Drawable[] array = new Drawable[] { mockDrawable1, mockDrawable2 };
        LayerDrawable layerDrawable = new LayerDrawable(array);

        layerDrawable.setDither(true);
        assertTrue(mockDrawable1.hasCalledSetDither());
        assertTrue(mockDrawable2.hasCalledSetDither());

        mockDrawable1.reset();
        mockDrawable2.reset();
        layerDrawable.setDither(false);
        assertTrue(mockDrawable1.hasCalledSetDither());
        assertTrue(mockDrawable2.hasCalledSetDither());
    }

    public void testSetHotspotBounds() {
        Rect bounds = new Rect(10, 15, 100, 150);
        MockDrawable mockDrawable1 = new MockDrawable();
        MockDrawable mockDrawable2 = new MockDrawable();
        Drawable[] array = new Drawable[] { mockDrawable1, mockDrawable2 };
        LayerDrawable layerDrawable = new LayerDrawable(array);

        layerDrawable.setHotspotBounds(bounds.left, bounds.top, bounds.right, bounds.bottom);
        Rect outRect = new Rect();
        layerDrawable.getHotspotBounds(outRect);
        assertTrue(bounds.equals(outRect));
    }

    public void testGetHotspotBounds() {
        Rect bounds = new Rect(10, 15, 100, 150);
        MockDrawable mockDrawable1 = new MockDrawable();
        MockDrawable mockDrawable2 = new MockDrawable();
        Drawable[] array = new Drawable[] { mockDrawable1, mockDrawable2 };
        LayerDrawable layerDrawable = new LayerDrawable(array);

        layerDrawable.setHotspotBounds(bounds.left, bounds.top, bounds.right, bounds.bottom);
        Rect outRect = new Rect();
        layerDrawable.getHotspotBounds(outRect);
        assertTrue(bounds.equals(outRect));
    }

    public void testSetAlpha() {
        MockDrawable mockDrawable1 = new MockDrawable();
        MockDrawable mockDrawable2 = new MockDrawable();
        Drawable[] array = new Drawable[] { mockDrawable1, mockDrawable2 };
        LayerDrawable layerDrawable = new LayerDrawable(array);

        layerDrawable.setAlpha(0);
        assertTrue(mockDrawable1.hasCalledSetAlpha());
        assertTrue(mockDrawable2.hasCalledSetAlpha());

        mockDrawable1.reset();
        mockDrawable2.reset();
        layerDrawable.setAlpha(Integer.MAX_VALUE);
        assertTrue(mockDrawable1.hasCalledSetAlpha());
        assertTrue(mockDrawable2.hasCalledSetAlpha());
    }

    public void testSetColorFilter() {
        MockDrawable mockDrawable1 = new MockDrawable();
        MockDrawable mockDrawable2 = new MockDrawable();
        Drawable[] array = new Drawable[] { mockDrawable1, mockDrawable2 };
        LayerDrawable layerDrawable = new LayerDrawable(array);

        layerDrawable.setColorFilter(new ColorFilter());
        assertTrue(mockDrawable1.hasCalledColorFilter());
        assertTrue(mockDrawable2.hasCalledColorFilter());

        mockDrawable1.reset();
        mockDrawable2.reset();
        layerDrawable.setColorFilter(null);
        assertTrue(mockDrawable1.hasCalledColorFilter());
        assertTrue(mockDrawable2.hasCalledColorFilter());
    }

    public void testAccessOpacity() {
        Drawable[] array = new Drawable[0];
        LayerDrawable layerDrawable = new LayerDrawable(array);
        assertEquals(PixelFormat.TRANSPARENT, layerDrawable.getOpacity());

        MockDrawable mockDrawable1 = new MockDrawable();
        MockDrawable mockDrawable2 = new MockDrawable();
        array = new Drawable[] { mockDrawable1, mockDrawable2 };
        layerDrawable = new LayerDrawable(array);
        assertEquals(PixelFormat.OPAQUE, layerDrawable.getOpacity());

        layerDrawable = new LayerDrawable(array);
        mockDrawable2.setOpacity(PixelFormat.TRANSPARENT);
        assertEquals(PixelFormat.TRANSPARENT, layerDrawable.getOpacity());

        layerDrawable = new LayerDrawable(array);
        mockDrawable2.setOpacity(PixelFormat.TRANSPARENT);
        mockDrawable1.setOpacity(PixelFormat.TRANSLUCENT);
        assertEquals(PixelFormat.TRANSLUCENT, layerDrawable.getOpacity());

        layerDrawable = new LayerDrawable(array);
        mockDrawable1.setOpacity(PixelFormat.TRANSLUCENT);
        mockDrawable2.setOpacity(PixelFormat.UNKNOWN);
        assertEquals(PixelFormat.UNKNOWN, layerDrawable.getOpacity());

        layerDrawable = new LayerDrawable(array);
        layerDrawable.setOpacity(PixelFormat.OPAQUE);
        mockDrawable1.setOpacity(PixelFormat.TRANSLUCENT);
        mockDrawable2.setOpacity(PixelFormat.UNKNOWN);
        assertEquals(PixelFormat.OPAQUE, layerDrawable.getOpacity());

    }

    @SuppressWarnings("deprecation")
    public void testIsStateful() {
        Drawable[] array = new Drawable[0];
        LayerDrawable layerDrawable = new LayerDrawable(array);
        assertFalse(layerDrawable.isStateful());

        array = new Drawable[] { new BitmapDrawable(), new MockDrawable(false) };
        layerDrawable = new LayerDrawable(array);
        assertFalse(layerDrawable.isStateful());

        array = new Drawable[] { new BitmapDrawable(), new StateListDrawable() };
        layerDrawable = new LayerDrawable(array);
        assertTrue(layerDrawable.isStateful());
    }

    public void testSetState() {
        MockDrawable mockDrawable1 = new MockDrawable(true);
        MockDrawable mockDrawable2 = new MockDrawable(true);
        Drawable[] array = new Drawable[] { mockDrawable1, mockDrawable2 };
        LayerDrawable layerDrawable = new LayerDrawable(array);

        // Call onStateChange() without actually changing the state.
        assertFalse(layerDrawable.setState(StateSet.WILD_CARD));
        assertFalse(mockDrawable1.hasCalledSetState());
        assertFalse(mockDrawable2.hasCalledSetState());
        assertFalse(mockDrawable1.hasCalledOnBoundsChange());
        assertFalse(mockDrawable2.hasCalledOnBoundsChange());

        // Call onStateChange() to change the state from WILD_CARD to null.
        // This alters the padding of both layers, which forces a bounds change
        // for the second layer due to the default "nest" padding mode.
        mockDrawable1.reset();
        mockDrawable2.reset();
        assertTrue(layerDrawable.setState(null));
        assertTrue(mockDrawable1.hasCalledSetState());
        assertTrue(mockDrawable2.hasCalledSetState());
        assertFalse(mockDrawable1.hasCalledOnBoundsChange());
        assertTrue(mockDrawable2.hasCalledOnBoundsChange());

        // Call onStateChange() to change the state from null to valid state
        // set. This alters the padding of both layers, which forces a bounds
        // change for the second layer due to the default "nest" padding mode.
        mockDrawable1.reset();
        mockDrawable2.reset();
        assertTrue(layerDrawable.setState(new int[]{
                android.R.attr.state_checked, android.R.attr.state_empty}));
        assertTrue(mockDrawable1.hasCalledSetState());
        assertTrue(mockDrawable2.hasCalledSetState());
        assertFalse(mockDrawable1.hasCalledOnBoundsChange());
        assertTrue(mockDrawable2.hasCalledOnBoundsChange());
    }

    public void testJumpToCurrentState() {
        MockDrawable mockDrawable1 = new MockDrawable();
        MockDrawable mockDrawable2 = new MockDrawable();
        Drawable[] array = new Drawable[] { mockDrawable1, mockDrawable2 };
        LayerDrawable layerDrawable = new LayerDrawable(array);

        assertFalse(mockDrawable1.hasCalledJumpToCurrentState());
        assertFalse(mockDrawable2.hasCalledJumpToCurrentState());

        layerDrawable.jumpToCurrentState();

        assertTrue(mockDrawable1.hasCalledJumpToCurrentState());
        assertTrue(mockDrawable2.hasCalledJumpToCurrentState());
    }

    public void testSetLevel() {
        MockDrawable mockDrawable1 = new MockDrawable();
        MockDrawable mockDrawable2 = new MockDrawable();
        Drawable[] array = new Drawable[] { mockDrawable1, mockDrawable2 };
        LayerDrawable layerDrawable = new LayerDrawable(array);

        // Call onLevelChange() without actually changing the level.
        assertFalse(layerDrawable.setLevel(0));
        assertFalse(mockDrawable1.hasCalledOnLevelChange());
        assertFalse(mockDrawable2.hasCalledOnLevelChange());
        assertFalse(mockDrawable1.hasCalledOnBoundsChange());
        assertFalse(mockDrawable2.hasCalledOnBoundsChange());

        // Call onLevelChange() to change the level from 0 to MAX_VALUE. This
        // alters the padding of both layers, which forces a bounds change for
        // the second layer due to the default "nest" padding mode.
        mockDrawable1.reset();
        mockDrawable2.reset();
        assertTrue(layerDrawable.setLevel(Integer.MAX_VALUE));
        assertTrue(mockDrawable1.hasCalledOnLevelChange());
        assertTrue(mockDrawable2.hasCalledOnLevelChange());
        assertFalse(mockDrawable1.hasCalledOnBoundsChange());
        assertTrue(mockDrawable2.hasCalledOnBoundsChange());

        // Call onLevelChange() to change the level from MAX_VALUE to
        // MIN_VALUE. This alters the padding of both layers, which forces a
        // bounds change for the second layer due to the default "nest" padding
        // mode.
        mockDrawable1.reset();
        mockDrawable2.reset();
        assertTrue(layerDrawable.setLevel(Integer.MIN_VALUE));
        assertTrue(mockDrawable1.hasCalledOnLevelChange());
        assertTrue(mockDrawable2.hasCalledOnLevelChange());
        assertFalse(mockDrawable1.hasCalledOnBoundsChange());
        assertTrue(mockDrawable2.hasCalledOnBoundsChange());
    }

    public void testSetBounds() {
        MockDrawable mockDrawable1 = new MockDrawable();
        MockDrawable mockDrawable2 = new MockDrawable();
        Drawable[] array = new Drawable[] { mockDrawable1, mockDrawable2 };
        LayerDrawable layerDrawable = new LayerDrawable(array);

        Rect inset1 = new Rect(1, 2, 3, 4);
        Rect inset2 = new Rect(2, 4, 6, 7);
        Rect padding1 = new Rect(11, 22, 33, 44);
        Rect padding2 = new Rect(21, 32, 43, 54);
        layerDrawable.setLayerInset(0, inset1.left, inset1.top, inset1.right, inset1.bottom);
        layerDrawable.setLayerInset(1, inset2.left, inset2.top, inset2.right, inset2.bottom);
        mockDrawable1.setPadding(padding1);
        mockDrawable2.setPadding(padding2);
        layerDrawable.getPadding(new Rect());

        // the children's bounds before call onBoundsChange
        assertEquals(0, mockDrawable1.getBounds().left);
        assertEquals(0, mockDrawable1.getBounds().top);
        assertEquals(0, mockDrawable1.getBounds().right);
        assertEquals(0, mockDrawable1.getBounds().bottom);
        assertEquals(0, mockDrawable2.getBounds().left);
        assertEquals(0, mockDrawable2.getBounds().top);
        assertEquals(0, mockDrawable2.getBounds().right);
        assertEquals(0, mockDrawable2.getBounds().bottom);

        Rect bounds = new Rect(10, 20, 30, 40);
        layerDrawable.setBounds(bounds);

        // all children's bounds will be changed after call onBoundsChange
        assertEquals(bounds.left + inset1.left, mockDrawable1.getBounds().left);
        assertEquals(bounds.top + inset1.top, mockDrawable1.getBounds().top);
        assertEquals(bounds.right - inset1.right, mockDrawable1.getBounds().right);
        assertEquals(bounds.bottom - inset1.bottom, mockDrawable1.getBounds().bottom);
        assertEquals(bounds.left + inset2.left + padding1.left, mockDrawable2.getBounds().left);
        assertEquals(bounds.top + inset2.top + padding1.top, mockDrawable2.getBounds().top);
        assertEquals(bounds.right - inset2.right - padding1.right,
                mockDrawable2.getBounds().right);
        assertEquals(bounds.bottom - inset2.bottom - padding1.bottom,
                mockDrawable2.getBounds().bottom);
    }

    public void testGetIntrinsicWidth() {
        MockDrawable largeMockDrawable = new MockDrawable();
        largeMockDrawable.setIntrinsicSize(10, 10);
        MockDrawable smallMockDrawable = new MockDrawable();
        smallMockDrawable.setIntrinsicSize(1, 1);
        Drawable[] array = new Drawable[] { largeMockDrawable, smallMockDrawable };
        LayerDrawable layerDrawable = new LayerDrawable(array);
        assertEquals(largeMockDrawable.getIntrinsicWidth(), layerDrawable.getIntrinsicWidth());

        Rect inset1 = new Rect(1, 2, 3, 4);
        Rect inset2 = new Rect(2, 4, 6, 7);
        Rect padding1 = new Rect(11, 22, 33, 44);
        Rect padding2 = new Rect(21, 32, 43, 54);
        layerDrawable.setLayerInset(0, inset1.left, inset1.top, inset1.right, inset1.bottom);
        layerDrawable.setLayerInset(1, inset2.left, inset2.top, inset2.right, inset2.bottom);
        largeMockDrawable.setPadding(padding1);
        smallMockDrawable.setPadding(padding2);
        layerDrawable.getPadding(new Rect());
        assertEquals(smallMockDrawable.getIntrinsicWidth() + inset2.left
                + inset2.right + padding1.left + padding1.right,
                layerDrawable.getIntrinsicWidth());

        inset1 = new Rect(inset2.left + padding1.left + 1, inset2.top + padding1.top + 1,
                inset2.right + padding1.right + 1, inset2.bottom + padding1.bottom + 1);
        layerDrawable.setLayerInset(0, inset1.left, inset1.top, inset1.right, inset1.bottom);
        assertEquals(largeMockDrawable.getIntrinsicWidth() + inset1.left + inset1.right,
                layerDrawable.getIntrinsicWidth());
    }

    public void testGetIntrinsicHeight() {
        MockDrawable largeMockDrawable = new MockDrawable();
        largeMockDrawable.setIntrinsicSize(10, 10);
        MockDrawable smallMockDrawable = new MockDrawable();
        smallMockDrawable.setIntrinsicSize(1, 1);
        Drawable[] array = new Drawable[] { largeMockDrawable, smallMockDrawable };
        LayerDrawable layerDrawable = new LayerDrawable(array);
        assertEquals(largeMockDrawable.getIntrinsicHeight(), layerDrawable.getIntrinsicHeight());

        Rect inset1 = new Rect(1, 2, 3, 4);
        Rect inset2 = new Rect(2, 4, 6, 7);
        Rect padding1 = new Rect(11, 22, 33, 44);
        Rect padding2 = new Rect(21, 32, 43, 54);
        layerDrawable.setLayerInset(0, inset1.left, inset1.top, inset1.right, inset1.bottom);
        layerDrawable.setLayerInset(1, inset2.left, inset2.top, inset2.right, inset2.bottom);
        largeMockDrawable.setPadding(padding1);
        smallMockDrawable.setPadding(padding2);
        layerDrawable.getPadding(new Rect());
        assertEquals(smallMockDrawable.getIntrinsicHeight() + inset2.top
                + inset2.bottom + padding1.top + padding1.bottom,
                layerDrawable.getIntrinsicHeight());

        inset1 = new Rect(inset2.left + padding1.left + 1, inset2.top + padding1.top + 1,
                inset2.right + padding1.right + 1, inset2.bottom + padding1.bottom + 1);
        layerDrawable.setLayerInset(0, inset1.left, inset1.top, inset1.right, inset1.bottom);
        assertEquals(largeMockDrawable.getIntrinsicHeight() + inset1.top + inset1.bottom,
                layerDrawable.getIntrinsicHeight());
    }

    @SuppressWarnings("deprecation")
    public void testGetConstantState() {
        Drawable[] array = new Drawable[] { new BitmapDrawable(), new ColorDrawable(Color.BLUE) };
        LayerDrawable layerDrawable = new LayerDrawable(array);
        ConstantState constantState = layerDrawable.getConstantState();
        assertNotNull(constantState);
        assertEquals(0, constantState.getChangingConfigurations());

        layerDrawable.setChangingConfigurations(1);
        constantState = layerDrawable.getConstantState();
        assertNotNull(constantState);
        assertEquals(1, constantState.getChangingConfigurations());
    }

    @SuppressWarnings("deprecation")
    public void testAddLayer() {
        Drawable[] array = new Drawable[] { new BitmapDrawable(), new ColorDrawable(Color.BLUE) };
        LayerDrawable layerDrawable = new LayerDrawable(array);
        BitmapDrawable newDrawable = new BitmapDrawable();
        int index = layerDrawable.addLayer(newDrawable);

        final int numLayers = layerDrawable.getNumberOfLayers();
        assertEquals(index, numLayers - 1);
        assertEquals(newDrawable, layerDrawable.getDrawable(index));
    }

    @SuppressWarnings("deprecation")
    public void testGetDrawable() {
        Drawable[] array = new Drawable[] { new BitmapDrawable(), new ColorDrawable(Color.BLUE) };
        LayerDrawable layerDrawable = new LayerDrawable(array);

        final int numLayers = layerDrawable.getNumberOfLayers();
        assertEquals(array[0], layerDrawable.getDrawable(0));
        assertEquals(array[1], layerDrawable.getDrawable(1));
        try {
            assertEquals(null, layerDrawable.getDrawable(2));
            fail("Should throw IndexOutOfBoundsException");
        } catch (IndexOutOfBoundsException e) {
        }
    }

    @SuppressWarnings("deprecation")
    public void testFindIndexByLayerId() {
        Drawable[] array = new Drawable[] { new BitmapDrawable(), new ColorDrawable(Color.BLUE) };
        LayerDrawable layerDrawable = new LayerDrawable(array);

        layerDrawable.setId(0, 10);
        layerDrawable.setId(1, 20);

        assertEquals(0, layerDrawable.findIndexByLayerId(10));
        assertEquals(1, layerDrawable.findIndexByLayerId(20));
        assertEquals(-1, layerDrawable.findIndexByLayerId(30));
    }

    @SuppressWarnings("deprecation")
    public void testSetDrawable() {
        Drawable[] array = new Drawable[]{new BitmapDrawable(), new ColorDrawable(Color.BLUE)};
        LayerDrawable layerDrawable = new LayerDrawable(array);
        BitmapDrawable newBitmapDrawable = new BitmapDrawable();
        ColorDrawable newColorDrawable = new ColorDrawable(Color.GREEN);
        layerDrawable.setDrawable(0, newColorDrawable);
        layerDrawable.setDrawable(1, newBitmapDrawable);

        final int numLayers = layerDrawable.getNumberOfLayers();
        assertEquals(2, numLayers);
        assertEquals(newColorDrawable, layerDrawable.getDrawable(0));
        assertEquals(newBitmapDrawable, layerDrawable.getDrawable(1));
        try {
            assertEquals(null, layerDrawable.getDrawable(2));
            fail("Should throw IndexOutOfBoundsException");
        } catch (IndexOutOfBoundsException e) {
        }
    }

    @SuppressWarnings("deprecation")
    public void testGetLeftPadding() {
        Drawable[] array = new Drawable[]{new BitmapDrawable()};
        LayerDrawable layerDrawable = new LayerDrawable(array);
        layerDrawable.setPadding(10, 11, 20, 21);

        assertEquals(10, layerDrawable.getLeftPadding());
    }

    @SuppressWarnings("deprecation")
    public void testGetTopPadding() {
        Drawable[] array = new Drawable[]{new BitmapDrawable()};
        LayerDrawable layerDrawable = new LayerDrawable(array);
        layerDrawable.setPadding(10, 11, 20, 21);

        assertEquals(11, layerDrawable.getTopPadding());
    }

    @SuppressWarnings("deprecation")
    public void testGetRightPadding() {
        Drawable[] array = new Drawable[]{new BitmapDrawable()};
        LayerDrawable layerDrawable = new LayerDrawable(array);
        layerDrawable.setPadding(10, 11, 20, 21);

        assertEquals(20, layerDrawable.getRightPadding());
    }

    @SuppressWarnings("deprecation")
    public void testGetBottomPadding() {
        Drawable[] array = new Drawable[]{new BitmapDrawable()};
        LayerDrawable layerDrawable = new LayerDrawable(array);
        layerDrawable.setPadding(10, 11, 20, 21);

        assertEquals(21, layerDrawable.getBottomPadding());
    }

    @SuppressWarnings("deprecation")
    public void testGetStartPadding() {
        Drawable[] array = new Drawable[]{new BitmapDrawable()};
        LayerDrawable layerDrawable = new LayerDrawable(array);
        layerDrawable.setPadding(10, 11, 20, 21);

        assertEquals(-1, layerDrawable.getStartPadding());
        layerDrawable.setPaddingRelative(10, 11, 20, 21);
        assertEquals(10, layerDrawable.getStartPadding());
    }

    @SuppressWarnings("deprecation")
    public void testGetEndPadding() {
        Drawable[] array = new Drawable[]{new BitmapDrawable()};
        LayerDrawable layerDrawable = new LayerDrawable(array);
        layerDrawable.setPadding(10, 11, 20, 21);

        assertEquals(-1, layerDrawable.getEndPadding());
        layerDrawable.setPaddingRelative(10, 11, 20, 21);
        assertEquals(20, layerDrawable.getEndPadding());
    }

    @SuppressWarnings("deprecation")
    public void testSetPadding() {
        Drawable[] array = new Drawable[]{new BitmapDrawable()};
        LayerDrawable layerDrawable = new LayerDrawable(array);
        layerDrawable.setPadding(10, 11, 20, 21);

        assertEquals(10, layerDrawable.getLeftPadding());
        assertEquals(11, layerDrawable.getTopPadding());
        assertEquals(20, layerDrawable.getRightPadding());
        assertEquals(21, layerDrawable.getBottomPadding());
        assertEquals(-1, layerDrawable.getStartPadding());
        assertEquals(-1, layerDrawable.getEndPadding());
    }

    @SuppressWarnings("deprecation")
    public void testSetPaddingRelative() {
        Drawable[] array = new Drawable[]{new BitmapDrawable()};
        LayerDrawable layerDrawable = new LayerDrawable(array);
        layerDrawable.setPaddingRelative(10, 11, 20, 21);

        assertEquals(10, layerDrawable.getStartPadding());
        assertEquals(11, layerDrawable.getTopPadding());
        assertEquals(20, layerDrawable.getEndPadding());
        assertEquals(21, layerDrawable.getBottomPadding());
        assertEquals(-1, layerDrawable.getLeftPadding());
        assertEquals(-1, layerDrawable.getRightPadding());
    }

    @SuppressWarnings("deprecation")
    public void testSetLayerGravity() {
        Drawable[] array = new Drawable[]{new BitmapDrawable(), new ColorDrawable(Color.BLUE)};
        LayerDrawable layerDrawable = new LayerDrawable(array);

        layerDrawable.setLayerGravity(0, Gravity.CENTER);
        layerDrawable.setLayerGravity(1, Gravity.NO_GRAVITY);

        try {
            layerDrawable.setLayerGravity(2, Gravity.TOP);
            fail("Should throw ArrayIndexOutOfBoundsException");
        } catch (ArrayIndexOutOfBoundsException e) {
        }
        assertEquals(Gravity.CENTER, layerDrawable.getLayerGravity(0));
        assertEquals(Gravity.NO_GRAVITY, layerDrawable.getLayerGravity(1));
    }

    @SuppressWarnings("deprecation")
    public void testGetLayerGravity() {
        Drawable[] array = new Drawable[]{new BitmapDrawable(), new ColorDrawable(Color.BLUE)};
        LayerDrawable layerDrawable = new LayerDrawable(array);

        layerDrawable.setLayerGravity(0, Gravity.CENTER);
        layerDrawable.setLayerGravity(1, Gravity.NO_GRAVITY);

        assertEquals(Gravity.CENTER, layerDrawable.getLayerGravity(0));
        assertEquals(Gravity.NO_GRAVITY, layerDrawable.getLayerGravity(1));
        try {
            layerDrawable.getLayerGravity(2);
            fail("Should throw ArrayIndexOutOfBoundsException");
        } catch (ArrayIndexOutOfBoundsException e) {
        }
    }

    @SuppressWarnings("deprecation")
    public void testSetLayerWidth() {
        Drawable[] array = new Drawable[]{new BitmapDrawable(), new ColorDrawable(Color.BLUE)};
        LayerDrawable layerDrawable = new LayerDrawable(array);

        layerDrawable.setLayerWidth(0, 100);
        layerDrawable.setLayerWidth(1, 200);

        try {
            layerDrawable.setLayerWidth(2, 300);
            fail("Should throw ArrayIndexOutOfBoundsException");
        } catch (ArrayIndexOutOfBoundsException e) {
        }
        assertEquals(100, layerDrawable.getLayerWidth(0));
        assertEquals(200, layerDrawable.getLayerWidth(1));
    }

    @SuppressWarnings("deprecation")
    public void testGetLayerWidth() {
        Drawable[] array = new Drawable[]{new BitmapDrawable(), new ColorDrawable(Color.BLUE)};
        LayerDrawable layerDrawable = new LayerDrawable(array);

        layerDrawable.setLayerWidth(0, 100);
        layerDrawable.setLayerWidth(1, 200);

        assertEquals(100, layerDrawable.getLayerWidth(0));
        assertEquals(200, layerDrawable.getLayerWidth(1));
        try {
            layerDrawable.getLayerWidth(2);
            fail("Should throw ArrayIndexOutOfBoundsException");
        } catch (ArrayIndexOutOfBoundsException e) {
        }
    }

    @SuppressWarnings("deprecation")
    public void testSetLayerHeight() {
        Drawable[] array = new Drawable[]{new BitmapDrawable(), new ColorDrawable(Color.BLUE)};
        LayerDrawable layerDrawable = new LayerDrawable(array);

        layerDrawable.setLayerHeight(0, 100);
        layerDrawable.setLayerHeight(1, 200);

        try {
            layerDrawable.setLayerHeight(2, 300);
            fail("Should throw ArrayIndexOutOfBoundsException");
        } catch (ArrayIndexOutOfBoundsException e) {
        }
        assertEquals(100, layerDrawable.getLayerHeight(0));
        assertEquals(200, layerDrawable.getLayerHeight(1));
    }

    @SuppressWarnings("deprecation")
    public void testGetLayerHeight() {
        Drawable[] array = new Drawable[]{new BitmapDrawable(), new ColorDrawable(Color.BLUE)};
        LayerDrawable layerDrawable = new LayerDrawable(array);

        layerDrawable.setLayerHeight(0, 100);
        layerDrawable.setLayerHeight(1, 200);

        assertEquals(100, layerDrawable.getLayerHeight(0));
        assertEquals(200, layerDrawable.getLayerHeight(1));
        try {
            layerDrawable.getLayerHeight(2);
            fail("Should throw ArrayIndexOutOfBoundsException");
        } catch (ArrayIndexOutOfBoundsException e) {
        }
    }

    @SuppressWarnings("deprecation")
    public void testSetLayerSize() {
        Drawable[] array = new Drawable[]{new BitmapDrawable(), new ColorDrawable(Color.BLUE)};
        LayerDrawable layerDrawable = new LayerDrawable(array);

        layerDrawable.setLayerSize(0, 100, 200);
        layerDrawable.setLayerSize(1, 300, 400);

        try {
            layerDrawable.setLayerSize(2, 500, 600);
            fail("Should throw ArrayIndexOutOfBoundsException");
        } catch (ArrayIndexOutOfBoundsException e) {
        }
        assertEquals(100, layerDrawable.getLayerWidth(0));
        assertEquals(200, layerDrawable.getLayerHeight(0));
        assertEquals(300, layerDrawable.getLayerWidth(1));
        assertEquals(400, layerDrawable.getLayerHeight(1));
    }

    @SuppressWarnings("deprecation")
    public void testSetLayerInsetRelative() {
        MockDrawable firstLayer = new MockDrawable();
        firstLayer.setIntrinsicSize(10, 10);
        MockDrawable secondLayer = new MockDrawable();
        secondLayer.setIntrinsicSize(-1, -1);

        Drawable[] array = new Drawable[] { firstLayer, secondLayer };
        LayerDrawable layerDrawable = new LayerDrawable(array);

        int start = 10;
        int top = 20;
        int end = 30;
        int bottom = 40;
        layerDrawable.setLayerInsetRelative(0, start, top, end, bottom);
        assertEquals(layerDrawable.getDrawable(0).getIntrinsicWidth() + start + end,
                layerDrawable.getIntrinsicWidth());
        assertEquals(layerDrawable.getDrawable(0).getIntrinsicHeight() + top + bottom,
                layerDrawable.getIntrinsicHeight());
        assertEquals(start, layerDrawable.getLayerInsetStart(0));
        assertEquals(top, layerDrawable.getLayerInsetTop(0));
        assertEquals(end, layerDrawable.getLayerInsetEnd(0));
        assertEquals(bottom, layerDrawable.getLayerInsetBottom(0));
        assertEquals(0, layerDrawable.getLayerInsetLeft(0));
        assertEquals(0, layerDrawable.getLayerInsetRight(0));

        // The drawable at index 1 has no intrinsic width or height, so it
        // won't be counted for the overall intrinsic width or height.
        layerDrawable.setLayerInsetRelative(1, 10, 10, 10, 10);
        assertEquals(layerDrawable.getDrawable(0).getIntrinsicWidth() + start + end,
                layerDrawable.getIntrinsicWidth());
        assertEquals(layerDrawable.getDrawable(0).getIntrinsicHeight() + top + bottom,
                layerDrawable.getIntrinsicHeight());

        try {
            layerDrawable.setLayerInsetRelative(-1, start, top, end, bottom);
            fail("Should throw IndexOutOfBoundsException");
        } catch (IndexOutOfBoundsException e) {
        }
    }

    @SuppressWarnings("deprecation")
    public void testSetLayerInsetLeft() {
        Drawable[] array = new Drawable[] { new BitmapDrawable() };
        LayerDrawable layerDrawable = new LayerDrawable(array);

        // set inset for layer 0
        int left = 10;
        int top = 20;
        int right = 30;
        int bottom = 40;
        layerDrawable.setLayerInset(0, left, top, right, bottom);
        assertEquals(layerDrawable.getDrawable(0).getIntrinsicWidth() + left + right,
                layerDrawable.getIntrinsicWidth());
        left += 5;
        layerDrawable.setLayerInsetLeft(0, left);
        assertEquals(layerDrawable.getDrawable(0).getIntrinsicWidth() + left + right,
                layerDrawable.getIntrinsicWidth());
        assertEquals(left, layerDrawable.getLayerInsetLeft(0));

        try {
            layerDrawable.setLayerInsetLeft(1, left);
            fail("Should throw IndexOutOfBoundsException");
        } catch (IndexOutOfBoundsException e) {
        }
    }

    @SuppressWarnings("deprecation")
    public void testGetLayerInsetLeft() {
        Drawable[] array = new Drawable[] { new BitmapDrawable() };
        LayerDrawable layerDrawable = new LayerDrawable(array);

        // set inset for layer 0
        int left = 10;
        int top = 20;
        int right = 30;
        int bottom = 40;
        layerDrawable.setLayerInset(0, left, top, right, bottom);
        assertEquals(left, layerDrawable.getLayerInsetLeft(0));
        left += 5;
        layerDrawable.setLayerInsetLeft(0, left);
        assertEquals(left, layerDrawable.getLayerInsetLeft(0));

        try {
            layerDrawable.getLayerInsetLeft(1);
            fail("Should throw IndexOutOfBoundsException");
        } catch (IndexOutOfBoundsException e) {
        }
    }

    @SuppressWarnings("deprecation")
    public void testSetLayerInsetTop() {
        Drawable[] array = new Drawable[] { new BitmapDrawable() };
        LayerDrawable layerDrawable = new LayerDrawable(array);

        // set inset for layer 0
        int left = 10;
        int top = 20;
        int right = 30;
        int bottom = 40;
        layerDrawable.setLayerInset(0, left, top, right, bottom);
        assertEquals(layerDrawable.getDrawable(0).getIntrinsicHeight() + top + bottom,
                layerDrawable.getIntrinsicHeight());
        top += 5;
        layerDrawable.setLayerInsetTop(0, top);
        assertEquals(layerDrawable.getDrawable(0).getIntrinsicHeight() + top + bottom,
                layerDrawable.getIntrinsicHeight());
        assertEquals(top, layerDrawable.getLayerInsetTop(0));

        try {
            layerDrawable.setLayerInsetTop(1, top);
            fail("Should throw IndexOutOfBoundsException");
        } catch (IndexOutOfBoundsException e) {
        }
    }

    @SuppressWarnings("deprecation")
    public void testGetLayerInsetTop() {
        Drawable[] array = new Drawable[] { new BitmapDrawable() };
        LayerDrawable layerDrawable = new LayerDrawable(array);

        // set inset for layer 0
        int left = 10;
        int top = 20;
        int right = 30;
        int bottom = 40;
        layerDrawable.setLayerInset(0, left, top, right, bottom);
        assertEquals(top, layerDrawable.getLayerInsetTop(0));
        top += 5;
        layerDrawable.setLayerInsetTop(0, top);
        assertEquals(top, layerDrawable.getLayerInsetTop(0));

        try {
            layerDrawable.getLayerInsetTop(1);
            fail("Should throw IndexOutOfBoundsException");
        } catch (IndexOutOfBoundsException e) {
        }
    }

    @SuppressWarnings("deprecation")
    public void testSetLayerInsetRight() {
        Drawable[] array = new Drawable[] { new BitmapDrawable() };
        LayerDrawable layerDrawable = new LayerDrawable(array);

        // set inset for layer 0
        int left = 10;
        int top = 20;
        int right = 30;
        int bottom = 40;
        layerDrawable.setLayerInset(0, left, top, right, bottom);
        assertEquals(layerDrawable.getDrawable(0).getIntrinsicWidth() + left + right,
                layerDrawable.getIntrinsicWidth());
        right += 5;
        layerDrawable.setLayerInsetRight(0, right);
        assertEquals(layerDrawable.getDrawable(0).getIntrinsicWidth() + left + right,
                layerDrawable.getIntrinsicWidth());
        assertEquals(right, layerDrawable.getLayerInsetRight(0));

        try {
            layerDrawable.setLayerInsetRight(1, right);
            fail("Should throw IndexOutOfBoundsException");
        } catch (IndexOutOfBoundsException e) {
        }
    }

    @SuppressWarnings("deprecation")
    public void testGetLayerInsetRight() {
        Drawable[] array = new Drawable[] { new BitmapDrawable() };
        LayerDrawable layerDrawable = new LayerDrawable(array);

        // set inset for layer 0
        int left = 10;
        int top = 20;
        int right = 30;
        int bottom = 40;
        layerDrawable.setLayerInset(0, left, top, right, bottom);
        assertEquals(right, layerDrawable.getLayerInsetRight(0));
        right += 5;
        layerDrawable.setLayerInsetRight(0, right);
        assertEquals(right, layerDrawable.getLayerInsetRight(0));

        try {
            layerDrawable.getLayerInsetRight(1);
            fail("Should throw IndexOutOfBoundsException");
        } catch (IndexOutOfBoundsException e) {
        }
    }

    @SuppressWarnings("deprecation")
    public void testSetLayerInsetBottom() {
        Drawable[] array = new Drawable[] { new BitmapDrawable() };
        LayerDrawable layerDrawable = new LayerDrawable(array);

        // set inset for layer 0
        int left = 10;
        int top = 20;
        int right = 30;
        int bottom = 40;
        layerDrawable.setLayerInset(0, left, top, right, bottom);
        assertEquals(layerDrawable.getDrawable(0).getIntrinsicHeight() + top + bottom,
                layerDrawable.getIntrinsicHeight());
        bottom += 5;
        layerDrawable.setLayerInsetBottom(0, bottom);
        assertEquals(layerDrawable.getDrawable(0).getIntrinsicHeight() + top + bottom,
                layerDrawable.getIntrinsicHeight());
        assertEquals(bottom, layerDrawable.getLayerInsetBottom(0));

        try {
            layerDrawable.setLayerInsetBottom(1, bottom);
            fail("Should throw IndexOutOfBoundsException");
        } catch (IndexOutOfBoundsException e) {
        }
    }

    @SuppressWarnings("deprecation")
    public void testGetLayerInsetBottom() {
        Drawable[] array = new Drawable[] { new BitmapDrawable() };
        LayerDrawable layerDrawable = new LayerDrawable(array);

        // set inset for layer 0
        int left = 10;
        int top = 20;
        int right = 30;
        int bottom = 40;
        layerDrawable.setLayerInset(0, left, top, right, bottom);
        assertEquals(bottom, layerDrawable.getLayerInsetBottom(0));
        bottom += 5;
        layerDrawable.setLayerInsetBottom(0, bottom);
        assertEquals(bottom, layerDrawable.getLayerInsetBottom(0));

        try {
            layerDrawable.getLayerInsetBottom(1);
            fail("Should throw IndexOutOfBoundsException");
        } catch (IndexOutOfBoundsException e) {
        }
    }

    @SuppressWarnings("deprecation")
    public void testSetLayerInsetStart() {
        Drawable[] array = new Drawable[] { new BitmapDrawable() };
        LayerDrawable layerDrawable = new LayerDrawable(array);

        // set inset for layer 0
        int start = 10;
        int top = 20;
        int end = 30;
        int bottom = 40;
        layerDrawable.setLayerInsetRelative(0, start, top, end, bottom);
        assertEquals(layerDrawable.getDrawable(0).getIntrinsicWidth() + start + end,
                layerDrawable.getIntrinsicWidth());
        start += 5;
        layerDrawable.setLayerInsetStart(0, start);
        assertEquals(layerDrawable.getDrawable(0).getIntrinsicWidth() + start + end,
                layerDrawable.getIntrinsicWidth());
        assertEquals(start, layerDrawable.getLayerInsetStart(0));

        try {
            layerDrawable.setLayerInset(1, start, top, end, bottom);
            fail("Should throw IndexOutOfBoundsException");
        } catch (IndexOutOfBoundsException e) {
        }
    }

    @SuppressWarnings("deprecation")
    public void testGetLayerInsetStart() {
        Drawable[] array = new Drawable[] { new BitmapDrawable() };
        LayerDrawable layerDrawable = new LayerDrawable(array);

        // set inset for layer 0
        int start = 10;
        int top = 20;
        int end = 30;
        int bottom = 40;
        layerDrawable.setLayerInsetRelative(0, start, top, end, bottom);
        assertEquals(start, layerDrawable.getLayerInsetStart(0));
        start += 5;
        layerDrawable.setLayerInsetStart(0, start);
        assertEquals(start, layerDrawable.getLayerInsetStart(0));

        try {
            layerDrawable.getLayerInsetStart(1);
            fail("Should throw IndexOutOfBoundsException");
        } catch (IndexOutOfBoundsException e) {
        }
    }

    @SuppressWarnings("deprecation")
    public void testSetLayerInsetEnd() {
        Drawable[] array = new Drawable[] { new BitmapDrawable() };
        LayerDrawable layerDrawable = new LayerDrawable(array);

        // set inset for layer 0
        int start = 10;
        int top = 20;
        int end = 30;
        int bottom = 40;
        layerDrawable.setLayerInsetRelative(0, start, top, end, bottom);
        assertEquals(end, layerDrawable.getLayerInsetEnd(0));
        assertEquals(layerDrawable.getDrawable(0).getIntrinsicWidth() + start + end,
                layerDrawable.getIntrinsicWidth());
        end += 5;
        layerDrawable.setLayerInsetEnd(0, end);
        assertEquals(layerDrawable.getDrawable(0).getIntrinsicWidth() + start + end,
                layerDrawable.getIntrinsicWidth());
        assertEquals(end, layerDrawable.getLayerInsetEnd(0));

        try {
            layerDrawable.setLayerInsetEnd(1, end);
            fail("Should throw IndexOutOfBoundsException");
        } catch (IndexOutOfBoundsException e) {
        }
    }

    @SuppressWarnings("deprecation")
    public void testGetLayerInsetEnd() {
        Drawable[] array = new Drawable[] { new BitmapDrawable() };
        LayerDrawable layerDrawable = new LayerDrawable(array);

        // set inset for layer 0
        int start = 10;
        int top = 20;
        int end = 30;
        int bottom = 40;
        layerDrawable.setLayerInsetRelative(0, start, top, end, bottom);
        assertEquals(end, layerDrawable.getLayerInsetEnd(0));
        end += 5;
        layerDrawable.setLayerInsetEnd(0, end);
        assertEquals(end, layerDrawable.getLayerInsetEnd(0));

        try {
            layerDrawable.getLayerInsetEnd(1);
            fail("Should throw IndexOutOfBoundsException");
        } catch (IndexOutOfBoundsException e) {
        }
    }

    public void testChildIntrinsicSize() {
        LayerDrawable dr;

        // Ensure that a child with no intrinsic size correctly reports bounds.
        dr = (LayerDrawable) getContext().getDrawable(R.drawable.layer_drawable_intrinsic);
        assertEquals(-1, dr.getIntrinsicWidth());
        assertEquals(-1, dr.getIntrinsicHeight());

        // Check when creating the drawble from code.
        dr = new LayerDrawable(new Drawable[] { new ColorDrawable(Color.RED) });
        dr.setLayerInset(0, 10, 10, 10, 10);
        assertEquals(-1, dr.getIntrinsicWidth());
        assertEquals(-1, dr.getIntrinsicHeight());

        // Ensure mixed children report bounds correctly as well.
        dr = (LayerDrawable) getContext().getDrawable(R.drawable.layer_drawable_intrinsic_mixed);
        int width = dr.getLayerInsetLeft(0) + dr.getLayerInsetRight(0)
                + dr.getDrawable(0).getIntrinsicWidth();
        int height = dr.getLayerInsetTop(0) + dr.getLayerInsetBottom(0)
                + dr.getDrawable(0).getIntrinsicHeight();
        assertEquals(width, dr.getIntrinsicWidth());
        assertEquals(height, dr.getIntrinsicHeight());
    }

    private static class MockDrawable extends Drawable {
        private boolean mCalledSetDither = false;
        private boolean mCalledSetAlpha = false;
        private boolean mCalledColorFilter = false;

        private boolean mCalledSetState = false;
        private boolean mCalledOnLevelChange = false;
        private boolean mCalledOnBoundsChange = false;
        private boolean mCalledJumpToCurrentState = false;

        private boolean mCalledDraw = false;

        private boolean mIsStateful = false;

        private int mOpacity = PixelFormat.OPAQUE;

        private int mIntrinsicWidth = -1;
        private int mIntrinsicHeight = -1;

        private boolean mDither = false;

        Rect mPadding = null;

        public MockDrawable() {
            this(false);
        }

        public MockDrawable(boolean isStateful) {
            mIsStateful = isStateful;
        }

        @Override
        public void draw(Canvas canvas) {
            mCalledDraw = true;
        }

        public boolean hasCalledDraw() {
            return mCalledDraw;
        }

        @Override
        public int getOpacity() {
            return mOpacity;
        }

        public void setOpacity(int opacity) {
            mOpacity = opacity;
        }

        @Override
        public void setAlpha(int alpha) {
            mCalledSetAlpha = true;
        }

        @Override
        public void setColorFilter(ColorFilter cf) {
            mCalledColorFilter = true;
        }

        @Override
        public void setDither(boolean dither) {
            mDither = dither;
            mCalledSetDither = true;
        }

        public void setIntrinsicSize(int width, int height) {
            mIntrinsicWidth = width;
            mIntrinsicHeight = height;
        }

        @Override
        public int getIntrinsicWidth() {
            return mIntrinsicWidth;
        }

        @Override
        public int getIntrinsicHeight() {
            return mIntrinsicHeight;
        }

        public boolean hasCalledSetDither() {
            return mCalledSetDither;
        }

        public boolean hasCalledSetAlpha() {
            return mCalledSetAlpha;
        }

        public boolean hasCalledColorFilter() {
            return mCalledColorFilter;
        }

        public void reset() {
            mCalledSetDither = false;
            mCalledSetAlpha = false;
            mCalledColorFilter = false;

            mCalledSetState = false;
            mCalledOnLevelChange = false;
            mCalledOnBoundsChange = false;
            mCalledJumpToCurrentState = false;

            mCalledDraw = false;
        }

        @Override
        public void jumpToCurrentState() {
            super.jumpToCurrentState();

            mCalledJumpToCurrentState = true;
        }

        public boolean hasCalledJumpToCurrentState() {
            return mCalledJumpToCurrentState;
        }

        @Override
        protected boolean onStateChange(int[] state) {
            increasePadding();
            return mIsStateful;
        }

        private void increasePadding() {
            Rect padding = new Rect();
            getPadding(padding);
            padding.left++;
            padding.top++;
            padding.right++;
            padding.bottom++;

            setPadding(padding);
        }

        @Override
        protected boolean onLevelChange(int level) {
            increasePadding();
            mCalledOnLevelChange = true;
            return true;
        }

        @Override
        protected void onBoundsChange(Rect bounds) {
            mCalledOnBoundsChange = true;
            super.onBoundsChange(bounds);
        }

        public boolean hasCalledOnBoundsChange() {
            return mCalledOnBoundsChange;
        }

        @Override
        public boolean isStateful() {
            return mIsStateful;
        }

        public boolean hasCalledSetState() {
            return mCalledSetState;
        }

        @Override
        public boolean setState(final int[] stateSet) {
            mCalledSetState = true;
            return super.setState(stateSet);
        }

        public boolean hasCalledOnLevelChange() {
            return mCalledOnLevelChange;
        }

        public void setPadding(Rect padding) {
            if (padding == null) {
                mPadding = null;
            } else {
                if (mPadding == null) {
                    mPadding = new Rect();
                }
                mPadding.set(padding);
            }
        }

        @Override
        public boolean getPadding(Rect padding) {
            if (mPadding != null) {
                padding.set(mPadding);
                return true;
            } else {
                return super.getPadding(padding);
            }
        }
    }

    public void testMutate() {
        LayerDrawable d1 = (LayerDrawable) mContext.getDrawable(R.drawable.layerdrawable);
        LayerDrawable d2 = (LayerDrawable) mContext.getDrawable(R.drawable.layerdrawable);
        LayerDrawable d3 = (LayerDrawable) mContext.getDrawable(R.drawable.layerdrawable);

        d1.setAlpha(100);
        assertEquals(100, ((BitmapDrawable) d1.getDrawable(0)).getPaint().getAlpha());
        assertEquals(100, ((BitmapDrawable) d1.getDrawable(0)).getPaint().getAlpha());
        assertEquals(100, ((BitmapDrawable) d2.getDrawable(0)).getPaint().getAlpha());
        assertEquals(100, ((BitmapDrawable) d2.getDrawable(0)).getPaint().getAlpha());
        assertEquals(100, ((BitmapDrawable) d3.getDrawable(0)).getPaint().getAlpha());
        assertEquals(100, ((BitmapDrawable) d2.getDrawable(0)).getPaint().getAlpha());

        d1.mutate();
        d1.setAlpha(200);
        assertEquals(200, ((BitmapDrawable) d1.getDrawable(0)).getPaint().getAlpha());
        assertEquals(200, ((BitmapDrawable) d1.getDrawable(0)).getPaint().getAlpha());
        assertEquals(100, ((BitmapDrawable) d2.getDrawable(0)).getPaint().getAlpha());
        assertEquals(100, ((BitmapDrawable) d2.getDrawable(0)).getPaint().getAlpha());
        assertEquals(100, ((BitmapDrawable) d3.getDrawable(0)).getPaint().getAlpha());
        assertEquals(100, ((BitmapDrawable) d3.getDrawable(0)).getPaint().getAlpha());

        d2.setAlpha(50);
        assertEquals(200, ((BitmapDrawable) d1.getDrawable(0)).getPaint().getAlpha());
        assertEquals(200, ((BitmapDrawable) d1.getDrawable(0)).getPaint().getAlpha());
        assertEquals(50, ((BitmapDrawable) d2.getDrawable(0)).getPaint().getAlpha());
        assertEquals(50, ((BitmapDrawable) d2.getDrawable(0)).getPaint().getAlpha());
        assertEquals(50, ((BitmapDrawable) d3.getDrawable(0)).getPaint().getAlpha());
        assertEquals(50, ((BitmapDrawable) d3.getDrawable(0)).getPaint().getAlpha());
    }


    public void testPreloadDensity() throws XmlPullParserException, IOException {
        final Resources res = getContext().getResources();
        final int densityDpi = res.getConfiguration().densityDpi;
        try {
            testPreloadDensityInner(res, densityDpi);
        } finally {
            DrawableTestUtils.setResourcesDensity(res, densityDpi);
        }
    }

    private void testPreloadDensityInner(Resources res, int densityDpi)
            throws XmlPullParserException, IOException {
        // Capture initial state at default density.
        final XmlResourceParser parser = DrawableTestUtils.getResourceParser(
                res, R.drawable.layer_drawable_density);
        final LayerDrawable preloadedDrawable = new LayerDrawable(new Drawable[0]);
        preloadedDrawable.inflate(res, parser, Xml.asAttributeSet(parser));
        final ConstantState preloadedConstantState = preloadedDrawable.getConstantState();
        final int initialLeftPadding = preloadedDrawable.getLeftPadding();
        final int initialRightPadding = preloadedDrawable.getRightPadding();
        final int initialContentInsetL = preloadedDrawable.getLayerInsetLeft(0);
        final int initialContentInsetR = preloadedDrawable.getLayerInsetRight(0);
        final int initialContentWidth = preloadedDrawable.getLayerWidth(0);
        final int initialIntrinsicWidth = preloadedDrawable.getIntrinsicWidth();
        final int initialBottomPadding = preloadedDrawable.getBottomPadding();
        final int initialTopPadding = preloadedDrawable.getTopPadding();
        final int initialLayerInsetLeft = preloadedDrawable.getLayerInsetLeft(0);
        final int initialLayerInsetRight = preloadedDrawable.getLayerInsetRight(0);
        final int initialLayerInsetTop = preloadedDrawable.getLayerInsetTop(0);
        final int initialLayerInsetBottom = preloadedDrawable.getLayerInsetBottom(0);
        final int initialLayerWidth = preloadedDrawable.getLayerWidth(0);
        final int initialLayerHeight = preloadedDrawable.getLayerHeight(0);

        // Set density to half of original. Padding and insets are
        // truncated, dimensions are rounded to the nearest pixel. Because
        // LayerDrawable's intrinsic width is a combination of padding and
        // dimensions, some computation is necessary.
        DrawableTestUtils.setResourcesDensity(res, densityDpi / 2);
        final LayerDrawable halfDrawable =
                (LayerDrawable) preloadedConstantState.newDrawable(res);
        final int halfContentWidth = Math.round(initialContentWidth / 2f);
        final int halfLeftPadding = initialLeftPadding / 2;
        final int halfRightPadding = initialRightPadding / 2;
        final int halfContentInsetL = initialContentInsetL / 2;
        final int halfContentInsetR = initialContentInsetR / 2;
        final int halfIntrinsicWidth = halfContentWidth + halfContentInsetL + halfContentInsetR;
        assertEquals(halfLeftPadding, halfDrawable.getLeftPadding());
        assertEquals(halfRightPadding, halfDrawable.getRightPadding());
        assertEquals(halfContentInsetL, halfDrawable.getLayerInsetRight(0));
        assertEquals(halfContentInsetR, halfDrawable.getLayerInsetLeft(0));
        assertEquals(halfContentWidth, halfDrawable.getLayerWidth(0));
        assertEquals(halfIntrinsicWidth, halfDrawable.getIntrinsicWidth());
        assertEquals(initialBottomPadding / 2, halfDrawable.getBottomPadding());
        assertEquals(initialTopPadding / 2, halfDrawable.getTopPadding());
        assertEquals(initialLayerInsetLeft / 2,halfDrawable.getLayerInsetLeft(0));
        assertEquals(initialLayerInsetRight / 2, halfDrawable.getLayerInsetRight(0));
        assertEquals(initialLayerInsetTop / 2, halfDrawable.getLayerInsetTop(0));
        assertEquals(initialLayerInsetBottom / 2, halfDrawable.getLayerInsetBottom(0));
        assertEquals(Math.round(initialLayerWidth / 2f), halfDrawable.getLayerWidth(0));
        assertEquals(Math.round(initialLayerHeight / 2f), halfDrawable.getLayerHeight(0));

        // Set density to double original.
        DrawableTestUtils.setResourcesDensity(res, densityDpi * 2);
        final LayerDrawable doubleDrawable =
                (LayerDrawable) preloadedConstantState.newDrawable(res);
        assertEquals(initialLeftPadding * 2, doubleDrawable.getLeftPadding());
        assertEquals(initialRightPadding * 2, doubleDrawable.getRightPadding());
        assertEquals(initialContentInsetL * 2, doubleDrawable.getLayerInsetRight(0));
        assertEquals(initialContentInsetR * 2, doubleDrawable.getLayerInsetLeft(0));
        assertEquals(initialContentWidth * 2, doubleDrawable.getLayerWidth(0));
        assertEquals(initialIntrinsicWidth * 2, doubleDrawable.getIntrinsicWidth());
        assertEquals(initialBottomPadding * 2, doubleDrawable.getBottomPadding());
        assertEquals(initialTopPadding * 2, doubleDrawable.getTopPadding());
        assertEquals(initialLayerInsetLeft * 2, doubleDrawable.getLayerInsetLeft(0));
        assertEquals(initialLayerInsetRight * 2, doubleDrawable.getLayerInsetRight(0));
        assertEquals(initialLayerInsetTop * 2, doubleDrawable.getLayerInsetTop(0));
        assertEquals(initialLayerInsetBottom * 2, doubleDrawable.getLayerInsetBottom(0));
        assertEquals(initialLayerWidth * 2, doubleDrawable.getLayerWidth(0));
        assertEquals(initialLayerHeight * 2, doubleDrawable.getLayerHeight(0));

        // Restore original configuration and metrics.
        DrawableTestUtils.setResourcesDensity(res, densityDpi);
        final LayerDrawable origDrawable =
                (LayerDrawable) preloadedConstantState.newDrawable(res);
        assertEquals(initialLeftPadding, origDrawable.getLeftPadding());
        assertEquals(initialRightPadding, origDrawable.getRightPadding());
        assertEquals(initialContentInsetL, origDrawable.getLayerInsetRight(0));
        assertEquals(initialContentInsetR, origDrawable.getLayerInsetLeft(0));
        assertEquals(initialContentWidth, origDrawable.getLayerWidth(0));
        assertEquals(initialIntrinsicWidth, origDrawable.getIntrinsicWidth());
        assertEquals(initialBottomPadding, origDrawable.getBottomPadding());
        assertEquals(initialTopPadding, origDrawable.getTopPadding());
        assertEquals(initialLayerInsetLeft, origDrawable.getLayerInsetLeft(0));
        assertEquals(initialLayerInsetRight, origDrawable.getLayerInsetRight(0));
        assertEquals(initialLayerInsetTop, origDrawable.getLayerInsetTop(0));
        assertEquals(initialLayerInsetBottom, origDrawable.getLayerInsetBottom(0));
        assertEquals(initialLayerWidth, origDrawable.getLayerWidth(0));
        assertEquals(initialLayerHeight, origDrawable.getLayerHeight(0));

        // Ensure theme density is applied correctly.
        final Theme t = res.newTheme();

        // The half-density drawable will scale-up all of the values that were
        // previously scaled-down, so we need to capture the rounding errors.
        halfDrawable.applyTheme(t);
        assertEquals(halfLeftPadding * 2, halfDrawable.getLeftPadding());
        assertEquals(halfRightPadding * 2, halfDrawable.getRightPadding());
        assertEquals(halfContentInsetL * 2, halfDrawable.getLayerInsetRight(0));
        assertEquals(halfContentInsetR * 2, halfDrawable.getLayerInsetLeft(0));
        assertEquals(halfContentWidth * 2, halfDrawable.getLayerWidth(0));
        assertEquals(halfIntrinsicWidth * 2, halfDrawable.getIntrinsicWidth());
        assertEquals(2 * (initialBottomPadding / 2), halfDrawable.getBottomPadding());
        assertEquals(2 * (initialTopPadding / 2), halfDrawable.getTopPadding());
        assertEquals(2 * (initialLayerInsetLeft / 2), halfDrawable.getLayerInsetLeft(0));
        assertEquals(2 * (initialLayerInsetRight / 2), halfDrawable.getLayerInsetRight(0));
        assertEquals(2 * (initialLayerInsetTop / 2), halfDrawable.getLayerInsetTop(0));
        assertEquals(2 * (initialLayerInsetBottom / 2), halfDrawable.getLayerInsetBottom(0));
        assertEquals(2 * Math.round(initialLayerWidth / 2f), halfDrawable.getLayerWidth(0));
        assertEquals(2 * Math.round(initialLayerHeight / 2f), halfDrawable.getLayerHeight(0));

        // The double-density drawable will scale-down all of the values that
        // were previously scaled-up, so we don't need to worry about rounding.
        doubleDrawable.applyTheme(t);
        assertEquals(initialLeftPadding, doubleDrawable.getLeftPadding());
        assertEquals(initialRightPadding, doubleDrawable.getRightPadding());
        assertEquals(initialContentInsetL, doubleDrawable.getLayerInsetRight(0));
        assertEquals(initialContentInsetR, doubleDrawable.getLayerInsetLeft(0));
        assertEquals(initialContentWidth, doubleDrawable.getLayerWidth(0));
        assertEquals(initialIntrinsicWidth, doubleDrawable.getIntrinsicWidth());
        assertEquals(initialBottomPadding, doubleDrawable.getBottomPadding());
        assertEquals(initialTopPadding, doubleDrawable.getTopPadding());
        assertEquals(initialLayerInsetLeft, doubleDrawable.getLayerInsetLeft(0));
        assertEquals(initialLayerInsetRight, doubleDrawable.getLayerInsetRight(0));
        assertEquals(initialLayerInsetTop, doubleDrawable.getLayerInsetTop(0));
        assertEquals(initialLayerInsetBottom, doubleDrawable.getLayerInsetBottom(0));
        assertEquals(initialLayerWidth, doubleDrawable.getLayerWidth(0));
        assertEquals(initialLayerHeight, doubleDrawable.getLayerHeight(0));
    }

    private static class IntrinsicSizeDrawable extends Drawable {
        private final int mIntrinsicWidth;
        private final int mIntrinsicHeight;
        private final Rect mPadding;

        public IntrinsicSizeDrawable(int intrinsicWidth, int intrinsicHeight, Rect padding) {
            mIntrinsicWidth = intrinsicWidth;
            mIntrinsicHeight = intrinsicHeight;
            mPadding = new Rect(padding);
        }

        @Override
        public boolean getPadding(Rect padding) {
            padding.set(mPadding);
            return true;
        }

        @Override
        public int getIntrinsicHeight() {
            return mIntrinsicHeight;
        }

        @Override
        public int getIntrinsicWidth() {
            return mIntrinsicWidth;
        }

        @Override
        public void draw(Canvas canvas) {
        }

        @Override
        public void setAlpha(int alpha) {
        }

        @Override
        public void setColorFilter(ColorFilter colorFilter) {
        }

        @Override
        public int getOpacity() {
            return 0;
        }
    }
}
