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

package android.uirendering.cts.testclasses;

import android.graphics.Canvas;
import android.graphics.Path;
import android.graphics.RectF;
import android.graphics.Region;
import android.test.suitebuilder.annotation.MediumTest;
import android.uirendering.cts.testinfrastructure.ActivityTestBase;
import org.junit.Test;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

/**
 * Tests of state query-able from canvas at draw time.
 *
 * Although these tests don't verify drawing content, they still make use of ActivityTestBase's
 * capability to test the hardware accelerated Canvas in the way that it is used by Views.
 */
@MediumTest
public class CanvasStateTests extends ActivityTestBase {
    @Test
    public void testClipRectReturnValues() {
        createTest()
                .addCanvasClient((canvas, width, height) -> {
                    canvas.save();
                    boolean isNonEmpty = canvas.clipRect(0, 0, 20, 20);
                    assertTrue("clip state should be non empty", isNonEmpty);

                    isNonEmpty = canvas.clipRect(0, 40, 20, 60);
                    assertFalse("clip state should be empty", isNonEmpty);
                    canvas.restore();
                })
                .runWithoutVerification();
    }

    @Test
    public void testClipRegionReturnValues() {
        createTest()
                .addCanvasClient((canvas, width, height) -> {
                    canvas.save();
                    RectF clipRectF = new RectF(0, 0, 20, 20);

                    assertFalse(canvas.quickReject(0, 0, 20, 20, Canvas.EdgeType.BW));
                    if (!canvas.isHardwareAccelerated()) {
                        // SW canvas may not be in View space, so we offset the clipping region
                        // so it will operate within the canvas client's window.
                        // (Currently, this isn't necessary, since SW layer size == draw area)
                        canvas.getMatrix().mapRect(clipRectF);
                    }

                    Region rectRegion = new Region();
                    rectRegion.set((int) clipRectF.left, (int) clipRectF.top,
                            (int) clipRectF.right, (int) clipRectF.bottom);

                    boolean isNonEmpty = canvas.clipRegion(rectRegion);
                    assertTrue("clip state should be non empty", isNonEmpty);

                    // Note: we don't test that non-intersecting clip regions empty the clip,
                    // For region clipping, the impl is allowed to return true conservatively
                    // in many cases.
                    canvas.restore();
                })
                .runWithoutVerification();
    }

    @Test
    public void testClipPathReturnValues() {
        createTest()
                .addCanvasClient((canvas, width, height) -> {
                    canvas.save();
                    Path rectPath = new Path();
                    rectPath.addRect(0, 0, 20, 20, Path.Direction.CW);

                    boolean isNonEmpty = canvas.clipPath(rectPath);
                    assertTrue("clip state should be non empty", isNonEmpty);

                    rectPath.offset(0, 40);
                    isNonEmpty = canvas.clipPath(rectPath);
                    assertFalse("clip state should be empty", isNonEmpty);
                    canvas.restore();
                })
                .runWithoutVerification();
    }
    @Test
    public void testQuickReject() {
        createTest()
                .addCanvasClient((canvas, width, height) -> {
                    canvas.save();
                    canvas.clipRect(0, 0, 20, 20);

                    // not rejected!
                    assertFalse(canvas.quickReject(0, 0, 20, 20, Canvas.EdgeType.BW));

                    // rejected!
                    assertTrue(canvas.quickReject(0, 40, 20, 60, Canvas.EdgeType.BW));
                    canvas.restore();
                })
                .runWithoutVerification();
    }
}
