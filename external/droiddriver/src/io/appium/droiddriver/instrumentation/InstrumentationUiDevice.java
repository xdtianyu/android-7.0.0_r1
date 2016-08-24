/*
 * Copyright (C) 2013 DroidDriver committers
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

package io.appium.droiddriver.instrumentation;

import android.graphics.Bitmap;
import android.graphics.Bitmap.Config;
import android.graphics.Canvas;
import android.graphics.Rect;
import android.graphics.RectF;
import android.util.Log;
import android.view.View;

import java.util.concurrent.Callable;

import io.appium.droiddriver.base.BaseUiDevice;
import io.appium.droiddriver.base.DroidDriverContext;
import io.appium.droiddriver.util.InstrumentationUtils;
import io.appium.droiddriver.util.Logs;

class InstrumentationUiDevice extends BaseUiDevice {
  private final DroidDriverContext<View, ViewElement> context;

  InstrumentationUiDevice(DroidDriverContext<View, ViewElement> context) {
    this.context = context;
  }

  @Override
  protected Bitmap takeScreenshot() {
    try {
      return InstrumentationUtils.runOnMainSyncWithTimeout(new GetScreenshot(
          context.getDriver().getRootElement().getRawElement()));
    } catch (Throwable e) {
      Logs.log(Log.ERROR, e);
      return null;
    }
  }

  @Override
  protected DroidDriverContext<View, ViewElement> getContext() {
    return context;
  }

  private static class GetScreenshot implements Callable<Bitmap> {
    private final View rootView;

    private GetScreenshot(View rootView) {
      this.rootView = rootView;
    }

    @Override
    public Bitmap call() {
      Bitmap screenshot;
      rootView.destroyDrawingCache();
      rootView.buildDrawingCache(false);
      Bitmap drawingCache = rootView.getDrawingCache();

      int[] xy = new int[2];
      rootView.getLocationOnScreen(xy);
      if (xy[0] == 0 && xy[1] == 0) {
        screenshot = Bitmap.createBitmap(drawingCache);
      } else {
        Canvas canvas = new Canvas();
        Rect rect = new Rect(0, 0, drawingCache.getWidth(), drawingCache.getHeight());
        rect.offset(xy[0], xy[1]);
        screenshot =
            Bitmap.createBitmap(rect.width() + xy[0], rect.height() + xy[1], Config.ARGB_8888);
        canvas.setBitmap(screenshot);
        canvas.drawBitmap(drawingCache, null, new RectF(rect), null);
        canvas.setBitmap(null);
      }
      rootView.destroyDrawingCache();
      return screenshot;
    }
  }
}
