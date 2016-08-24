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

import android.app.Activity;
import android.content.Context;
import android.os.Debug;
import android.test.FlakyTest;
import android.util.Log;

import java.io.IOException;
import java.lang.Thread.UncaughtExceptionHandler;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.lang.reflect.Modifier;

import io.appium.droiddriver.DroidDriver;
import io.appium.droiddriver.exceptions.UnrecoverableException;
import io.appium.droiddriver.util.FileUtils;
import io.appium.droiddriver.util.Logs;

/**
 * Base class for tests using DroidDriver that reports uncaught exceptions, for * example OOME,
 * instead of crash. Also supports other features, including taking screenshot on failure. It is NOT
 * required, but provides handy features.
 */
public abstract class BaseDroidDriverTest<T extends Activity> extends
    D2ActivityInstrumentationTestCase2<T> {

  private static boolean classSetUpDone = false;
  // In case of device-wide fatal errors, e.g. OOME, the remaining tests will
  // fail and the messages will not help, so skip them.
  private static boolean skipRemainingTests = false;
  // Store uncaught exception from AUT.
  private static volatile Throwable uncaughtException;
  static {
    Thread.setDefaultUncaughtExceptionHandler(new UncaughtExceptionHandler() {
      @Override
      public void uncaughtException(Thread thread, Throwable ex) {
        uncaughtException = ex;
        // In most cases uncaughtException will be reported by onFailure().
        // But if it occurs in InstrumentationTestRunner, it's swallowed.
        // Always log it for all cases.
        Logs.log(Log.ERROR, uncaughtException, "uncaughtException");
      }
    });
  }

  protected DroidDriver driver;

  protected BaseDroidDriverTest(Class<T> activityClass) {
    super(activityClass);
  }

  @Override
  protected void setUp() throws Exception {
    super.setUp();
    if (!classSetUpDone) {
      classSetUp();
      classSetUpDone = true;
    }
    driver = DroidDrivers.get();
  }

  @Override
  protected void tearDown() throws Exception {
    super.tearDown();
    driver = null;
  }

  protected Context getTargetContext() {
    return getInstrumentation().getTargetContext();
  }

  /**
   * Initializes test fixture once for all tests extending this class. This may have unexpected
   * behavior - if multiple subclasses override this method, only the first override is executed.
   * Other overrides are silently ignored. You can either use {@link SingleRun} in {@link #setUp},
   * or override this method, which is a simpler alternative with the aforementioned catch.
   * <p>
   * If an InstrumentationDriver is used, this is a good place to call {@link
   * io.appium.droiddriver.instrumentation.ViewElement#overrideClassName}
   */
  protected void classSetUp() {
    DroidDriversInitializer.get(DroidDrivers.newDriver()).singleRun();
  }

  protected boolean reportSkippedAsFailed() {
    return false;
  }

  protected void skip() {
    if (reportSkippedAsFailed()) {
      fail("Skipped due to prior failure");
    }
  }

  /**
   * Hook for handling failure, for example, taking a screenshot.
   */
  protected void onFailure(Throwable failure) throws Throwable {
    // If skipRemainingTests is true, the failure has already been reported.
    if (skipRemainingTests) {
      return;
    }
    if (shouldSkipRemainingTests(failure)) {
      skipRemainingTests = true;
    }

    // Give uncaughtException (thrown by AUT instead of tests) high priority
    if (uncaughtException != null) {
      failure = uncaughtException;
    }

    try {
      if (failure instanceof OutOfMemoryError) {
        dumpHprof();
      } else if (uncaughtException == null) {
        String baseFileName = getBaseFileName();
        driver.dumpUiElementTree(baseFileName + ".xml");
        driver.getUiDevice().takeScreenshot(baseFileName + ".png");
      }
    } catch (Throwable e) {
      // This method is for troubleshooting. Do not throw new error; we'll
      // throw the original failure.
      Logs.log(Log.WARN, e);
      if (e instanceof OutOfMemoryError && !(failure instanceof OutOfMemoryError)) {
        skipRemainingTests = true;
        try {
          dumpHprof();
        } catch (Throwable ignored) {
        }
      }
    }

    throw failure;
  }

  protected boolean shouldSkipRemainingTests(Throwable e) {
    return e instanceof UnrecoverableException || e instanceof OutOfMemoryError
        || skipRemainingTests || uncaughtException != null;
  }

  /**
   * Gets the base filename for troubleshooting files. For example, a screenshot
   * is saved in the file "basename".png.
   */
  protected String getBaseFileName() {
    return "dd/" + getClass().getSimpleName() + "." + getName();
  }

  protected void dumpHprof() throws IOException {
    String path = FileUtils.getAbsoluteFile(getBaseFileName() + ".hprof").getPath();
    // create an empty readable file
    FileUtils.open(path).close();
    Debug.dumpHprofData(path);
  }

  /**
   * Fixes JUnit3: always call tearDown even when setUp throws. Also adds the
   * {@link #onFailure} hook.
   */
  @Override
  public void runBare() throws Throwable {
    if (skipRemainingTests) {
      skip();
      return;
    }
    if (uncaughtException != null) {
      onFailure(uncaughtException);
    }

    Throwable exception = null;
    try {
      setUp();
      runTest();
    } catch (Throwable runException) {
      exception = runException;
      // ActivityInstrumentationTestCase2.tearDown() finishes activity
      // created by getActivity(), so call this before tearDown().
      onFailure(exception);
    } finally {
      try {
        tearDown();
      } catch (Throwable tearDownException) {
        if (exception == null) {
          exception = tearDownException;
        }
      }
    }
    if (exception != null) {
      throw exception;
    }
  }

  /**
   * Overrides to fail fast when the test is annotated as   FlakyTest and we should skip remaining
   * tests (the failure is fatal). Most lines are copied from super classes.
   * <p>
   * When a flaky test is re-run, tearDown() and setUp() are called first in order to reset state.
   */
  @Override
  protected void runTest() throws Throwable {
    String fName = getName();
    assertNotNull(fName);
    Method method = null;
    try {
      // use getMethod to get all public inherited
      // methods. getDeclaredMethods returns all
      // methods of this class but excludes the
      // inherited ones.
      method = getClass().getMethod(fName, (Class[]) null);
    } catch (NoSuchMethodException e) {
      fail("Method \"" + fName + "\" not found");
    }

    if (!Modifier.isPublic(method.getModifiers())) {
      fail("Method \"" + fName + "\" should be public");
    }

    int tolerance = 1;
    if (method.isAnnotationPresent(FlakyTest.class)) {
      tolerance = method.getAnnotation(FlakyTest.class).tolerance();
    }

    for (int runCount = 0; runCount < tolerance; runCount++) {
      if (runCount > 0) {
        Logs.logfmt(Log.INFO, "Running %s round %d of %d attempts", fName, runCount + 1, tolerance);
        // We are re-attempting a test, so reset all state.
        tearDown();
        setUp();
      }

      try {
        method.invoke(this);
        return;
      } catch (InvocationTargetException e) {
        e.fillInStackTrace();
        Throwable exception = e.getTargetException();
        if (shouldSkipRemainingTests(exception) || runCount >= tolerance - 1) {
          throw exception;
        }
        Logs.log(Log.WARN, exception);
      } catch (IllegalAccessException e) {
        e.fillInStackTrace();
        throw e;
      }
    }
  }
}
