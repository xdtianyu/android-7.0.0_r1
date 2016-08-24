/*
 * Copyright (C) 2015 DroidDriver committers
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

package io.appium.droiddriver.android_support_test;

import android.app.Activity;
import android.os.Bundle;
import android.os.Looper;
import android.support.test.runner.AndroidJUnitRunner;
import android.support.test.runner.lifecycle.ActivityLifecycleMonitorRegistry;
import android.support.test.runner.lifecycle.Stage;
import android.util.Log;

import java.util.Iterator;
import java.util.concurrent.Callable;

import io.appium.droiddriver.util.ActivityUtils;
import io.appium.droiddriver.util.InstrumentationUtils;
import io.appium.droiddriver.util.Logs;

/**
 * Integrates DroidDriver with AndroidJUnitRunner. <p> TODO: support DroidDriver test filter
 * annotations.
 */
public class D2AndroidJUnitRunner extends AndroidJUnitRunner {
  private static final Callable<Activity> GET_RUNNING_ACTIVITY = new Callable<Activity>() {
    @Override
    public Activity call() {
      Iterator<Activity> activityIterator = ActivityLifecycleMonitorRegistry.getInstance()
          .getActivitiesInStage(Stage.RESUMED).iterator();
      return activityIterator.hasNext() ? activityIterator.next() : null;
    }
  };

  /**
   * {@inheritDoc} <p> Initializes {@link InstrumentationUtils}.
   */
  @Override
  public void onCreate(Bundle arguments) {
    InstrumentationUtils.init(this, arguments);
    super.onCreate(arguments);
  }

  /**
   * {@inheritDoc} <p> Hooks {@link ActivityUtils#setRunningActivitySupplier} to {@link
   * ActivityLifecycleMonitorRegistry}.
   */
  @Override
  public void onStart() {
    ActivityUtils.setRunningActivitySupplier(new ActivityUtils.Supplier<Activity>() {
      @Override
      public Activity get() {
        try {
          // If this is called on main (UI) thread, don't call runOnMainSync
          if (Looper.myLooper() == Looper.getMainLooper()) {
            return GET_RUNNING_ACTIVITY.call();
          }

          return InstrumentationUtils.runOnMainSyncWithTimeout(GET_RUNNING_ACTIVITY);
        } catch (Exception e) {
          Logs.log(Log.WARN, e);
          return null;
        }
      }
    });

    super.onStart();
  }
}
