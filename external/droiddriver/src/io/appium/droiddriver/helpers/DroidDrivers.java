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

package io.appium.droiddriver.helpers;

import android.annotation.TargetApi;
import android.app.Instrumentation;
import android.os.Build;

import io.appium.droiddriver.DroidDriver;
import io.appium.droiddriver.exceptions.DroidDriverException;
import io.appium.droiddriver.instrumentation.InstrumentationDriver;
import io.appium.droiddriver.uiautomation.UiAutomationDriver;
import io.appium.droiddriver.util.InstrumentationUtils;

/**
 * Static utility methods using a singleton {@link DroidDriver} instance. This class is NOT
 * required, but it is handy and using a singleton driver can avoid memory leak when you have many
 * instances around (for example, one in every test - JUnit framework keeps the test instances in
 * memory after running them).
 */
public class DroidDrivers {
  private static DroidDriver driver;

  /**
   * Gets the singleton driver. Throws if {@link #setSingleton} has not been called.
   */
  public static DroidDriver get() {
    if (driver == null) {
      throw new DroidDriverException("setSingleton() has not been called");
    }
    return driver;
  }

  /**
   * Sets the singleton driver.
   */
  public static void setSingleton(DroidDriver driver) {
    if (DroidDrivers.driver != null) {
      throw new DroidDriverException("setSingleton() can only be called once");
    }
    DroidDrivers.driver = driver;
  }

  /**
   * Returns whether the running target (device or emulator) has {@link android.app.UiAutomation}
   * API, which is introduced in SDK API 18 (JELLY_BEAN_MR2).
   */
  public static boolean hasUiAutomation() {
    return Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR2;
  }

  /**
   * Returns a new DroidDriver instance. If am instrument options have "driver", treat it as the
   * fully-qualified-class-name and create a new instance of it with {@code instrumentation} as the
   * argument; otherwise a new platform-dependent default DroidDriver instance.
   */
  public static DroidDriver newDriver() {
    Instrumentation instrumentation = InstrumentationUtils.getInstrumentation();
    String driverClass = InstrumentationUtils.getD2Option("driver");
    if (driverClass != null) {
      try {
        return (DroidDriver) Class.forName(driverClass).getConstructor(Instrumentation.class)
            .newInstance(instrumentation);
      } catch (Throwable t) {
        throw DroidDriverException.propagate(t);
      }
    }

    // If "dd.driver" is not specified, return default.
    if (hasUiAutomation()) {
      checkUiAutomation();
      return new UiAutomationDriver(instrumentation);
    }
    return new InstrumentationDriver(instrumentation);
  }

  /** Checks if UiAutomation API is available */
  @TargetApi(18)
  public static void checkUiAutomation() {
    if (!hasUiAutomation()) {
      throw new DroidDriverException("UiAutomation is not available below API 18. "
          + "See http://developer.android.com/reference/android/app/UiAutomation.html");
    }
    if (InstrumentationUtils.getInstrumentation().getUiAutomation() == null) {
      throw new DroidDriverException(
          "uiAutomation==null: did you forget to set '-w' flag for 'am instrument'?");
    }
  }
}
