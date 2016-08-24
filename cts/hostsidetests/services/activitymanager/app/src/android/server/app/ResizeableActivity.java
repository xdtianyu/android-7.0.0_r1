/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy of
 * the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations under
 * the License.
 */
package android.server.app;

import android.content.res.Configuration;
import android.graphics.Point;
import android.os.Bundle;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.Display;
import android.view.WindowManager;

public class ResizeableActivity extends AbstractLifecycleLogActivity {
    @Override
    protected String getTag() {
        return "ResizeableActivity";
    }

    @Override
    protected void onCreate(Bundle icicle) {
        super.onCreate(icicle);
        dumpDisplaySize(getResources().getConfiguration());
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
        dumpDisplaySize(newConfig);
    }

    private void dumpDisplaySize(Configuration config) {
        // Dump the display size as seen by this Activity.
        final WindowManager wm = getSystemService(WindowManager.class);
        final Display display = wm.getDefaultDisplay();
        final Point point = new Point();
        display.getSize(point);
        final DisplayMetrics metrics = getResources().getDisplayMetrics();

        final String line = "config" +
                " size=" + buildCoordString(config.screenWidthDp, config.screenHeightDp) +
                " displaySize=" + buildCoordString(point.x, point.y) +
                " metricsSize=" + buildCoordString(metrics.widthPixels, metrics.heightPixels);

        Log.i(getTag(), line);
    }

    private static String buildCoordString(int x, int y) {
        return "(" + x + "," + y + ")";
    }
}
