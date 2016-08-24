/*
 * Copyright (C) 2014 The Android Open Source Project
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

package android.uirendering.cts.testclasses;

import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.PorterDuff;
import android.graphics.Rect;
import android.graphics.drawable.NinePatchDrawable;
import android.test.suitebuilder.annotation.MediumTest;
import android.uirendering.cts.bitmapcomparers.BitmapComparer;
import android.uirendering.cts.bitmapcomparers.ExactComparer;
import android.uirendering.cts.bitmapverifiers.BitmapVerifier;
import android.uirendering.cts.bitmapverifiers.RectVerifier;
import android.uirendering.cts.testinfrastructure.ActivityTestBase;
import android.uirendering.cts.R;
import org.junit.Test;

@MediumTest
public class ExactCanvasTests extends ActivityTestBase {
    private final BitmapComparer mExactComparer = new ExactComparer();

    @Test
    public void testBlueRect() {
        final Rect rect = new Rect(10, 10, 80, 80);
        createTest()
                .addCanvasClient((canvas, width, height) -> {
                    Paint p = new Paint();
                    p.setAntiAlias(false);
                    p.setColor(Color.BLUE);
                    canvas.drawRect(rect, p);
                })
                .runWithVerifier(new RectVerifier(Color.WHITE, Color.BLUE, rect));
    }

    @Test
    public void testPoints() {
        createTest()
                .addCanvasClient((canvas, width, height) -> {
                    Paint p = new Paint();
                    p.setAntiAlias(false);
                    p.setStrokeWidth(1f);
                    p.setColor(Color.BLACK);
                    for (int i = 0; i < 10; i++) {
                        canvas.drawPoint(i * 10, i * 10, p);
                    }
                })
                .runWithComparer(mExactComparer);
    }

    @Test
    public void testBlackRectWithStroke() {
        createTest()
                .addCanvasClient((canvas, width, height) -> {
                    Paint p = new Paint();
                    p.setColor(Color.RED);
                    canvas.drawRect(0, 0, ActivityTestBase.TEST_WIDTH,
                            ActivityTestBase.TEST_HEIGHT, p);
                    p.setColor(Color.BLACK);
                    p.setStrokeWidth(5);
                    canvas.drawRect(10, 10, 80, 80, p);
                })
                .runWithComparer(mExactComparer);
    }

    @Test
    public void testBlackLineOnGreenBack() {
        createTest()
                .addCanvasClient((canvas, width, height) -> {
                    canvas.drawColor(Color.GREEN);
                    Paint p = new Paint();
                    p.setColor(Color.BLACK);
                    p.setStrokeWidth(10);
                    canvas.drawLine(0, 0, 50, 0, p);
                })
                .runWithComparer(mExactComparer);
    }

    @Test
    public void testDrawRedRectOnBlueBack() {
        createTest()
                .addCanvasClient((canvas, width, height) -> {
                    canvas.drawColor(Color.BLUE);
                    Paint p = new Paint();
                    p.setColor(Color.RED);
                    canvas.drawRect(10, 10, 40, 40, p);
                })
                .runWithComparer(mExactComparer);
    }

    @Test
    public void testDrawLine() {
        createTest()
                .addCanvasClient((canvas, width, height) -> {
                    Paint p = new Paint();
                    canvas.drawColor(Color.WHITE);
                    p.setColor(Color.BLACK);
                    float[] pts = {
                            0, 0, 80, 80, 80, 0, 0, 80, 40, 50, 60, 50
                    };
                    canvas.drawLines(pts, p);
                })
                .runWithComparer(mExactComparer);
    }

    @Test
    public void testDrawWhiteScreen() {
        createTest()
                .addCanvasClient((canvas, width, height) -> canvas.drawColor(Color.WHITE))
                .runWithComparer(mExactComparer);
    }

    @Test
    public void testBasicText() {
        final String testString = "THIS IS A TEST";
        createTest()
                .addCanvasClient((canvas, width, height) -> {
                    Paint p = new Paint();
                    canvas.drawColor(Color.BLACK);
                    p.setColor(Color.WHITE);
                    p.setStrokeWidth(5);
                    canvas.drawText(testString, 30, 50, p);
                })
                .runWithComparer(mExactComparer);
    }

    @Test
    public void testBasicColorXfermode() {
        createTest()
                .addCanvasClient((canvas, width, height) -> {
                    canvas.drawColor(Color.GRAY);
                    canvas.drawColor(Color.BLUE, PorterDuff.Mode.MULTIPLY);
                })
                .runWithComparer(mExactComparer);
    }

    @Test
    public void testBluePaddedSquare() {
        final NinePatchDrawable ninePatchDrawable = (NinePatchDrawable)
            getActivity().getResources().getDrawable(R.drawable.blue_padded_square);
        ninePatchDrawable.setBounds(0, 0, 90, 90);

        BitmapVerifier verifier = new RectVerifier(Color.WHITE, Color.BLUE,
                new Rect(10, 10, 80, 80));

        createTest()
                .addCanvasClient((canvas, width, height) -> {
                    canvas.drawColor(Color.WHITE);
                    Paint p = new Paint();
                    p.setColor(Color.BLUE);
                    canvas.drawRect(10, 10, 80, 80, p);
                })
                .addCanvasClient((canvas, width, height) -> ninePatchDrawable.draw(canvas))
                .addLayout(R.layout.blue_padded_square, null)
                .runWithVerifier(verifier);
    }

    @Test
    public void testEmptyLayer() {
        createTest()
                .addCanvasClient((canvas, width, height) -> {
                    canvas.drawColor(Color.CYAN);
                    Paint p = new Paint();
                    p.setColor(Color.BLACK);
                    canvas.saveLayer(10, 10, 80, 80, p);
                    canvas.restore();
                })
                .runWithComparer(mExactComparer);
    }

}
