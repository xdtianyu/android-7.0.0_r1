/*
 * Copyright (C) 2011 The Android Open Source Project
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

package android.app.cts;

import android.app.WallpaperManager;
import android.content.Context;
import android.graphics.Point;
import android.test.AndroidTestCase;
import android.view.Display;
import android.view.WindowManager;

public class WallpaperManagerTest extends AndroidTestCase {

    private WallpaperManager mWallpaperManager;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mWallpaperManager = WallpaperManager.getInstance(mContext);
    }

    /**
     * Suggesting desired dimensions is only a hint to the system that can be ignored.
     *
     * Test if the desired minimum width or height the WallpaperManager returns
     * is greater than 0. If so, then we check whether that the size is at least the
     * as big as the screen.
     */
    public void testSuggestDesiredDimensions() {
        final Point min = getScreenSize();
        final int w = min.x * 3;
        final int h = min.y * 2;
        assertDesiredMinimum(new Point(min.x / 2, min.y / 2), min);

        assertDesiredMinimum(new Point(w, h), min);

        assertDesiredMinimum(new Point(min.x / 2, h), min);

        assertDesiredMinimum(new Point(w, min.y / 2), min);
    }

    private void assertDesiredMinimum(Point suggestedSize, Point minSize) {
        mWallpaperManager.suggestDesiredDimensions(suggestedSize.x, suggestedSize.y);
        Point actualSize = new Point(mWallpaperManager.getDesiredMinimumWidth(),
                mWallpaperManager.getDesiredMinimumHeight());
        if (actualSize.x > 0 || actualSize.y > 0) {
            if((actualSize.x < minSize.x || actualSize.y < minSize.y)){
                throw new AssertionError("Expected at least x: " + minSize.x + " y: "
                                         + minSize.y + ", got x: " + actualSize.x +
                                         " y: " + actualSize.y );
            }
        }
    }

    private Point getScreenSize() {
        WindowManager wm = (WindowManager)mContext.getSystemService(Context.WINDOW_SERVICE);
        Display d = wm.getDefaultDisplay();
        Point p = new Point();
        d.getRealSize(p);
        return p;
    }
}
