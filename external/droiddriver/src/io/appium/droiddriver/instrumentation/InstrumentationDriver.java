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

import android.app.Instrumentation;
import android.os.SystemClock;
import android.util.Log;
import android.view.View;

import java.util.List;
import java.util.concurrent.Callable;

import io.appium.droiddriver.actions.InputInjector;
import io.appium.droiddriver.base.BaseDroidDriver;
import io.appium.droiddriver.base.DroidDriverContext;
import io.appium.droiddriver.exceptions.NoRunningActivityException;
import io.appium.droiddriver.util.ActivityUtils;
import io.appium.droiddriver.util.InstrumentationUtils;
import io.appium.droiddriver.util.Logs;

/**
 * Implementation of DroidDriver that is driven via instrumentation.
 */
public class InstrumentationDriver extends BaseDroidDriver<View, ViewElement> {
  private final DroidDriverContext<View, ViewElement> context;
  private final InputInjector injector;
  private final InstrumentationUiDevice uiDevice;

  public InstrumentationDriver(Instrumentation instrumentation) {
    context = new DroidDriverContext<View, ViewElement>(instrumentation, this);
    injector = new InstrumentationInputInjector(instrumentation);
    uiDevice = new InstrumentationUiDevice(context);
  }

  @Override
  public InputInjector getInjector() {
    return injector;
  }

  @Override
  protected ViewElement newRootElement() {
    return context.newRootElement(findRootView());
  }

  @Override
  protected ViewElement newUiElement(View rawElement, ViewElement parent) {
    return new ViewElement(context, rawElement, parent);
  }

  private static final Callable<View> FIND_ROOT_VIEW = new Callable<View>() {
    @Override
    public View call() {
      List<View> views = RootFinder.getRootViews();
      if (views.size() > 1) {
        Logs.log(Log.VERBOSE, "views.size()=" + views.size());
        for (View view : views) {
          if (view.hasWindowFocus()) {
            return view;
          }
        }
      }
      // Fall back to DecorView.
      return ActivityUtils.getRunningActivity().getWindow().getDecorView();
    }
  };

  private View findRootView() {
    waitForRunningActivity();
    return InstrumentationUtils.runOnMainSyncWithTimeout(FIND_ROOT_VIEW);
  }

  private void waitForRunningActivity() {
    long timeoutMillis = getPoller().getTimeoutMillis();
    long end = SystemClock.uptimeMillis() + timeoutMillis;
    while (true) {
      if (ActivityUtils.getRunningActivity() != null) {
        return;
      }
      long remainingMillis = end - SystemClock.uptimeMillis();
      if (remainingMillis < 0) {
        throw new NoRunningActivityException(String.format(
            "Cannot find the running activity after %d milliseconds", timeoutMillis));
      }
      SystemClock.sleep(Math.min(250, remainingMillis));
    }
  }

  @Override
  public InstrumentationUiDevice getUiDevice() {
    return uiDevice;
  }
}
