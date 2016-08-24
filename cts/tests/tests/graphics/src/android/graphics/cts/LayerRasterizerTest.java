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

package android.graphics.cts;

import junit.framework.TestCase;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.LayerRasterizer;
import android.graphics.Paint;
import android.graphics.Rasterizer;

public class LayerRasterizerTest extends TestCase {
    private final static int BITMAP_WIDTH = 16;
    private final static int BITMAP_HEIGHT = 16;

    private void exerciseRasterizer(Rasterizer rasterizer) {
        Bitmap bm = Bitmap.createBitmap(BITMAP_WIDTH, BITMAP_HEIGHT, Bitmap.Config.ARGB_8888);
        Canvas canvas = new Canvas(bm);
        Paint paint = new Paint();

        // just want to confirm that we don't crash or throw an exception
        paint.setRasterizer(rasterizer);
        canvas.drawCircle(BITMAP_WIDTH/2, BITMAP_WIDTH/2, BITMAP_WIDTH/2, paint);
    }

    public void testConstructor() {
        exerciseRasterizer(new LayerRasterizer());
    }

    public void testAddLayer1() {
        LayerRasterizer layerRasterizer = new LayerRasterizer();
        Paint p = new Paint();
        layerRasterizer.addLayer(p);
        exerciseRasterizer(layerRasterizer);
    }

    public void testAddLayer2() {
        LayerRasterizer layerRasterizer = new LayerRasterizer();
        layerRasterizer.addLayer(new Paint(), 1.0f, 1.0f);
        exerciseRasterizer(layerRasterizer);
        // explicitly add another layer and draw again
        layerRasterizer.addLayer(new Paint(), 2.0f, 2.0f);
        exerciseRasterizer(layerRasterizer);
    }

}
