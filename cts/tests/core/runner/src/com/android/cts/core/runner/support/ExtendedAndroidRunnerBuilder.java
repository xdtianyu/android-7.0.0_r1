/*
 * Copyright (C) 2016 The Android Open Source Project
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

package com.android.cts.core.runner.support;

import android.support.test.internal.runner.AndroidLogOnlyBuilder;
import android.support.test.internal.util.AndroidRunnerParams;
import android.util.Log;
import org.junit.runners.model.RunnerBuilder;
import org.junit.runner.Runner;

/**
 * Extends {@link AndroidRunnerBuilder} in order to provide alternate {@link RunnerBuilder}
 * implementations.
 */
public class ExtendedAndroidRunnerBuilder extends AndroidRunnerBuilder {

    private final ExtendedAndroidLogOnlyBuilder androidLogOnlyBuilder;
    private static final boolean DEBUG = false;

    private final TestNgRunnerBuilder mTestNgBuilder;

    /**
     * @param runnerParams {@link AndroidRunnerParams} that stores common runner parameters
     */
    public ExtendedAndroidRunnerBuilder(AndroidRunnerParams runnerParams) {
        super(runnerParams, false /* CTSv1 filtered out Test suite() classes. */);
        androidLogOnlyBuilder = new ExtendedAndroidLogOnlyBuilder(runnerParams);
        mTestNgBuilder = new TestNgRunnerBuilder(runnerParams);
    }

    @Override
    public Runner runnerForClass(Class<?> testClass) throws Throwable {
      if (DEBUG) {
        Log.d("ExAndRunBuild", "runnerForClass: Searching runner for class " + testClass.getName());
      }

      Runner runner;
      // Give TestNG tests a chance to participate in the Runner search first.
      // (Note that the TestNG runner handles log-only runs by itself)
      runner = mTestNgBuilder.runnerForClass(testClass);
      if (runner != null) {
        logFoundRunner(runner);
        return runner;
      }

      // Check if this is a dry-run with -e log true argument passed to the runner.
      runner = androidLogOnlyBuilder.runnerForClass(testClass);
      if (runner != null) {
        logFoundRunner(runner);
        return runner;
      }

      // Use the normal Runner search mechanism (for Junit tests).
      runner = super.runnerForClass(testClass);
      logFoundRunner(runner);

      return runner;
    }

    private static void logFoundRunner(Runner runner) {
      if (DEBUG) {
        Log.d("ExAndRunBuild", "runnerForClass: Found runner of type " +
            ((runner == null) ? "<null>" : runner.getClass().getName()));
      }
    }
}
