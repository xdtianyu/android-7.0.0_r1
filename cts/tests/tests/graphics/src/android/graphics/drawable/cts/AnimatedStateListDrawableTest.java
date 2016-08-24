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

package android.graphics.drawable.cts;

import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserException;

import android.R.attr;
import android.content.Context;
import android.content.res.Resources;
import android.content.res.XmlResourceParser;
import android.graphics.Canvas;
import android.graphics.ColorFilter;
import android.graphics.cts.R;
import android.graphics.drawable.Animatable;
import android.graphics.drawable.Animatable2;
import android.graphics.drawable.AnimatedStateListDrawable;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.DrawableContainer.DrawableContainerState;
import android.graphics.drawable.StateListDrawable;
import android.test.InstrumentationTestCase;
import android.util.StateSet;
import android.util.Xml;

import java.io.IOException;
import java.util.Arrays;
import java.util.HashSet;

import static org.mockito.Mockito.mock;

public class AnimatedStateListDrawableTest extends InstrumentationTestCase {
    private static final int[] STATE_EMPTY = new int[] { };
    private static final int[] STATE_FOCUSED = new int[] { attr.state_focused };

    private Context mContext;
    private Resources mResources;

    @Override
    protected void setUp() throws Exception {
        super.setUp();

        mContext = getInstrumentation().getTargetContext();
        mResources = mContext.getResources();
    }

    public void testStateListDrawable() {
        new AnimatedStateListDrawable();

        // Check the values set in the constructor
        assertNotNull(new AnimatedStateListDrawable().getConstantState());
    }

    public void testAddState() {
        AnimatedStateListDrawable asld = new AnimatedStateListDrawable();
        DrawableContainerState cs = (DrawableContainerState) asld.getConstantState();
        assertEquals(0, cs.getChildCount());

        try {
            asld.addState(StateSet.WILD_CARD, null, R.id.focused);
            fail("Expected IllegalArgumentException");
        } catch (IllegalArgumentException e) {
            // Expected.
        }

        Drawable unfocused = mock(Drawable.class);
        asld.addState(StateSet.WILD_CARD, unfocused, R.id.focused);
        assertEquals(1, cs.getChildCount());

        Drawable focused = mock(Drawable.class);
        asld.addState(STATE_FOCUSED, focused, R.id.unfocused);
        assertEquals(2, cs.getChildCount());
    }

    public void testAddTransition() {
        AnimatedStateListDrawable asld = new AnimatedStateListDrawable();
        DrawableContainerState cs = (DrawableContainerState) asld.getConstantState();

        Drawable focused = mock(Drawable.class);
        Drawable unfocused = mock(Drawable.class);
        asld.addState(STATE_FOCUSED, focused, R.id.focused);
        asld.addState(StateSet.WILD_CARD, unfocused, R.id.unfocused);

        try {
            asld.addTransition(R.id.focused, R.id.focused, null, false);
            fail("Expected IllegalArgumentException");
        } catch (IllegalArgumentException e) {
            // Expected.
        }

        MockTransition focusedToUnfocused = mock(MockTransition.class);
        asld.addTransition(R.id.focused, R.id.unfocused, focusedToUnfocused, false);
        assertEquals(3, cs.getChildCount());

        MockTransition unfocusedToFocused = mock(MockTransition.class);
        asld.addTransition(R.id.unfocused, R.id.focused, unfocusedToFocused, false);
        assertEquals(4, cs.getChildCount());

        MockTransition reversible = mock(MockTransition.class);
        asld.addTransition(R.id.focused, R.id.unfocused, reversible, true);
        assertEquals(5, cs.getChildCount());
    }

    public void testIsStateful() {
        assertTrue(new AnimatedStateListDrawable().isStateful());
    }

    public void testOnStateChange() {
        AnimatedStateListDrawable asld = new AnimatedStateListDrawable();

        Drawable focused = mock(Drawable.class);
        Drawable unfocused = mock(Drawable.class);
        asld.addState(STATE_FOCUSED, focused, R.id.focused);
        asld.addState(StateSet.WILD_CARD, unfocused, R.id.unfocused);

        MockTransition focusedToUnfocused = mock(MockTransition.class);
        MockTransition unfocusedToFocused = mock(MockTransition.class);
        asld.addTransition(R.id.focused, R.id.unfocused, focusedToUnfocused, false);
        asld.addTransition(R.id.unfocused, R.id.focused, unfocusedToFocused, false);

        asld.setState(STATE_EMPTY);
        assertSame(unfocused, asld.getCurrent());

        asld.setState(STATE_FOCUSED);
        assertSame(unfocusedToFocused, asld.getCurrent());

        asld.setState(STATE_FOCUSED);
        assertSame(unfocusedToFocused, asld.getCurrent());
    }

    public void testPreloadDensity() throws XmlPullParserException, IOException {
        runPreloadDensityTestForDrawable(
                R.drawable.animated_state_list_density, false);
    }

    public void testPreloadDensityConstantSize() throws XmlPullParserException, IOException {
        runPreloadDensityTestForDrawable(
                R.drawable.animated_state_list_density_constant_size, true);
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
        final AnimatedStateListDrawable asld = new AnimatedStateListDrawable();
        asld.inflate(res, parser, Xml.asAttributeSet(parser));

        final DrawableContainerState cs = (DrawableContainerState) asld.getConstantState();
        final int count = cs.getChildCount();
        final int[] origWidth = new int[count];
        int max = 0;
        for (int i = 0; i < count; i++) {
            final int width = cs.getChild(i).getIntrinsicWidth();
            max = Math.max(max, width);
            origWidth[i] = width;
        }
        if (isConstantSize) {
            Arrays.fill(origWidth, max);
        }

        // Set density to half of original.
        DrawableTestUtils.setResourcesDensity(res, densityDpi / 2);
        final StateListDrawable halfDrawable =
                (StateListDrawable) cs.newDrawable(res);
        for (int i = 0; i < count; i++) {
            halfDrawable.selectDrawable(i);
            assertEquals(Math.round(origWidth[i] / 2f), halfDrawable.getIntrinsicWidth());
        }

        // Set density to double original.
        DrawableTestUtils.setResourcesDensity(res, densityDpi * 2);
        final StateListDrawable doubleDrawable =
                (StateListDrawable) cs.newDrawable(res);
        for (int i = 0; i < count; i++) {
            doubleDrawable.selectDrawable(i);
            assertEquals(origWidth[i] * 2, doubleDrawable.getIntrinsicWidth());
        }

        // Restore original configuration and metrics.
        DrawableTestUtils.setResourcesDensity(res, densityDpi);
        final StateListDrawable origDrawable =
                (StateListDrawable) cs.newDrawable(res);
        for (int i = 0; i < count; i++) {
            origDrawable.selectDrawable(i);
            assertEquals(origWidth[i], origDrawable.getIntrinsicWidth());
        }
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

    public void testInflate() throws XmlPullParserException, IOException {
        AnimatedStateListDrawable asld = (AnimatedStateListDrawable) mContext.getDrawable(
                R.drawable.animated_state_list_density);
        DrawableContainerState asldState = (DrawableContainerState) asld.getConstantState();
        assertTrue(asld.isVisible());
        assertFalse(asldState.isConstantSize());
        assertNull(asldState.getConstantPadding());
        assertEquals(4, asldState.getChildCount());
    }

    public abstract class MockTransition extends MockDrawable implements Animatable, Animatable2 {
        private HashSet<AnimationCallback> mCallbacks = new HashSet<>();

        @Override
        public void registerAnimationCallback(AnimationCallback callback) {
            mCallbacks.add(callback);
        }

        @Override
        public boolean unregisterAnimationCallback(AnimationCallback callback) {
            return mCallbacks.remove(callback);
        }

        @Override
        public void clearAnimationCallbacks() {
            mCallbacks.clear();
        }
    }

    public class MockDrawable extends Drawable {
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
