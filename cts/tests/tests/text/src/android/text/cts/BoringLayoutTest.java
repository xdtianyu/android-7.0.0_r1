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

package android.text.cts;

import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Bitmap.Config;
import android.test.AndroidTestCase;
import android.text.BoringLayout;
import android.text.BoringLayout.Metrics;
import android.text.Layout;
import android.text.TextPaint;
import android.text.TextUtils;
import android.text.Layout.Alignment;

public class BoringLayoutTest extends AndroidTestCase {
    private static final float SPACING_MULT_NO_SCALE = 1.0f;
    private static final float SPACING_ADD_NO_SCALE = 0.0f;
    private static final int DEFAULT_OUTER_WIDTH = 100;
    private static final int METRICS_TOP = 10;
    private static final int METRICS_ASCENT = 20;
    private static final int METRICS_DESCENT = 40;
    private static final int METRICS_BOTTOM = 50;
    private static final int METRICS_WIDTH = 50;
    private static final int METRICS_LEADING = 50;

    private static final CharSequence DEFAULT_CHAR_SEQUENCE = "default";
    private static final TextPaint DEFAULT_PAINT = new TextPaint();
    private static final Layout.Alignment DEFAULT_ALIGN = Layout.Alignment.ALIGN_CENTER;
    private static final Metrics DEFAULT_METRICS = createMetrics(
            METRICS_TOP,
            METRICS_ASCENT,
            METRICS_DESCENT,
            METRICS_BOTTOM,
            METRICS_WIDTH,
            METRICS_LEADING);

    private BoringLayout mBoringLayout;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mBoringLayout = makeDefaultBoringLayout();
    }

    public void testConstructors() {
        new BoringLayout(DEFAULT_CHAR_SEQUENCE,
                DEFAULT_PAINT,
                DEFAULT_OUTER_WIDTH,
                DEFAULT_ALIGN,
                SPACING_MULT_NO_SCALE,
                SPACING_ADD_NO_SCALE,
                DEFAULT_METRICS,
                true);

        new BoringLayout(DEFAULT_CHAR_SEQUENCE,
                DEFAULT_PAINT,
                DEFAULT_OUTER_WIDTH,
                DEFAULT_ALIGN,
                SPACING_MULT_NO_SCALE,
                SPACING_ADD_NO_SCALE,
                DEFAULT_METRICS,
                true,
                TextUtils.TruncateAt.START,
                DEFAULT_OUTER_WIDTH);
    }

    private void verifyMultAddScale(float spacingMult, float spacingAdd) {
        final int height = METRICS_BOTTOM - METRICS_TOP;

        BoringLayout boringLayout = makeBoringLayout(spacingMult, spacingAdd);
        assertEquals(height, boringLayout.getHeight());
        assertEquals(height + METRICS_TOP, boringLayout.getLineDescent(0));
    }

    public void testScale() {
        // no scale
        verifyMultAddScale(1.0f, 0.0f);

        // test line spacing multiplier
        verifyMultAddScale(2.0f, 0.0f);
        verifyMultAddScale(0.5f, 0.0f);

        // test line spacing add
        verifyMultAddScale(1.0f, 1.5f);
        verifyMultAddScale(1.0f, -1.6f);
        verifyMultAddScale(1.0f, 1.4f);
        verifyMultAddScale(1.0f, -1.4f);
        verifyMultAddScale(1.0f, 3.0f);
        verifyMultAddScale(1.0f, -3.0f);
    }

    public void testPreconditions() {
        assertEquals(1, mBoringLayout.getLineCount());
        assertEquals(0, mBoringLayout.getLineTop(0));
        assertEquals(mBoringLayout.getHeight(), mBoringLayout.getLineTop(1));
        assertEquals(mBoringLayout.getHeight(), mBoringLayout.getLineTop(10));
        assertEquals(0, mBoringLayout.getLineStart(0));
        assertEquals(DEFAULT_CHAR_SEQUENCE.length(), mBoringLayout.getLineStart(1));
        assertEquals(DEFAULT_CHAR_SEQUENCE.length(), mBoringLayout.getLineStart(10));
        assertEquals(Layout.DIR_LEFT_TO_RIGHT, mBoringLayout.getParagraphDirection(0));
        assertFalse(mBoringLayout.getLineContainsTab(0));
        assertEquals((float) METRICS_WIDTH, mBoringLayout.getLineMax(0));
        assertEquals(Layout.DIR_LEFT_TO_RIGHT, mBoringLayout.getParagraphDirection(0));
        assertEquals(0, mBoringLayout.getEllipsisCount(0));
        mBoringLayout.ellipsized(0, 1);
        assertEquals(1, mBoringLayout.getEllipsisCount(0));
        mBoringLayout.ellipsized(1, 2);
        assertEquals(1, mBoringLayout.getEllipsisStart(0));
    }

    public void testReplaceOrMake() {
        String source = "This is a SpannableString.";
        BoringLayout layout_1 = mBoringLayout.replaceOrMake(
                source,
                DEFAULT_PAINT,
                DEFAULT_OUTER_WIDTH,
                DEFAULT_ALIGN,
                SPACING_MULT_NO_SCALE,
                SPACING_ADD_NO_SCALE,
                DEFAULT_METRICS,
                true);
        assertSame(mBoringLayout, layout_1);

        layout_1 = mBoringLayout.replaceOrMake(
                source,
                DEFAULT_PAINT,
                DEFAULT_OUTER_WIDTH,
                DEFAULT_ALIGN,
                SPACING_MULT_NO_SCALE,
                SPACING_ADD_NO_SCALE,
                DEFAULT_METRICS,
                true,
                TextUtils.TruncateAt.START,
                100);
        assertSame(mBoringLayout, layout_1);
        assertEquals(100, mBoringLayout.getEllipsizedWidth());
    }


    public void testAlignment() {
        BoringLayout boringLayout = makeBoringLayoutAlign(Layout.Alignment.ALIGN_NORMAL);
        assertEquals(0.0f, boringLayout.getLineLeft(0));
        assertEquals((float) DEFAULT_METRICS.width, boringLayout.getLineRight(0));

        boringLayout = makeBoringLayoutAlign(Layout.Alignment.ALIGN_CENTER);
        int expectedWidth = DEFAULT_OUTER_WIDTH - METRICS_WIDTH;
        assertEquals((float) expectedWidth / 2, boringLayout.getLineLeft(0));
        expectedWidth = DEFAULT_OUTER_WIDTH + METRICS_WIDTH;
        assertEquals((float) expectedWidth / 2, boringLayout.getLineRight(0));

        boringLayout = makeBoringLayoutAlign(Layout.Alignment.ALIGN_OPPOSITE);
        expectedWidth = DEFAULT_OUTER_WIDTH - METRICS_WIDTH;
        assertEquals((float) expectedWidth, boringLayout.getLineLeft(0));
        assertEquals((float) DEFAULT_OUTER_WIDTH, boringLayout.getLineRight(0));
    }

    public void testGetLineDescent_withIncludePadding() {
        final int height = METRICS_BOTTOM - METRICS_TOP;
        assertEquals(height + METRICS_TOP, mBoringLayout.getLineDescent(0));
    }

    public void testGetLineDescent_withoutIncludePadding() {
        BoringLayout boringLayout = new BoringLayout(
                DEFAULT_CHAR_SEQUENCE,
                DEFAULT_PAINT,
                DEFAULT_OUTER_WIDTH,
                DEFAULT_ALIGN,
                SPACING_MULT_NO_SCALE,
                SPACING_ADD_NO_SCALE,
                DEFAULT_METRICS,
                false);

        final int height = METRICS_DESCENT - METRICS_ASCENT;
        assertEquals(height + METRICS_ASCENT, boringLayout.getLineDescent(0));
    }

    public void testIncludePadding() {
        assertEquals(METRICS_TOP - METRICS_ASCENT, mBoringLayout.getTopPadding());
        assertEquals(METRICS_BOTTOM - METRICS_DESCENT, mBoringLayout.getBottomPadding());
        assertEquals(METRICS_BOTTOM - METRICS_TOP, mBoringLayout.getHeight());

        BoringLayout boringLayout = new BoringLayout(
                DEFAULT_CHAR_SEQUENCE,
                DEFAULT_PAINT,
                DEFAULT_OUTER_WIDTH,
                DEFAULT_ALIGN,
                SPACING_MULT_NO_SCALE,
                SPACING_ADD_NO_SCALE,
                DEFAULT_METRICS,
                false);

        assertEquals(0, boringLayout.getTopPadding());
        assertEquals(0, boringLayout.getBottomPadding());
        assertEquals(METRICS_DESCENT - METRICS_ASCENT, boringLayout.getHeight());
    }

    public void testIsBoringString() {
        TextPaint paint = new TextPaint();
        assertNotNull(BoringLayout.isBoring("hello android", paint));

        Metrics metrics = new Metrics();
        metrics.width = 100;
        assertNotNull(BoringLayout.isBoring("hello android", paint, metrics));

        assertNull(BoringLayout.isBoring("\u0590 \u0591", paint));
        assertNull(BoringLayout.isBoring("hello \t android", paint));
        assertNull(BoringLayout.isBoring("hello \n android", paint));
        assertNull(BoringLayout.isBoring("hello \n\n\n android", paint));
        assertNull(BoringLayout.isBoring("\nhello \n android\n", paint));
        assertNull(BoringLayout.isBoring("hello android\n\n\n", paint));
    }

    public void testIsBoring_resetsFontMetrics() {
        int someInt = 100;
        String text = "some text";

        TextPaint paint = new TextPaint();
        Paint.FontMetricsInt paintMetrics = paint.getFontMetricsInt();
        Metrics changedMetrics = new Metrics();
        changedMetrics.top = paintMetrics.top - someInt;
        changedMetrics.ascent = paintMetrics.ascent - someInt;
        changedMetrics.bottom = paintMetrics.bottom + someInt;
        changedMetrics.descent = paintMetrics.descent + someInt;
        changedMetrics.leading = paintMetrics.leading + someInt;

        Metrics expectedMetrics = BoringLayout.isBoring(text, paint, (Metrics) null);
        Metrics actualMetrics = BoringLayout.isBoring(text, paint, changedMetrics);

        assertNotNull(actualMetrics);
        assertNotNull(expectedMetrics);
        assertEquals(expectedMetrics.top, actualMetrics.top);
        assertEquals(expectedMetrics.ascent, actualMetrics.ascent);
        assertEquals(expectedMetrics.bottom, actualMetrics.bottom);
        assertEquals(expectedMetrics.descent, actualMetrics.descent);
        assertEquals(expectedMetrics.leading, actualMetrics.leading);
    }

    public void testGetLineDirections() {
        assertNotNull(mBoringLayout.getLineDirections(0));
        assertNotNull(mBoringLayout.getLineDirections(2));
    }

    public void testMake() {
        BoringLayout boringLayout = BoringLayout.make(DEFAULT_CHAR_SEQUENCE,
                DEFAULT_PAINT,
                DEFAULT_OUTER_WIDTH,
                DEFAULT_ALIGN,
                SPACING_MULT_NO_SCALE,
                SPACING_ADD_NO_SCALE,
                DEFAULT_METRICS,
                true);
        assertNotNull(boringLayout);

        boringLayout = null;
        boringLayout = BoringLayout.make(DEFAULT_CHAR_SEQUENCE,
                DEFAULT_PAINT,
                DEFAULT_OUTER_WIDTH,
                DEFAULT_ALIGN,
                SPACING_MULT_NO_SCALE,
                SPACING_ADD_NO_SCALE,
                DEFAULT_METRICS,
                true,
                TextUtils.TruncateAt.START,
                DEFAULT_OUTER_WIDTH);
        assertNotNull(boringLayout);
    }

    public void testDraw() {
        BoringLayout boringLayout = BoringLayout.make((String)DEFAULT_CHAR_SEQUENCE,
                DEFAULT_PAINT,
                DEFAULT_OUTER_WIDTH,
                Alignment.ALIGN_NORMAL,
                SPACING_MULT_NO_SCALE,
                SPACING_ADD_NO_SCALE,
                DEFAULT_METRICS,
                true);

        Bitmap mMutableBitmap = Bitmap.createBitmap(10, 28, Config.ARGB_8888);
        MockCanvas c = new MockCanvas(mMutableBitmap);
        boringLayout.draw(c, null, null, 0);
        assertTrue(c.isCanvasCalling);
    }

    private class MockCanvas extends Canvas {
        public boolean isCanvasCalling = false;

        public MockCanvas(Bitmap bitmap) {
            super(bitmap);
        }

        @Override
        public void drawText(String text, float x, float y, Paint paint) {
            super.drawText(text, x, y, paint);
            isCanvasCalling = true;
        }
    }

    private static Metrics createMetrics(
            final int top,
            final int ascent,
            final int descent,
            final int bottom,
            final int width,
            final int leading) {

        final Metrics metrics = new Metrics();

        metrics.top = top;
        metrics.ascent = ascent;
        metrics.descent = descent;
        metrics.bottom = bottom;
        metrics.width = width;
        metrics.leading = leading;

        return metrics;
    }

    private BoringLayout makeDefaultBoringLayout(){
        return new BoringLayout(DEFAULT_CHAR_SEQUENCE,
                                DEFAULT_PAINT,
                                DEFAULT_OUTER_WIDTH,
                                DEFAULT_ALIGN,
                                SPACING_MULT_NO_SCALE,
                                SPACING_ADD_NO_SCALE,
                                DEFAULT_METRICS,
                                true);
    }

    private BoringLayout makeBoringLayout(float spacingMult,float spacingAdd){
        return new BoringLayout(DEFAULT_CHAR_SEQUENCE,
                                DEFAULT_PAINT,
                                DEFAULT_OUTER_WIDTH,
                                DEFAULT_ALIGN,
                                spacingMult,
                                spacingAdd,
                                DEFAULT_METRICS,
                                true);
    }

    private BoringLayout makeBoringLayoutAlign(Alignment align){
        return new BoringLayout(DEFAULT_CHAR_SEQUENCE,
                                DEFAULT_PAINT,
                                DEFAULT_OUTER_WIDTH,
                                align,
                                SPACING_MULT_NO_SCALE,
                                SPACING_ADD_NO_SCALE,
                                DEFAULT_METRICS,
                                true);
    }
}
