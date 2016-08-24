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

import java.io.IOException;

import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserException;

import android.R.attr;
import android.content.res.Configuration;
import android.content.res.Resources;
import android.content.res.XmlResourceParser;
import android.graphics.Canvas;
import android.graphics.ColorFilter;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.Drawable.ConstantState;
import android.graphics.drawable.StateListDrawable;
import android.graphics.drawable.DrawableContainer.DrawableContainerState;
import android.test.InstrumentationTestCase;
import android.util.DisplayMetrics;
import android.util.StateSet;
import android.util.Xml;

import android.graphics.cts.R;


public class StateListDrawableTest extends InstrumentationTestCase {
    private MockStateListDrawable mMockDrawable;
    private StateListDrawable mDrawable;

    private Resources mResources;

    private DrawableContainerState mDrawableContainerState;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mDrawable = mMockDrawable = new MockStateListDrawable();
        mDrawableContainerState = (DrawableContainerState) mMockDrawable.getConstantState();
        mResources = getInstrumentation().getTargetContext().getResources();
    }

    public void testStateListDrawable() {
        new StateListDrawable();
        // Check the values set in the constructor
        assertNotNull(new StateListDrawable().getConstantState());
        assertTrue(new MockStateListDrawable().hasCalledOnStateChanged());
    }

    public void testAddState() {
        // Workaround for CTS coverage not recognizing calls on subclasses.
        StateListDrawable dr = mMockDrawable;

        assertEquals(0, mDrawableContainerState.getChildCount());

        // nothing happens if drawable is null
        mMockDrawable.reset();
        dr.addState(StateSet.WILD_CARD, null);
        assertEquals(0, mDrawableContainerState.getChildCount());
        assertFalse(mMockDrawable.hasCalledOnStateChanged());

        // call onLevelChanged to assure that the correct drawable is selected.
        mMockDrawable.reset();
        dr.addState(StateSet.WILD_CARD, new MockDrawable());
        assertEquals(1, mDrawableContainerState.getChildCount());
        assertTrue(mMockDrawable.hasCalledOnStateChanged());

        mMockDrawable.reset();
        dr.addState(new int[] { attr.state_focused, - attr.state_selected }, new MockDrawable());
        assertEquals(2, mDrawableContainerState.getChildCount());
        assertTrue(mMockDrawable.hasCalledOnStateChanged());

        // call onLevelChanged will not throw NPE here because the first drawable with wild card
        // state is matched first. There is no chance that other drawables will be matched.
        mMockDrawable.reset();
        dr.addState(null, new MockDrawable());
        assertEquals(3, mDrawableContainerState.getChildCount());
        assertTrue(mMockDrawable.hasCalledOnStateChanged());
    }

    public void testIsStateful() {
        assertTrue(new StateListDrawable().isStateful());
    }

    public void testOnStateChange() {
        mMockDrawable.addState(new int[] { attr.state_focused, - attr.state_selected },
                new MockDrawable());
        mMockDrawable.addState(StateSet.WILD_CARD, new MockDrawable());
        mMockDrawable.addState(StateSet.WILD_CARD, new MockDrawable());

        // the method is not called if same state is set
        mMockDrawable.reset();
        mMockDrawable.setState(mMockDrawable.getState());
        assertFalse(mMockDrawable.hasCalledOnStateChanged());

        // the method is called if different state is set
        mMockDrawable.reset();
        mMockDrawable.setState(new int[] { attr.state_focused, - attr.state_selected });
        assertTrue(mMockDrawable.hasCalledOnStateChanged());

        mMockDrawable.reset();
        mMockDrawable.setState(null);
        assertTrue(mMockDrawable.hasCalledOnStateChanged());

        // check that correct drawable is selected.
        mMockDrawable.onStateChange(new int[] { attr.state_focused, - attr.state_selected });
        assertSame(mMockDrawable.getCurrent(), mDrawableContainerState.getChildren()[0]);

        assertFalse(mMockDrawable.onStateChange(new int[] { attr.state_focused }));
        assertSame(mMockDrawable.getCurrent(), mDrawableContainerState.getChildren()[0]);

        assertTrue(mMockDrawable.onStateChange(StateSet.WILD_CARD));
        assertSame(mMockDrawable.getCurrent(), mDrawableContainerState.getChildren()[1]);

        // null state will match the wild card
        assertFalse(mMockDrawable.onStateChange(null));
        assertSame(mMockDrawable.getCurrent(), mDrawableContainerState.getChildren()[1]);
    }

    public void testOnStateChangeWithWildCardAtFirst() {
        mMockDrawable.addState(StateSet.WILD_CARD, new MockDrawable());
        mMockDrawable.addState(new int[] { attr.state_focused, - attr.state_selected },
                new MockDrawable());

        // matches the first wild card although the second one is more accurate
        mMockDrawable.onStateChange(new int[] { attr.state_focused, - attr.state_selected });
        assertSame(mMockDrawable.getCurrent(), mDrawableContainerState.getChildren()[0]);
    }

    public void testOnStateChangeWithNullStateSet() {
        assertEquals(0, mDrawableContainerState.getChildCount());
        try {
            mMockDrawable.addState(null, new MockDrawable());
            fail("Should throw NullPointerException.");
        } catch (NullPointerException e) {
        }
        assertEquals(1, mDrawableContainerState.getChildCount());

        try {
            mMockDrawable.onStateChange(StateSet.WILD_CARD);
            fail("Should throw NullPointerException.");
        } catch (NullPointerException e) {
        }
    }

    public void testPreloadDensity() throws XmlPullParserException, IOException {
        runPreloadDensityTestForDrawable(R.drawable.state_list_density, false);
    }

    public void testPreloadDensityConstantSize() throws XmlPullParserException, IOException {
        runPreloadDensityTestForDrawable(R.drawable.state_list_density_constant_size, true);
    }

    private void runPreloadDensityTestForDrawable(int drawableResId, boolean isConstantSize)
            throws XmlPullParserException, IOException {
        final Resources res = mResources;
        final int densityDpi = res.getConfiguration().densityDpi;
        try {
            runPreloadDensityTestForDrawableInner(res, densityDpi, drawableResId, isConstantSize);
        } finally {
            DrawableTestUtils.setResourcesDensity(res, densityDpi);
        }
    }

    private void runPreloadDensityTestForDrawableInner(Resources res, int densityDpi,
            int drawableResId, boolean isConstantSize) throws XmlPullParserException, IOException {
        // Capture initial state at default density.
        final XmlResourceParser parser = getResourceParser(drawableResId);
        final StateListDrawable preloadedDrawable = new StateListDrawable();
        preloadedDrawable.inflate(res, parser, Xml.asAttributeSet(parser));
        final ConstantState preloadedConstantState = preloadedDrawable.getConstantState();
        preloadedDrawable.selectDrawable(0);
        final int tempWidth0 = preloadedDrawable.getIntrinsicWidth();
        preloadedDrawable.selectDrawable(1);
        final int tempWidth1 = preloadedDrawable.getIntrinsicWidth();

        // Pick comparison widths based on constant size.
        final int origWidth0;
        final int origWidth1;
        if (isConstantSize) {
            origWidth0 = Math.max(tempWidth0, tempWidth1);
            origWidth1 = origWidth0;
        } else {
            origWidth0 = tempWidth0;
            origWidth1 = tempWidth1;
        }

        // Set density to half of original.
        DrawableTestUtils.setResourcesDensity(res, densityDpi / 2);
        final StateListDrawable halfDrawable =
                (StateListDrawable) preloadedConstantState.newDrawable(res);
        halfDrawable.selectDrawable(0);
        assertEquals(Math.round(origWidth0 / 2f), halfDrawable.getIntrinsicWidth());
        halfDrawable.selectDrawable(1);
        assertEquals(Math.round(origWidth1 / 2f), halfDrawable.getIntrinsicWidth());

        // Set density to double original.
        DrawableTestUtils.setResourcesDensity(res, densityDpi * 2);
        final StateListDrawable doubleDrawable =
                (StateListDrawable) preloadedConstantState.newDrawable(res);
        doubleDrawable.selectDrawable(0);
        assertEquals(origWidth0 * 2, doubleDrawable.getIntrinsicWidth());
        doubleDrawable.selectDrawable(1);
        assertEquals(origWidth1 * 2, doubleDrawable.getIntrinsicWidth());

        // Restore original configuration and metrics.
        DrawableTestUtils.setResourcesDensity(res, densityDpi);
        final StateListDrawable origDrawable =
                (StateListDrawable) preloadedConstantState.newDrawable(res);
        origDrawable.selectDrawable(0);
        assertEquals(origWidth0, origDrawable.getIntrinsicWidth());
        origDrawable.selectDrawable(1);
        assertEquals(origWidth1, origDrawable.getIntrinsicWidth());
    }

    public void testInflate() throws XmlPullParserException, IOException {
        XmlResourceParser parser = getResourceParser(R.xml.selector_correct);

        mMockDrawable.reset();
        mMockDrawable.inflate(mResources, parser, Xml.asAttributeSet(parser));
        // android:visible="false"
        assertFalse(mMockDrawable.isVisible());
        // android:constantSize="true"
        assertTrue(mDrawableContainerState.isConstantSize());
        // android:variablePadding="true"
        assertNull(mDrawableContainerState.getConstantPadding());
        assertTrue(mMockDrawable.hasCalledOnStateChanged());
        assertEquals(2, mDrawableContainerState.getChildCount());
        // check the android:state_* by calling setState
        mMockDrawable.setState(new int[]{ attr.state_focused, - attr.state_pressed });
        assertSame(mMockDrawable.getCurrent(), mDrawableContainerState.getChildren()[0]);
        mMockDrawable.setState(StateSet.WILD_CARD);
        assertSame(mMockDrawable.getCurrent(), mDrawableContainerState.getChildren()[1]);

        mMockDrawable = new MockStateListDrawable();
        mDrawableContainerState = (DrawableContainerState) mMockDrawable.getConstantState();
        assertNull(mMockDrawable.getCurrent());
        mMockDrawable.reset();
        assertTrue(mMockDrawable.isVisible());
        parser = getResourceParser(R.xml.selector_missing_selector_attrs);
        mMockDrawable.inflate(mResources, parser, Xml.asAttributeSet(parser));
        // use current the visibility
        assertTrue(mMockDrawable.isVisible());
        // default value of android:constantSize is false
        assertFalse(mDrawableContainerState.isConstantSize());
        // default value of android:variablePadding is false
        // TODO: behavior of mDrawableContainerState.getConstantPadding() when variablePadding is
        // false is undefined
        //assertNotNull(mDrawableContainerState.getConstantPadding());
        assertTrue(mMockDrawable.hasCalledOnStateChanged());
        assertEquals(1, mDrawableContainerState.getChildCount());
        mMockDrawable.setState(new int[]{ - attr.state_pressed, attr.state_focused });
        assertSame(mMockDrawable.getCurrent(), mDrawableContainerState.getChildren()[0]);
        mMockDrawable.setState(StateSet.WILD_CARD);
        assertNull(mMockDrawable.getCurrent());

        parser = getResourceParser(R.xml.selector_missing_item_drawable);
        try {
            mMockDrawable.inflate(mResources, parser, Xml.asAttributeSet(parser));
            fail("Should throw XmlPullParserException if drawable of item is missing");
        } catch (XmlPullParserException e) {
        }
    }

    public void testInflateWithNullParameters() throws XmlPullParserException, IOException{
        XmlResourceParser parser = getResourceParser(R.xml.level_list_correct);
        try {
            mMockDrawable.inflate(null, parser, Xml.asAttributeSet(parser));
            fail("Should throw XmlPullParserException if resource is null");
        } catch (NullPointerException e) {
        }

        try {
            mMockDrawable.inflate(mResources, null, Xml.asAttributeSet(parser));
            fail("Should throw XmlPullParserException if parser is null");
        } catch (NullPointerException e) {
        }

        try {
            mMockDrawable.inflate(mResources, parser, null);
            fail("Should throw XmlPullParserException if AttributeSet is null");
        } catch (NullPointerException e) {
        }
    }

    public void testMutate() {
        StateListDrawable d1 =
            (StateListDrawable) mResources.getDrawable(R.drawable.statelistdrawable);
        StateListDrawable d2 =
            (StateListDrawable) mResources.getDrawable(R.drawable.statelistdrawable);
        StateListDrawable d3 =
            (StateListDrawable) mResources.getDrawable(R.drawable.statelistdrawable);

        // StateListDrawable mutates its children when jumping to a new drawable
        d1.getCurrent().setAlpha(100);
        assertEquals(100, ((BitmapDrawable) d1.getCurrent()).getPaint().getAlpha());
        assertEquals(255, ((BitmapDrawable) d2.getCurrent()).getPaint().getAlpha());
        assertEquals(255, ((BitmapDrawable) d3.getCurrent()).getPaint().getAlpha());

        d1.mutate();

        // TODO: add verification

    }

    private XmlResourceParser getResourceParser(int resId) throws XmlPullParserException,
            IOException {
        XmlResourceParser parser = getInstrumentation().getTargetContext().getResources().getXml(
                resId);
        int type;
        while ((type = parser.next()) != XmlPullParser.START_TAG
                && type != XmlPullParser.END_DOCUMENT) {
            // Empty loop
        }
        return parser;
    }

    private class MockStateListDrawable extends StateListDrawable {
        private boolean mHasCalledOnStateChanged;

        public boolean hasCalledOnStateChanged() {
            return mHasCalledOnStateChanged;
        }

        public void reset() {
            mHasCalledOnStateChanged = false;
        }

        @Override
        protected boolean onStateChange(int[] stateSet) {
            boolean result = super.onStateChange(stateSet);
            mHasCalledOnStateChanged = true;
            return result;
        }
    }

    private class MockDrawable extends Drawable {
        @Override
        public void draw(Canvas canvas) {
        }

        @Override
        public int getOpacity() {
            return 0;
        }

        @Override
        public void setAlpha(int alpha) {
        }

        @Override
        public void setColorFilter(ColorFilter cf) {
        }
    }
}
