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

import junit.framework.TestCase;

import java.util.Arrays;

import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.ColorFilter;
import android.graphics.PixelFormat;
import android.graphics.PorterDuff.Mode;
import android.graphics.Rect;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.DrawableContainer;
import android.graphics.drawable.DrawableContainer.DrawableContainerState;
import android.graphics.drawable.LevelListDrawable;

public class DrawableContainerTest extends TestCase {
    private DrawableContainerState mDrawableContainerState;

    private MockDrawableContainer mMockDrawableContainer;
    private DrawableContainer mDrawableContainer;

    @Override
    protected void setUp() throws Exception {
        super.setUp();

        // DrawableContainerState has no public constructor. Obtain an instance through
        // LevelListDrawable.getConstants(). This is fine for testing the final methods of
        // DrawableContainerState.
        mDrawableContainerState =
            (DrawableContainerState) new LevelListDrawable().getConstantState();
        assertNotNull(mDrawableContainerState);

        mMockDrawableContainer = new MockDrawableContainer();
        mDrawableContainer = mMockDrawableContainer;
    }

    public void testDraw() {
        assertConstantStateNotSet();
        assertNull(mDrawableContainer.getCurrent());

        mDrawableContainer.draw(null);
        mDrawableContainer.draw(new Canvas());

        mMockDrawableContainer.setConstantState(mDrawableContainerState);
        MockDrawable dr = new MockDrawable();
        addAndSelectDrawable(dr);

        dr.reset();
        mDrawableContainer.draw(null);
        assertTrue(dr.hasDrawCalled());

        dr.reset();
        mDrawableContainer.draw(new Canvas());
        assertTrue(dr.hasDrawCalled());
    }

    public void testSetEnterFadeDuration() {
        helpTestSetEnterFadeDuration(1000);
        helpTestSetEnterFadeDuration(0);
    }

    private void helpTestSetEnterFadeDuration(int enterFadeDuration) {
        DrawableContainer container = new LevelListDrawable();
        DrawableContainerState cs = ((DrawableContainerState) container.getConstantState());
        container.setEnterFadeDuration(enterFadeDuration);
        assertEquals(enterFadeDuration, cs.getEnterFadeDuration());
    }

    public void testSetExitFadeDuration() {
        helpTestSetExitFadeDuration(1000);
        helpTestSetExitFadeDuration(0);
    }

    private void helpTestSetExitFadeDuration(int exitFadeDuration) {
        DrawableContainer container = new LevelListDrawable();
        DrawableContainerState cs = ((DrawableContainerState) container.getConstantState());
        container.setExitFadeDuration(exitFadeDuration);
        assertEquals(exitFadeDuration, cs.getExitFadeDuration());
    }

    public void testGetChangingConfigurations() {
        // Workaround for CTS coverage not recognizing calls on subclasses.
        DrawableContainer dr = mDrawableContainer;

        assertConstantStateNotSet();

        try {
            mDrawableContainer.getChangingConfigurations();
            fail("Should throw NullPointerException if the constant state is not set.");
        } catch (NullPointerException e) {
        }

        mMockDrawableContainer.setConstantState(mDrawableContainerState);
        MockDrawable dr0 = new MockDrawable();
        dr0.setChangingConfigurations(0x001);
        mDrawableContainerState.addChild(dr0);
        MockDrawable dr1 = new MockDrawable();
        dr1.setChangingConfigurations(0x010);
        mDrawableContainerState.addChild(dr1);
        dr.selectDrawable(0);
        assertSame(dr0, mDrawableContainer.getCurrent());

        // can not set mDrawableContainerState's ChangingConfigurations
        mDrawableContainer.setChangingConfigurations(0x100);
        assertEquals(0x111 | mDrawableContainerState.getChangingConfigurations(),
                mDrawableContainer.getChangingConfigurations());
    }

    public void testGetPadding() {
        // Workaround for CTS coverage not recognizing calls on subclasses.
        DrawableContainer dr = mDrawableContainer;

        assertConstantStateNotSet();
        assertNull(mDrawableContainer.getCurrent());

        Rect result = new Rect(1, 1, 1, 1);
        try {
            mDrawableContainer.getPadding(result);
            fail("Should throw NullPointerException if the constant state is not set.");
        } catch (NullPointerException e) {
        }

        mMockDrawableContainer.setConstantState(mDrawableContainerState);
        MockDrawable dr0 = new MockDrawable();
        dr0.setPadding(new Rect(1, 2, 0, 0));
        mDrawableContainerState.addChild(dr0);
        MockDrawable dr1 = new MockDrawable();
        dr1.setPadding(new Rect(0, 0, 3, 4));
        mDrawableContainerState.addChild(dr1);
        dr.selectDrawable(0);
        assertSame(dr0, mDrawableContainer.getCurrent());

        // use the current drawable's padding
        mDrawableContainerState.setVariablePadding(true);
        assertNull(mDrawableContainerState.getConstantPadding());
        assertTrue(mDrawableContainer.getPadding(result));
        assertEquals(new Rect(1, 2, 0, 0), result);

        // use constant state's padding
        mDrawableContainerState.setVariablePadding(false);
        assertNotNull(mDrawableContainerState.getConstantPadding());
        assertTrue(mDrawableContainer.getPadding(result));
        assertEquals(mDrawableContainerState.getConstantPadding(), result);

        // use default padding
        dr.selectDrawable(-1);
        assertNull(mDrawableContainer.getCurrent());
        mDrawableContainerState.setVariablePadding(true);
        assertNull(mDrawableContainerState.getConstantPadding());
        assertFalse(mDrawableContainer.getPadding(result));
        assertEquals(new Rect(0, 0, 0, 0), result);

        try {
            mDrawableContainer.getPadding(null);
            fail("Should throw NullPointerException if the padding is null.");
        } catch (NullPointerException e) {
        }
    }

    public void testSetAlpha() {
        // Workaround for CTS coverage not recognizing calls on subclasses.
        DrawableContainer dr = mDrawableContainer;

        assertConstantStateNotSet();
        assertNull(mDrawableContainer.getCurrent());

        dr.setAlpha(0);

        mMockDrawableContainer.setConstantState(mDrawableContainerState);
        MockDrawable mockDrawable = new MockDrawable();
        addAndSelectDrawable(mockDrawable);

        // call current drawable's setAlpha if alpha is changed.
        mockDrawable.reset();
        dr.setAlpha(1);
        assertTrue(mockDrawable.hasSetAlphaCalled());

        // does not call it if alpha is not changed.
        mockDrawable.reset();
        dr.setAlpha(1);
        assertFalse(mockDrawable.hasSetAlphaCalled());
    }

    public void testSetDither() {
        assertConstantStateNotSet();
        assertNull(mDrawableContainer.getCurrent());

        mMockDrawableContainer.setConstantState(mDrawableContainerState);
        mDrawableContainer.setDither(false);
        mDrawableContainer.setDither(true);

        MockDrawable dr = new MockDrawable();
        addAndSelectDrawable(dr);

        // call current drawable's setDither if dither is changed.
        dr.reset();
        mDrawableContainer.setDither(false);
        assertTrue(dr.hasSetDitherCalled());

        // does not call it if dither is not changed.
        dr.reset();
        mDrawableContainer.setDither(true);
        assertTrue(dr.hasSetDitherCalled());
    }

    public void testSetHotspotBounds() {
        Rect bounds = new Rect(10, 15, 100, 150);
        assertConstantStateNotSet();
        assertNull(mDrawableContainer.getCurrent());

        mMockDrawableContainer.setConstantState(mDrawableContainerState);

        MockDrawable dr = new MockDrawable();
        addAndSelectDrawable(dr);

        dr.reset();
        mDrawableContainer.setHotspotBounds(bounds.left, bounds.top, bounds.right, bounds.bottom);
        Rect outRect = new Rect();
        mDrawableContainer.getHotspotBounds(outRect);
        assertEquals(bounds, outRect);

        dr.reset();
    }

    public void testGetHotspotBounds() {
        Rect bounds = new Rect(10, 15, 100, 150);
        assertConstantStateNotSet();
        assertNull(mDrawableContainer.getCurrent());

        mMockDrawableContainer.setConstantState(mDrawableContainerState);

        MockDrawable dr = new MockDrawable();
        addAndSelectDrawable(dr);

        dr.reset();
        mDrawableContainer.setHotspotBounds(bounds.left, bounds.top, bounds.right, bounds.bottom);
        Rect outRect = new Rect();
        mDrawableContainer.getHotspotBounds(outRect);
        assertEquals(bounds, outRect);

        dr.reset();
    }

    public void testSetColorFilter() {
        // Workaround for CTS coverage not recognizing calls on subclasses.
        DrawableContainer dr = mDrawableContainer;

        assertConstantStateNotSet();
        assertNull(mDrawableContainer.getCurrent());

        mMockDrawableContainer.setConstantState(mDrawableContainerState);
        dr.setColorFilter(null);
        dr.setColorFilter(new ColorFilter());

        MockDrawable mockDrawable = new MockDrawable();
        addAndSelectDrawable(mockDrawable);

        // call current drawable's setColorFilter if filter is changed.
        mockDrawable.reset();
        dr.setColorFilter(null);
        assertTrue(mockDrawable.hasSetColorFilterCalled());

        // does not call it if filter is not changed.
        mockDrawable.reset();
        dr.setColorFilter(new ColorFilter());
        assertTrue(mockDrawable.hasSetColorFilterCalled());
    }

    public void testSetTint() {
        assertConstantStateNotSet();
        assertNull(mDrawableContainer.getCurrent());

        mMockDrawableContainer.setConstantState(mDrawableContainerState);
        mDrawableContainer.setTint(Color.BLACK);
        mDrawableContainer.setTintMode(Mode.SRC_OVER);

        MockDrawable dr = new MockDrawable();
        addAndSelectDrawable(dr);

        assertEquals("Initial tint propagates", Mode.SRC_OVER, dr.getTintMode());

        dr.reset();
        mDrawableContainer.setTintList(null);
        mDrawableContainer.setTintMode(null);
        assertTrue("setImageTintList() propagates", dr.hasSetTintCalled());
    }

    public void testOnBoundsChange() {
        assertConstantStateNotSet();
        assertNull(mDrawableContainer.getCurrent());

        mMockDrawableContainer.onBoundsChange(new Rect());
        mMockDrawableContainer.onBoundsChange(null);

        mMockDrawableContainer.setConstantState(mDrawableContainerState);
        MockDrawable dr = new MockDrawable();
        dr.setBounds(new Rect());
        addAndSelectDrawable(dr);

        // set current drawable's bounds.
        dr.reset();
        assertEquals(new Rect(), dr.getBounds());
        mMockDrawableContainer.onBoundsChange(new Rect(1, 1, 1, 1));
        assertTrue(dr.hasOnBoundsChangedCalled());
        assertEquals(new Rect(1, 1, 1, 1), dr.getBounds());

        dr.reset();
        mMockDrawableContainer.onBoundsChange(new Rect(1, 1, 1, 1));
        assertFalse(dr.hasOnBoundsChangedCalled());
        assertEquals(new Rect(1, 1, 1, 1), dr.getBounds());

        try {
            mMockDrawableContainer.onBoundsChange(null);
            fail("Should throw NullPointerException if the bounds is null.");
        } catch (NullPointerException e) {
        }
    }

    public void testIsStateful() {
        assertConstantStateNotSet();

        try {
            mDrawableContainer.isStateful();
            fail("Should throw NullPointerException if the constant state is not set.");
        } catch (NullPointerException e) {
        }

        mMockDrawableContainer.setConstantState(mDrawableContainerState);
        MockDrawable dr0 = new MockDrawable();
        dr0.setStateful(true);
        mDrawableContainerState.addChild(dr0);
        MockDrawable dr1 = new MockDrawable();
        dr1.setStateful(false);
        mDrawableContainerState.addChild(dr1);

        // return result of constant state's isStateful
        assertEquals(mDrawableContainerState.isStateful(), mDrawableContainer.isStateful());
        assertEquals(true, mDrawableContainer.isStateful());

        mDrawableContainer.selectDrawable(1);
        assertEquals(mDrawableContainerState.isStateful(), mDrawableContainer.isStateful());
        assertEquals(true, mDrawableContainer.isStateful());
    }

    public void testOnStateChange() {
        assertConstantStateNotSet();
        assertNull(mDrawableContainer.getCurrent());

        assertFalse(mMockDrawableContainer.onStateChange(new int[] { 0 }));
        assertFalse(mMockDrawableContainer.onStateChange(null));

        mMockDrawableContainer.setConstantState(mDrawableContainerState);
        MockDrawable dr = new MockDrawable();
        dr.setState(new int[] { 0 });
        addAndSelectDrawable(dr);

        // set current drawable's state.
        dr.reset();
        assertNotNull(dr.getState());
        mMockDrawableContainer.onStateChange(null);
        assertTrue(dr.hasOnStateChangedCalled());
        assertNull(dr.getState());

        dr.reset();
        mMockDrawableContainer.onStateChange(new int[] { 0 });
        assertTrue(dr.hasOnStateChangedCalled());
        assertTrue(Arrays.equals(new int[] { 0 }, dr.getState()));

        dr.reset();
        assertFalse(mMockDrawableContainer.onStateChange(new int[] { 0 }));
        assertFalse(dr.hasOnStateChangedCalled());
        assertTrue(Arrays.equals(new int[] { 0 }, dr.getState()));
    }

    public void testOnLevelChange() {
        assertConstantStateNotSet();
        assertNull(mDrawableContainer.getCurrent());

        assertFalse(mMockDrawableContainer.onLevelChange(Integer.MAX_VALUE));
        assertFalse(mMockDrawableContainer.onLevelChange(Integer.MIN_VALUE));

        mMockDrawableContainer.setConstantState(mDrawableContainerState);
        MockDrawable dr = new MockDrawable();
        dr.setLevel(0);
        addAndSelectDrawable(dr);

        // set current drawable's level.
        dr.reset();
        assertEquals(0, dr.getLevel());
        mMockDrawableContainer.onLevelChange(Integer.MAX_VALUE);
        assertEquals(Integer.MAX_VALUE, dr.getLevel());
        assertTrue(dr.hasOnLevelChangedCalled());

        dr.reset();
        assertEquals(Integer.MAX_VALUE, dr.getLevel());
        mMockDrawableContainer.onLevelChange(Integer.MIN_VALUE);
        assertEquals(Integer.MIN_VALUE, dr.getLevel());
        assertTrue(dr.hasOnLevelChangedCalled());

        dr.reset();
        assertEquals(Integer.MIN_VALUE, dr.getLevel());
        assertFalse(mMockDrawableContainer.onLevelChange(Integer.MIN_VALUE));
        assertEquals(Integer.MIN_VALUE, dr.getLevel());
        assertFalse(dr.hasOnLevelChangedCalled());
    }

    public void testGetIntrinsicWidth() {
        assertConstantStateNotSet();

        try {
            mDrawableContainer.getIntrinsicWidth();
            fail("Should throw NullPointerException if the constant state is not set.");
        } catch (NullPointerException e) {
        }

        mMockDrawableContainer.setConstantState(mDrawableContainerState);
        MockDrawable dr0 = new MockDrawable();
        dr0.setIntrinsicWidth(1);
        mDrawableContainerState.addChild(dr0);
        MockDrawable dr1 = new MockDrawable();
        dr1.setIntrinsicWidth(2);
        mDrawableContainerState.addChild(dr1);

        // return result of constant state's getConstantWidth
        mDrawableContainerState.setConstantSize(true);
        assertEquals(mDrawableContainerState.getConstantWidth(),
                mDrawableContainer.getIntrinsicWidth());
        assertEquals(2, mDrawableContainer.getIntrinsicWidth());

        // return default value
        mDrawableContainerState.setConstantSize(false);
        assertNull(mDrawableContainer.getCurrent());
        assertEquals(-1, mDrawableContainer.getIntrinsicWidth());

        // return current drawable's getIntrinsicWidth
        mDrawableContainer.selectDrawable(0);
        assertSame(dr0, mDrawableContainer.getCurrent());
        assertEquals(1, mDrawableContainer.getIntrinsicWidth());
    }

    public void testGetIntrinsicHeight() {
        assertConstantStateNotSet();

        try {
            mDrawableContainer.getIntrinsicHeight();
            fail("Should throw NullPointerException if the constant state is not set.");
        } catch (NullPointerException e) {
        }

        mMockDrawableContainer.setConstantState(mDrawableContainerState);
        MockDrawable dr0 = new MockDrawable();
        dr0.setIntrinsicHeight(1);
        mDrawableContainerState.addChild(dr0);
        MockDrawable dr1 = new MockDrawable();
        dr1.setIntrinsicHeight(2);
        mDrawableContainerState.addChild(dr1);

        // return result of constant state's getConstantHeight
        mDrawableContainerState.setConstantSize(true);
        assertEquals(mDrawableContainerState.getConstantHeight(),
                mDrawableContainer.getIntrinsicHeight());
        assertEquals(2, mDrawableContainer.getIntrinsicHeight());

        // return default value
        mDrawableContainerState.setConstantSize(false);
        assertNull(mDrawableContainer.getCurrent());
        assertEquals(-1, mDrawableContainer.getIntrinsicHeight());

        // return current drawable's getIntrinsicHeight
        mDrawableContainer.selectDrawable(0);
        assertSame(dr0, mDrawableContainer.getCurrent());
        assertEquals(1, mDrawableContainer.getIntrinsicHeight());
    }

    public void testGetMinimumWidth() {
        assertConstantStateNotSet();

        try {
            mDrawableContainer.getMinimumWidth();
            fail("Should throw NullPointerException if the constant state is not set.");
        } catch (NullPointerException e) {
        }

        mMockDrawableContainer.setConstantState(mDrawableContainerState);
        MockDrawable dr0 = new MockDrawable();
        dr0.setMinimumWidth(1);
        mDrawableContainerState.addChild(dr0);
        MockDrawable dr1 = new MockDrawable();
        dr1.setMinimumWidth(2);
        mDrawableContainerState.addChild(dr1);

        // return result of constant state's getConstantMinimumWidth
        mDrawableContainerState.setConstantSize(true);
        assertEquals(mDrawableContainerState.getConstantMinimumWidth(),
                mDrawableContainer.getMinimumWidth());
        assertEquals(2, mDrawableContainer.getMinimumWidth());

        // return default value
        mDrawableContainerState.setConstantSize(false);
        assertNull(mDrawableContainer.getCurrent());
        assertEquals(0, mDrawableContainer.getMinimumWidth());

        // return current drawable's getMinimumWidth
        mDrawableContainer.selectDrawable(0);
        assertSame(dr0, mDrawableContainer.getCurrent());
        assertEquals(1, mDrawableContainer.getMinimumWidth());
    }

    public void testGetMinimumHeight() {
        assertConstantStateNotSet();

        try {
            mDrawableContainer.getMinimumHeight();
            fail("Should throw NullPointerException if the constant state is not set.");
        } catch (NullPointerException e) {
        }

        mMockDrawableContainer.setConstantState(mDrawableContainerState);
        MockDrawable dr0 = new MockDrawable();
        dr0.setMinimumHeight(1);
        mDrawableContainerState.addChild(dr0);
        MockDrawable dr1 = new MockDrawable();
        dr1.setMinimumHeight(2);
        mDrawableContainerState.addChild(dr1);

        // return result of constant state's getConstantMinimumHeight
        mDrawableContainerState.setConstantSize(true);
        assertEquals(mDrawableContainerState.getConstantMinimumHeight(),
                mDrawableContainer.getMinimumHeight());
        assertEquals(2, mDrawableContainer.getMinimumHeight());

        // return default value
        mDrawableContainerState.setConstantSize(false);
        assertNull(mDrawableContainer.getCurrent());
        assertEquals(0, mDrawableContainer.getMinimumHeight());

        // return current drawable's getMinimumHeight
        mDrawableContainer.selectDrawable(0);
        assertSame(dr0, mDrawableContainer.getCurrent());
        assertEquals(1, mDrawableContainer.getMinimumHeight());
    }

    public void testInvalidateDrawable() {
        // Workaround for CTS coverage not recognizing calls on subclasses.
        DrawableContainer dr = mDrawableContainer;

        assertConstantStateNotSet();
        assertNull(mDrawableContainer.getCurrent());

        mDrawableContainer.setCallback(null);
        dr.invalidateDrawable(mDrawableContainer);
        dr.invalidateDrawable(null);

        MockCallBack callback = new MockCallBack();
        mDrawableContainer.setCallback(callback);

        callback.reset();
        dr.invalidateDrawable(mDrawableContainer);
        assertFalse(callback.hasInvalidateDrawableCalled());

        // the callback method can be called if the drawable passed in and the
        // current drawble are both null
        callback.reset();
        dr.invalidateDrawable(null);
        assertTrue(callback.hasInvalidateDrawableCalled());

        mMockDrawableContainer.setConstantState(mDrawableContainerState);
        MockDrawable mockDrawable = new MockDrawable();
        addAndSelectDrawable(mockDrawable);

        callback.reset();
        dr.invalidateDrawable(mDrawableContainer);
        assertFalse(callback.hasInvalidateDrawableCalled());

        callback.reset();
        dr.invalidateDrawable(null);
        assertFalse(callback.hasInvalidateDrawableCalled());

        // Call the callback method if the drawable is selected.
        callback.reset();
        dr.invalidateDrawable(mockDrawable);
        assertTrue(callback.hasInvalidateDrawableCalled());
    }

    public void testScheduleDrawable() {
        // Workaround for CTS coverage not recognizing calls on subclasses.
        DrawableContainer dr = mDrawableContainer;

        assertConstantStateNotSet();
        assertNull(mDrawableContainer.getCurrent());

        mDrawableContainer.setCallback(null);
        dr.scheduleDrawable(mDrawableContainer, null, 0);
        dr.scheduleDrawable(null, new Runnable() {
                public void run() {
                }
            }, 0);

        MockCallBack callback = new MockCallBack();
        mDrawableContainer.setCallback(callback);

        callback.reset();
        dr.scheduleDrawable(mDrawableContainer, null, 0);
        assertFalse(callback.hasScheduleDrawableCalled());

        // the callback method can be called if the drawable passed in and the
        // current drawble are both null
        callback.reset();
        dr.scheduleDrawable(null, new Runnable() {
                public void run() {
                }
            }, 0);
        assertTrue(callback.hasScheduleDrawableCalled());

        mMockDrawableContainer.setConstantState(mDrawableContainerState);
        MockDrawable mockDrawable = new MockDrawable();
        addAndSelectDrawable(mockDrawable);

        callback.reset();
        dr.scheduleDrawable(mDrawableContainer, null, 0);
        assertFalse(callback.hasScheduleDrawableCalled());

        callback.reset();
        dr.scheduleDrawable(null, new Runnable() {
                public void run() {
                }
            }, 0);
        assertFalse(callback.hasScheduleDrawableCalled());

        // Call the callback method if the drawable is selected.
        callback.reset();
        dr.scheduleDrawable(mockDrawable, null, 0);
        assertTrue(callback.hasScheduleDrawableCalled());
    }

    public void testUnscheduleDrawable() {
        // Workaround for CTS coverage not recognizing calls on subclasses.
        DrawableContainer dr = mDrawableContainer;

        assertConstantStateNotSet();
        assertNull(mDrawableContainer.getCurrent());

        mDrawableContainer.setCallback(null);
        dr.unscheduleDrawable(mDrawableContainer, null);
        dr.unscheduleDrawable(null, new Runnable() {
                public void run() {
                }
            });

        MockCallBack callback = new MockCallBack();
        mDrawableContainer.setCallback(callback);

        callback.reset();
        dr.unscheduleDrawable(mDrawableContainer, null);
        assertFalse(callback.hasUnscheduleDrawableCalled());

        // the callback method can be called if the drawable passed in and the
        // current drawble are both null
        callback.reset();
        dr.unscheduleDrawable(null, new Runnable() {
                public void run() {
                }
            });
        assertTrue(callback.hasUnscheduleDrawableCalled());

        mMockDrawableContainer.setConstantState(mDrawableContainerState);
        MockDrawable mockDrawable = new MockDrawable();
        addAndSelectDrawable(mockDrawable);

        callback.reset();
        dr.unscheduleDrawable(mDrawableContainer, null);
        assertFalse(callback.hasUnscheduleDrawableCalled());

        callback.reset();
        dr.unscheduleDrawable(null, new Runnable() {
                public void run() {
                }
            });
        assertFalse(callback.hasUnscheduleDrawableCalled());

        // Call the callback method if the drawable is selected.
        callback.reset();
        dr.unscheduleDrawable(mockDrawable, null);
        assertTrue(callback.hasUnscheduleDrawableCalled());
    }

    public void testSetVisible() {
        assertConstantStateNotSet();
        assertNull(mDrawableContainer.getCurrent());

        assertTrue(mDrawableContainer.isVisible());
        assertFalse(mDrawableContainer.setVisible(true, false));
        assertTrue(mDrawableContainer.setVisible(false, false));
        assertFalse(mDrawableContainer.setVisible(false, false));
        assertTrue(mDrawableContainer.setVisible(true, false));

        mMockDrawableContainer.setConstantState(mDrawableContainerState);
        MockDrawable dr = new MockDrawable();
        addAndSelectDrawable(dr);

        // set current drawable's visibility
        assertTrue(mDrawableContainer.isVisible());
        assertTrue(dr.isVisible());
        assertTrue(mDrawableContainer.setVisible(false, false));
        assertFalse(mDrawableContainer.isVisible());
        assertFalse(dr.isVisible());
    }

    public void testGetOpacity() {
        // Workaround for CTS coverage not recognizing calls on subclasses.
        DrawableContainer dr = mDrawableContainer;

        assertConstantStateNotSet();

        // there is no child, so the container is transparent
        assertEquals(PixelFormat.TRANSPARENT, dr.getOpacity());

        mMockDrawableContainer.setConstantState(mDrawableContainerState);
        MockDrawable dr0 = new MockDrawable();
        dr0.setOpacity(PixelFormat.OPAQUE);
        mDrawableContainerState.addChild(dr0);
        // no child selected yet
        assertEquals(PixelFormat.TRANSPARENT, dr.getOpacity());

        dr.selectDrawable(0);
        assertEquals(mDrawableContainerState.getOpacity(), dr.getOpacity());
        assertEquals(PixelFormat.OPAQUE, mDrawableContainer.getOpacity());

        MockDrawable dr1 = new MockDrawable();
        dr1.setOpacity(PixelFormat.TRANSLUCENT);
        mDrawableContainerState.addChild(dr1);

        dr.selectDrawable(1);
        assertEquals(mDrawableContainerState.getOpacity(), dr.getOpacity());
        assertEquals(PixelFormat.TRANSLUCENT, dr.getOpacity());
    }

    public void testAccessCurrentDrawable() {
        // Workaround for CTS coverage not recognizing calls on subclasses.
        DrawableContainer dr = mDrawableContainer;

        assertConstantStateNotSet();

        assertNull(mDrawableContainer.getCurrent());
        try {
            dr.selectDrawable(0);
            fail("Should throw NullPointerException if the constant state is not set.");
        } catch (NullPointerException e) {
        }

        mMockDrawableContainer.setConstantState(mDrawableContainerState);
        MockDrawable dr0 = new MockDrawable();
        dr0.setVisible(false, false);
        assertFalse(dr0.isVisible());
        mDrawableContainerState.addChild(dr0);
        MockDrawable dr1 = new MockDrawable();
        dr1.setVisible(false, false);
        assertFalse(dr1.isVisible());
        mDrawableContainerState.addChild(dr1);

        assertTrue(dr.selectDrawable(0));
        assertSame(dr0, mDrawableContainer.getCurrent());
        assertTrue(dr0.isVisible());

        assertFalse(dr.selectDrawable(0));

        assertTrue(dr.selectDrawable(1));
        assertSame(dr1, mDrawableContainer.getCurrent());
        assertTrue(dr1.isVisible());
        assertFalse(dr0.isVisible());

        assertFalse(dr.selectDrawable(1));

        assertTrue(dr.selectDrawable(-1));
        assertNull(mDrawableContainer.getCurrent());
        assertFalse(dr0.isVisible());
        assertFalse(dr1.isVisible());

        assertTrue(dr.selectDrawable(2));
        assertNull(mDrawableContainer.getCurrent());
        assertFalse(dr0.isVisible());
        assertFalse(dr1.isVisible());
    }

    public void testAccessConstantState() {
        try {
            mDrawableContainer.getConstantState();
            fail("Should throw NullPointerException if the constant state is not set.");
        } catch (NullPointerException e) {
        }

        mMockDrawableContainer.setConstantState(mDrawableContainerState);
        assertSame(mDrawableContainerState, mDrawableContainer.getConstantState());

        mMockDrawableContainer.setConstantState(null);
        assertConstantStateNotSet();
    }

    public void testMutate() {
        assertConstantStateNotSet();
        try {
            mDrawableContainer.mutate();
            fail("Should throw NullPointerException.");
        } catch (NullPointerException e) {
        }

        mMockDrawableContainer.setConstantState(mDrawableContainerState);
        MockDrawable dr0 = new MockDrawable();
        mDrawableContainerState.addChild(dr0);
        mDrawableContainer.mutate();
        assertTrue(dr0.hasMutateCalled());
    }

    private void addAndSelectDrawable(MockDrawable mockDrawable) {
        // Workaround for CTS coverage not recognizing calls on subclasses.
        DrawableContainer dr = mDrawableContainer;

        int pos = mDrawableContainerState.addChild(mockDrawable);
        dr.selectDrawable(pos);
        assertSame(mockDrawable, dr.getCurrent());
    }

    private void assertConstantStateNotSet() {
        try {
            mDrawableContainer.getConstantState();
            fail("Should throw NullPointerException.");
        } catch (NullPointerException e) {
        }
    }

    private class MockDrawableContainer extends DrawableContainer {
        @Override
        protected void onBoundsChange(Rect bounds) {
            super.onBoundsChange(bounds);
        }

        @Override
        protected boolean onLevelChange(int level) {
            return super.onLevelChange(level);
        }

        @Override
        protected boolean onStateChange(int[] state) {
            return super.onStateChange(state);
        }

        @Override
        protected void setConstantState(DrawableContainerState state) {
            super.setConstantState(state);
        }
    }

    private class MockDrawable extends Drawable {
        private boolean mHasCalledDraw;
        private boolean mHasCalledSetAlpha;
        private boolean mHasCalledSetColorFilter;
        private boolean mHasCalledSetDither;
        private boolean mHasCalledSetTint;
        private boolean mHasCalledOnBoundsChanged;
        private boolean mHasCalledOnStateChanged;
        private boolean mHasCalledOnLevelChanged;
        private boolean mHasCalledMutate;

        private boolean mIsStateful;

        private Rect mPadding;

        private int mIntrinsicHeight;
        private int mIntrinsicWidth;

        private int mMinimumHeight;
        private int mMinimumWidth;

        private int mOpacity;

        private Mode mTintMode;

        @Override
        public int getOpacity() {
            return mOpacity;
        }

        @Override
        public boolean isStateful() {
            return mIsStateful;
        }

        public void setStateful(boolean isStateful) {
            mIsStateful = isStateful;
        }

        public Mode getTintMode() {
            return mTintMode;
        }

        public void setPadding(Rect rect) {
            if (mPadding == null) {
                mPadding = new Rect();
            }
            mPadding.set(rect);
        }

        @Override
        public boolean getPadding(Rect padding) {
            if (padding == null || mPadding == null) {
                return false;
            }
            padding.set(mPadding);
            return true;
        }

        @Override
        public int getMinimumHeight() {
            return mMinimumHeight;
        }

        @Override
        public int getMinimumWidth() {
            return mMinimumWidth;
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
        public Drawable mutate() {
            mHasCalledMutate = true;
            return this;
        }

        @Override
        public void setTintMode(Mode tintMode) {
            mTintMode = tintMode;
            mHasCalledSetTint = true;
        }

        public void setMinimumHeight(int h) {
            mMinimumHeight = h;
        }

        public void setMinimumWidth(int w) {
            mMinimumWidth = w;
        }

        public void setIntrinsicHeight(int h) {
            mIntrinsicHeight = h;
        }

        public void setIntrinsicWidth(int w) {
            mIntrinsicWidth = w;
        }

        public void setOpacity(int opacity) {
            mOpacity = opacity;
        }

        public boolean hasDrawCalled() {
            return mHasCalledDraw;
        }

        public boolean hasSetAlphaCalled() {
            return mHasCalledSetAlpha;
        }

        public boolean hasSetColorFilterCalled() {
            return mHasCalledSetColorFilter;
        }

        public boolean hasSetDitherCalled() {
            return mHasCalledSetDither;
        }

        public boolean hasSetTintCalled() {
            return mHasCalledSetTint;
        }

        public boolean hasOnBoundsChangedCalled() {
            return mHasCalledOnBoundsChanged;
        }

        public boolean hasOnStateChangedCalled() {
            return mHasCalledOnStateChanged;
        }

        public boolean hasOnLevelChangedCalled() {
            return mHasCalledOnLevelChanged;
        }

        public boolean hasMutateCalled() {
            return mHasCalledMutate;
        }

        public void reset() {
            mHasCalledOnLevelChanged = false;
            mHasCalledOnStateChanged = false;
            mHasCalledOnBoundsChanged = false;
            mHasCalledSetDither = false;
            mHasCalledSetColorFilter = false;
            mHasCalledSetAlpha = false;
            mHasCalledDraw = false;
            mHasCalledMutate = false;
        }

        @Override
        public void draw(Canvas canvas) {
            mHasCalledDraw = true;
        }

        @Override
        public void setAlpha(int alpha) {
            mHasCalledSetAlpha = true;
        }

        @Override
        public void setColorFilter(ColorFilter cf) {
            mHasCalledSetColorFilter = true;
        }

        @Override
        protected void onBoundsChange(Rect bounds) {
            super.onBoundsChange(bounds);
            mHasCalledOnBoundsChanged = true;
        }

        @Override
        protected boolean onLevelChange(int level) {
            boolean result = super.onLevelChange(level);
            mHasCalledOnLevelChanged = true;
            return result;
        }

        @Override
        protected boolean onStateChange(int[] state) {
            boolean result = super.onStateChange(state);
            mHasCalledOnStateChanged = true;
            return result;

        }

        @Override
        public void setDither(boolean dither) {
            super.setDither(dither);
            mHasCalledSetDither = true;
        }
    }

    private class MockCallBack implements Drawable.Callback {
        private boolean mCalledInvalidateDrawable;

        private boolean mCalledScheduleDrawable;

        private boolean mCalledUnscheduleDrawable;

        public boolean hasInvalidateDrawableCalled() {
            return mCalledInvalidateDrawable;
        }

        public boolean hasScheduleDrawableCalled() {
            return mCalledScheduleDrawable;
        }

        public boolean hasUnscheduleDrawableCalled() {
            return mCalledUnscheduleDrawable;
        }

        public void reset() {
            mCalledUnscheduleDrawable = false;
            mCalledScheduleDrawable = false;
            mCalledInvalidateDrawable = false;
        }

        public void invalidateDrawable(Drawable who) {
            mCalledInvalidateDrawable = true;
        }

        public void scheduleDrawable(Drawable who, Runnable what, long when) {
            mCalledScheduleDrawable = true;
        }

        public void unscheduleDrawable(Drawable who, Runnable what) {
            mCalledUnscheduleDrawable = true;
        }

        public int getResolvedLayoutDirection(Drawable who) {
            return 0;
        }
    }
}
