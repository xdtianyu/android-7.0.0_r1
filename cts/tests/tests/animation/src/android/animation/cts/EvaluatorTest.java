/*
 * Copyright (C) 2012 The Android Open Source Project
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

package android.animation.cts;

import android.animation.ArgbEvaluator;
import android.animation.FloatArrayEvaluator;
import android.animation.FloatEvaluator;
import android.animation.IntArrayEvaluator;
import android.animation.IntEvaluator;
import android.animation.PointFEvaluator;
import android.animation.RectEvaluator;
import android.graphics.Color;
import android.graphics.PointF;
import android.graphics.Rect;
import android.test.InstrumentationTestCase;

/**
 * Tests for the various Evaluator classes in android.animation
 */
public class EvaluatorTest extends InstrumentationTestCase {

    public void testFloatEvaluator() {
        float start = 0.0f;
        float end = 1.0f;
        float fraction = 0.5f;
        FloatEvaluator floatEvaluator = new FloatEvaluator();

        float result = floatEvaluator.evaluate(0, start, end);
        assertEquals(start, result, .001f);

        result = floatEvaluator.evaluate(fraction, start, end);
        assertEquals(.5f, result, .001f);

        result = floatEvaluator.evaluate(1, start, end);
        assertEquals(end, result, .001f);
    }

    public void testFloatArrayEvaluator() {
        float[] start = {0f, 0f};
        float[] end = {.8f, 1.0f};
        float fraction = 0.5f;
        FloatArrayEvaluator evaluator = new FloatArrayEvaluator();

        float[] result = evaluator.evaluate(0, start, end);
        assertEquals(start[0], result[0], .001f);
        assertEquals(start[1], result[1], .001f);

        result = evaluator.evaluate(fraction, start, end);
        assertEquals(.4f, result[0], .001f);
        assertEquals(.5f, result[1], .001f);

        result = evaluator.evaluate(1, start, end);
        assertEquals(end[0], result[0], .001f);
        assertEquals(end[1], result[1], .001f);
    }

    public void testArgbEvaluator() throws Throwable {
        final int RED =  0xffFF8080;
        final int BLUE = 0xff8080FF;
        int aRED = Color.alpha(RED);
        int rRED = Color.red(RED);
        int gRED = Color.green(RED);
        int bRED = Color.blue(RED);
        int aBLUE = Color.alpha(BLUE);
        int rBLUE = Color.red(BLUE);
        int gBLUE = Color.green(BLUE);
        int bBLUE = Color.blue(BLUE);

        final ArgbEvaluator evaluator = new ArgbEvaluator();

        int result = (Integer) evaluator.evaluate(0, RED, BLUE);
        int aResult = Color.alpha(result);
        int rResult = Color.red(result);
        int gResult = Color.green(result);
        int bResult = Color.blue(result);
        assertEquals(aRED, aResult);
        assertEquals(rRED, rResult);
        assertEquals(gRED, gResult);
        assertEquals(bRED, bResult);

        result = (Integer) evaluator.evaluate(.5f, RED, BLUE);
        aResult = Color.alpha(result);
        rResult = Color.red(result);
        gResult = Color.green(result);
        bResult = Color.blue(result);
        assertEquals(0xff, aResult);
        assertEquals(rRED + (int)(.5f * (rBLUE - rRED)), rResult);
        assertEquals(gRED + (int)(.5f * (gBLUE - gRED)), gResult);
        assertEquals(bRED + (int)(.5f * (bBLUE - bRED)), bResult);

        result = (Integer) evaluator.evaluate(1, RED, BLUE);
        aResult = Color.alpha(result);
        rResult = Color.red(result);
        gResult = Color.green(result);
        bResult = Color.blue(result);
        assertEquals(aBLUE, aResult);
        assertEquals(rBLUE, rResult);
        assertEquals(gBLUE, gResult);
        assertEquals(bBLUE, bResult);
    }

    public void testIntEvaluator() throws Throwable {
        final int start = 0;
        final int end = 100;
        final float fraction = 0.5f;
        final IntEvaluator intEvaluator = new IntEvaluator();

        int result = intEvaluator.evaluate(0, start, end);
        assertEquals(start, result);

        result = intEvaluator.evaluate(fraction, start, end);
        assertEquals(50, result);

        result = intEvaluator.evaluate(1, start, end);
        assertEquals(end, result);
    }

    public void testIntArrayEvaluator() {
        int[] start = {0, 0};
        int[] end = {80, 100};
        float fraction = 0.5f;
        IntArrayEvaluator evaluator = new IntArrayEvaluator();

        int[] result = evaluator.evaluate(0, start, end);
        assertEquals(start[0], result[0]);
        assertEquals(start[1], result[1]);

        result = evaluator.evaluate(fraction, start, end);
        assertEquals(40, result[0]);
        assertEquals(50, result[1]);

        result = evaluator.evaluate(1, start, end);
        assertEquals(end[0], result[0]);
        assertEquals(end[1], result[1]);
    }

    public void testRectEvaluator() throws Throwable {
        final RectEvaluator evaluator = new RectEvaluator();
        final Rect start = new Rect(0, 0, 0, 0);
        final Rect end = new Rect(100, 200, 300, 400);
        final float fraction = 0.5f;

        Rect result = evaluator.evaluate(0, start, end);
        assertEquals(start.left, result.left, .001f);
        assertEquals(start.top, result.top, .001f);
        assertEquals(start.right, result.right, .001f);
        assertEquals(start.bottom, result.bottom, 001f);

        result = evaluator.evaluate(fraction, start, end);
        assertEquals(50, result.left, .001f);
        assertEquals(100, result.top, .001f);
        assertEquals(150, result.right, .001f);
        assertEquals(200, result.bottom, .001f);

        result = evaluator.evaluate(1, start, end);
        assertEquals(end.left, result.left, .001f);
        assertEquals(end.top, result.top, .001f);
        assertEquals(end.right, result.right, .001f);
        assertEquals(end.bottom, result.bottom, .001f);
    }

    public void testPointFEvaluator() throws Throwable {
        final PointFEvaluator evaluator = new PointFEvaluator();
        final PointF start = new PointF(0, 0);
        final PointF end = new PointF(100, 200);
        final float fraction = 0.5f;

        PointF result = evaluator.evaluate(0, start, end);
        assertEquals(start.x, result.x, .001f);
        assertEquals(start.y, result.y, .001f);

        result = evaluator.evaluate(fraction, start, end);
        assertEquals(50, result.x, .001f);
        assertEquals(100, result.y, .001f);

        result = evaluator.evaluate(1, start, end);
        assertEquals(end.x, result.x, .001f);
        assertEquals(end.y, result.y, .001f);
    }

    /**
     * Utility method to compare float values. Exact equality is error-prone
     * with floating point values, so we ensure that the actual value is at least
     * within some epsilon of the expected value.
     */
    private void assertEquals(float expected, float actual) {
        if (expected != actual) {
            final float epsilon = .001f;
            assertTrue(actual <= expected + epsilon);
            assertTrue(actual >= expected - epsilon);
        }
    }
}

