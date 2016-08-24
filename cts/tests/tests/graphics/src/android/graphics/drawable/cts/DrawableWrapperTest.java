/*
 * Copyright (C) 2015 The Android Open Source Project.
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

import android.graphics.drawable.DrawableWrapper;
import android.graphics.cts.R;


import java.util.Arrays;

import android.graphics.Canvas;
import android.graphics.ColorFilter;
import android.graphics.Rect;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.Drawable.ConstantState;
import android.test.AndroidTestCase;
import android.util.StateSet;

public class DrawableWrapperTest extends AndroidTestCase {

    static class MyWrapper extends DrawableWrapper {
        public MyWrapper(Drawable dr) {
            super(dr);
        }
    }

    @SuppressWarnings("deprecation")
    public void testConstructor() {
        Drawable d = new BitmapDrawable();
        DrawableWrapper wrapper = new MyWrapper(d);
        assertSame(d, wrapper.getDrawable());

        new MyWrapper(null);
    }

    @SuppressWarnings("deprecation")
    public void testGetDrawable() {
        Drawable d = new BitmapDrawable();
        DrawableWrapper wrapper = new MyWrapper(d);
        assertSame(d, wrapper.getDrawable());
    }

    @SuppressWarnings("deprecation")
    public void testSetDrawable() {
        Drawable d = new BitmapDrawable();
        DrawableWrapper wrapper = new MyWrapper(null);
        assertSame(null, wrapper.getDrawable());

        wrapper.setDrawable(d);
        assertSame(d, wrapper.getDrawable());
    }

    @SuppressWarnings("deprecation")
    public void testInvalidateDrawable() {
        DrawableWrapper wrapper = new MyWrapper(new BitmapDrawable());

        MockCallback cb = new MockCallback();
        wrapper.setCallback(cb);
        wrapper.invalidateDrawable(null);
        assertTrue(cb.hasCalledInvalidate());

        cb.reset();
        wrapper.invalidateDrawable(new BitmapDrawable());
        assertTrue(cb.hasCalledInvalidate());

        cb.reset();
        wrapper.setCallback(null);
        wrapper.invalidateDrawable(null);
        assertFalse(cb.hasCalledInvalidate());
    }

    @SuppressWarnings("deprecation")
    public void testScheduleDrawable() {
        DrawableWrapper wrapper = new MyWrapper(new BitmapDrawable());

        MockCallback cb = new MockCallback();
        wrapper.setCallback(cb);
        wrapper.scheduleDrawable(null, null, 0);
        assertTrue(cb.hasCalledSchedule());

        cb.reset();
        wrapper.scheduleDrawable(new BitmapDrawable(), new Runnable() {
            public void run() {
            }
        }, 1000L);
        assertTrue(cb.hasCalledSchedule());

        cb.reset();
        wrapper.setCallback(null);
        wrapper.scheduleDrawable(null, null, 0);
        assertFalse(cb.hasCalledSchedule());
    }

    @SuppressWarnings("deprecation")
    public void testUnscheduleDrawable() {
        DrawableWrapper wrapper = new MyWrapper(new BitmapDrawable());

        MockCallback cb = new MockCallback();
        wrapper.setCallback(cb);
        wrapper.unscheduleDrawable(null, null);
        assertTrue(cb.hasCalledUnschedule());

        cb.reset();
        wrapper.unscheduleDrawable(new BitmapDrawable(), new Runnable() {
            public void run() {
            }
        });
        assertTrue(cb.hasCalledUnschedule());

        cb.reset();
        wrapper.setCallback(null);
        wrapper.unscheduleDrawable(null, null);
        assertFalse(cb.hasCalledUnschedule());
    }

    private static class MockCallback implements Drawable.Callback {
        private boolean mCalledInvalidate;
        private boolean mCalledSchedule;
        private boolean mCalledUnschedule;

        public void invalidateDrawable(Drawable who) {
            mCalledInvalidate = true;
        }

        public void scheduleDrawable(Drawable who, Runnable what, long when) {
            mCalledSchedule = true;
        }

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
        MockDrawable mockDrawable = new MockDrawable();
        DrawableWrapper wrapper = new MyWrapper(mockDrawable);

        wrapper.draw(new Canvas());
        assertTrue(mockDrawable.hasCalledDraw());

        mockDrawable.reset();
        wrapper.draw(null);
        assertTrue(mockDrawable.hasCalledDraw());
    }

    public void testGetChangingConfigurations() {
        final int SUPER_CONFIG = 1;
        final int CONTAINED_DRAWABLE_CONFIG = 2;

        MockDrawable mockDrawable = new MockDrawable();
        DrawableWrapper wrapper = new MyWrapper(mockDrawable);

        assertEquals(0, wrapper.getChangingConfigurations());

        mockDrawable.setChangingConfigurations(CONTAINED_DRAWABLE_CONFIG);
        assertEquals(CONTAINED_DRAWABLE_CONFIG, wrapper.getChangingConfigurations());

        wrapper.setChangingConfigurations(SUPER_CONFIG);
        assertEquals(SUPER_CONFIG | CONTAINED_DRAWABLE_CONFIG,
                wrapper.getChangingConfigurations());
    }

    public void testGetPadding() {
        MockDrawable mockDrawable = new MockDrawable();
        DrawableWrapper wrapper = new MyWrapper(mockDrawable);

        // this method will call contained drawable's getPadding method.
        wrapper.getPadding(new Rect());
        assertTrue(mockDrawable.hasCalledGetPadding());

        // input null as param
        try {
            wrapper.getPadding(null);
            fail("Should throw NullPointerException");
        } catch (NullPointerException e) {
        }
    }

    public void testSetVisible() {
        MockDrawable mockDrawable = new MockDrawable();
        DrawableWrapper wrapper = new MyWrapper(mockDrawable);
        assertTrue(wrapper.isVisible());

        assertTrue(wrapper.setVisible(false, false));
        assertFalse(wrapper.isVisible());
        assertTrue(mockDrawable.hasCalledSetVisible());

        mockDrawable.reset();
        assertFalse(wrapper.setVisible(false, false));
        assertFalse(wrapper.isVisible());
        assertTrue(mockDrawable.hasCalledSetVisible());

        mockDrawable.reset();
        assertTrue(wrapper.setVisible(true, false));
        assertTrue(wrapper.isVisible());
        assertTrue(mockDrawable.hasCalledSetVisible());
    }

    public void testSetAlpha() {
        MockDrawable mockDrawable = new MockDrawable();
        DrawableWrapper wrapper = new MyWrapper(mockDrawable);

        // this method will call contained drawable's setAlpha method.
        wrapper.setAlpha(100);
        assertTrue(mockDrawable.hasCalledSetAlpha());

        mockDrawable.reset();
        wrapper.setAlpha(Integer.MAX_VALUE);
        assertTrue(mockDrawable.hasCalledSetAlpha());

        mockDrawable.reset();
        wrapper.setAlpha(-1);
        assertTrue(mockDrawable.hasCalledSetAlpha());
    }

    public void testSetColorFilter() {
        MockDrawable mockDrawable = new MockDrawable();
        DrawableWrapper wrapper = new MyWrapper(mockDrawable);

        // this method will call contained drawable's setColorFilter method.
        wrapper.setColorFilter(new ColorFilter());
        assertTrue(mockDrawable.hasCalledSetColorFilter());

        mockDrawable.reset();
        wrapper.setColorFilter(null);
        assertTrue(mockDrawable.hasCalledSetColorFilter());
    }

    public void testGetOpacity() {
        MockDrawable mockDrawable = new MockDrawable();
        DrawableWrapper wrapper = new MyWrapper(mockDrawable);

        // This method will call contained drawable's getOpacity method.
        wrapper.setLevel(1);
        wrapper.getOpacity();
        assertTrue(mockDrawable.hasCalledGetOpacity());
    }

    public void testIsStateful() {
        MockDrawable mockDrawable = new MockDrawable();
        DrawableWrapper wrapper = new MyWrapper(mockDrawable);

        // this method will call contained drawable's isStateful method.
        wrapper.isStateful();
        assertTrue(mockDrawable.hasCalledIsStateful());
    }

    public void testOnStateChange() {
        Drawable d = new MockDrawable();
        MockDrawableWrapper wrapper = new MockDrawableWrapper(d);
        assertEquals("initial child state is empty", d.getState(), StateSet.WILD_CARD);

        int[] state = new int[] {1, 2, 3};
        assertFalse("child did not change", wrapper.onStateChange(state));
        assertEquals("child state did not change", d.getState(), StateSet.WILD_CARD);

        d = mContext.getDrawable(R.drawable.statelistdrawable);
        wrapper = new MockDrawableWrapper(d);
        assertEquals("initial child state is empty", d.getState(), StateSet.WILD_CARD);
        wrapper.onStateChange(state);
        assertTrue("child state changed", Arrays.equals(state, d.getState()));

        // input null as param
        wrapper.onStateChange(null);
        // expected, no Exception thrown out, test success
    }

    public void testOnLevelChange() {
        MockDrawable mockDrawable = new MockDrawable();
        MockDrawableWrapper mockDrawableWrapper = new MockDrawableWrapper(mockDrawable);

        assertEquals(0, mockDrawable.getLevel());
        assertFalse(mockDrawableWrapper.onLevelChange(0));
        assertFalse(mockDrawable.hasCalledOnLevelChange());

        assertFalse(mockDrawableWrapper.onLevelChange(1000));
        assertTrue(mockDrawable.hasCalledOnLevelChange());
        assertEquals(1000, mockDrawable.getLevel());

        mockDrawable.reset();
        mockDrawableWrapper.reset();
        assertFalse(mockDrawableWrapper.onLevelChange(Integer.MIN_VALUE));
        assertTrue(mockDrawable.hasCalledOnLevelChange());
    }

    public void testOnBoundsChange() {
        MockDrawable mockDrawable = new MockDrawable();
        MockDrawableWrapper mockDrawableWrapper = new MockDrawableWrapper(mockDrawable);
        Rect bounds = new Rect(2, 2, 26, 32);
        mockDrawable.setBounds(bounds);
        mockDrawableWrapper.onBoundsChange(bounds);

        mockDrawableWrapper = new MockDrawableWrapper(mockDrawable);
        mockDrawable.setBounds(bounds);
        mockDrawableWrapper.onBoundsChange(bounds);
        assertEquals(bounds.left, mockDrawable.getBounds().left);
        assertEquals(bounds.top, mockDrawable.getBounds().top);
        assertEquals(bounds.right, mockDrawable.getBounds().right);
        assertEquals(bounds.bottom, mockDrawable.getBounds().bottom);

        bounds = mockDrawable.getBounds();
        assertEquals(2, bounds.left);
        assertEquals(2, bounds.top);
        assertEquals(26, bounds.right);
        assertEquals(32, bounds.bottom);

        // input null as param
        try {
            mockDrawableWrapper.onBoundsChange(null);
            fail("There should be a NullPointerException thrown out.");
        } catch (NullPointerException e) {
            // expected, test success
        }

    }

    public void testGetIntrinsicWidth() {
        MockDrawable mockDrawable = new MockDrawable();
        MyWrapper wrapper = new MyWrapper(mockDrawable);

        // this method will call contained drawable's getIntrinsicWidth method.
        wrapper.getIntrinsicWidth();
        assertTrue(mockDrawable.hasCalledGetIntrinsicWidth());
    }

    public void testGetIntrinsicHeight() {
        MockDrawable mockDrawable = new MockDrawable();
        DrawableWrapper wrapper = new MyWrapper(mockDrawable);

        // this method will call contained drawable's getIntrinsicHeight method.
        wrapper.getIntrinsicHeight();
        assertTrue(mockDrawable.hasCalledGetIntrinsicHeight());
    }

    @SuppressWarnings("deprecation")
    public void testGetConstantState() {
        DrawableWrapper wrapper = new MyWrapper(new BitmapDrawable());
        ConstantState constantState = wrapper.getConstantState();
    }

    private static class MockDrawable extends Drawable {
        private boolean mCalledDraw = false;
        private boolean mCalledGetPadding = false;
        private boolean mCalledSetVisible = false;
        private boolean mCalledSetAlpha = false;
        private boolean mCalledGetOpacity = false;
        private boolean mCalledSetColorFilter = false;
        private boolean mCalledIsStateful = false;
        private boolean mCalledGetIntrinsicWidth = false;
        private boolean mCalledGetIntrinsicHeight = false;
        private boolean mCalledSetState = false;
        private boolean mCalledOnLevelChange = false;

        @Override
        public void draw(Canvas canvas) {
            mCalledDraw = true;
        }

        @Override
        public int getOpacity() {
            mCalledGetOpacity = true;
            return 0;
        }

        @Override
        public void setAlpha(int alpha) {
            mCalledSetAlpha = true;
        }

        @Override
        public void setColorFilter(ColorFilter cf) {
            mCalledSetColorFilter = true;
        }

        @Override
        public boolean getPadding(Rect padding) {
            mCalledGetPadding = true;
            return super.getPadding(padding);
        }

        @Override
        public boolean setVisible(boolean visible, boolean restart) {
            mCalledSetVisible = true;
            return super.setVisible(visible, restart);
        }

        @Override
        public boolean isStateful() {
            mCalledIsStateful = true;
            return super.isStateful();
        }

        @Override
        public int getIntrinsicWidth() {
            mCalledGetIntrinsicWidth = true;
            return super.getIntrinsicWidth();
        }

        @Override
        public int getIntrinsicHeight() {
            mCalledGetIntrinsicHeight = true;
            return super.getIntrinsicHeight();

        }

        @Override
        public boolean setState(final int[] stateSet) {
            mCalledSetState = true;
            return super.setState(stateSet);
        }

        @Override
        protected boolean onLevelChange(int level) {
            mCalledOnLevelChange = true;
            return super.onLevelChange(level);
        }

        public boolean hasCalledDraw() {
            return mCalledDraw;
        }

        public boolean hasCalledGetPadding() {
            return mCalledGetPadding;
        }

        public boolean hasCalledSetVisible() {
            return mCalledSetVisible;
        }

        public boolean hasCalledSetAlpha() {
            return mCalledSetAlpha;
        }

        public boolean hasCalledGetOpacity() {
            return mCalledGetOpacity;
        }

        public boolean hasCalledSetColorFilter() {
            return mCalledSetColorFilter;
        }

        public boolean hasCalledIsStateful() {
            return mCalledIsStateful;
        }

        public boolean hasCalledGetIntrinsicWidth() {
            return mCalledGetIntrinsicWidth;
        }

        public boolean hasCalledGetIntrinsicHeight() {
            return mCalledGetIntrinsicHeight;
        }

        public boolean hasCalledSetState() {
            return mCalledSetState;
        }

        public boolean hasCalledOnLevelChange() {
            return mCalledOnLevelChange;
        }

        public void reset() {
            mCalledDraw = false;
            mCalledGetPadding = false;
            mCalledSetVisible = false;
            mCalledSetAlpha = false;
            mCalledGetOpacity = false;
            mCalledSetColorFilter = false;
            mCalledIsStateful = false;
            mCalledGetIntrinsicWidth = false;
            mCalledGetIntrinsicHeight = false;
            mCalledSetState = false;
            mCalledOnLevelChange = false;
        }
    }

    private static class MockDrawableWrapper extends DrawableWrapper {
        private boolean mCalledOnBoundsChange = false;

        MockDrawableWrapper() {
            super(null);
        }

        public MockDrawableWrapper(Drawable drawable) {
            super(drawable);
        }

        @Override
        protected boolean onStateChange(int[] state) {
            return super.onStateChange(state);
        }

        @Override
        protected boolean onLevelChange(int level) {
            return super.onLevelChange(level);
        }

        @Override
        protected void onBoundsChange(Rect bounds) {
            mCalledOnBoundsChange = true;
            super.onBoundsChange(bounds);
        }

        public boolean hasCalledOnBoundsChange() {
            return mCalledOnBoundsChange;
        }

        public void reset() {
            mCalledOnBoundsChange = false;
        }
    }
}
