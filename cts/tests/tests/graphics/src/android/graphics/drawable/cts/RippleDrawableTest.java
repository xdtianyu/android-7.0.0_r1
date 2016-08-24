/*
 * Copyright (C) 2015 The Android Open Source Project
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

import static org.mockito.Mockito.*;

import org.xmlpull.v1.XmlPullParserException;

import android.content.res.ColorStateList;
import android.content.res.Resources;
import android.content.res.Resources.Theme;
import android.content.res.XmlResourceParser;
import android.graphics.Color;
import android.graphics.cts.R;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.Drawable.ConstantState;
import android.graphics.drawable.RippleDrawable;
import android.test.AndroidTestCase;
import android.util.Xml;

import java.io.IOException;

public class RippleDrawableTest extends AndroidTestCase {
    public void testConstructor() {
        new RippleDrawable(ColorStateList.valueOf(Color.RED), null, null);
    }

    public void testAccessRadius() {
        RippleDrawable drawable =
            new RippleDrawable(ColorStateList.valueOf(Color.RED), null, null);
        assertEquals(RippleDrawable.RADIUS_AUTO, drawable.getRadius());
        drawable.setRadius(10);
        assertEquals(10, drawable.getRadius());
    }

    public void testRadiusAttr() {
        RippleDrawable drawable =
                (RippleDrawable) getContext().getDrawable(R.drawable.rippledrawable_radius);
        assertEquals(10, drawable.getRadius());
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
                res, R.drawable.rippledrawable_radius);
        final RippleDrawable preloadedDrawable = new RippleDrawable(
                ColorStateList.valueOf(Color.BLACK), null, null);
        preloadedDrawable.inflate(res, parser, Xml.asAttributeSet(parser));
        final ConstantState preloadedConstantState = preloadedDrawable.getConstantState();
        final int initialRadius = preloadedDrawable.getRadius();

        // Set density to half of original. Unlike offsets, which are
        // truncated, dimensions are rounded to the nearest pixel.
        DrawableTestUtils.setResourcesDensity(res, densityDpi / 2);
        final RippleDrawable halfDrawable =
                (RippleDrawable) preloadedConstantState.newDrawable(res);
        assertEquals(Math.round(initialRadius / 2f), halfDrawable.getRadius());

        // Set density to double original.
        DrawableTestUtils.setResourcesDensity(res, densityDpi * 2);
        final RippleDrawable doubleDrawable =
                (RippleDrawable) preloadedConstantState.newDrawable(res);
        assertEquals(initialRadius * 2, doubleDrawable.getRadius());

        // Restore original density.
        DrawableTestUtils.setResourcesDensity(res, densityDpi);
        final RippleDrawable origDrawable =
                (RippleDrawable) preloadedConstantState.newDrawable(res);
        assertEquals(initialRadius, origDrawable.getRadius());

        // Ensure theme density is applied correctly.
        final Theme t = res.newTheme();
        halfDrawable.applyTheme(t);
        assertEquals(initialRadius, halfDrawable.getRadius());
        doubleDrawable.applyTheme(t);
        assertEquals(initialRadius, doubleDrawable.getRadius());
    }

    public void testSetColor() {
        Drawable.Callback cb = mock(Drawable.Callback.class);
        RippleDrawable dr = new RippleDrawable(ColorStateList.valueOf(Color.RED), null, null);
        dr.setCallback(cb);

        dr.setColor(ColorStateList.valueOf(Color.BLACK));
        verify(cb, times(1)).invalidateDrawable(dr);
    }
}
