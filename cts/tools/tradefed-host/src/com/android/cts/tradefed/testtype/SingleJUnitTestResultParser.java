/*
 * Copyright (C) 2014 The Android Open Source Project
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

package com.android.cts.tradefed.testtype;

import com.android.ddmlib.MultiLineReceiver;
import com.android.ddmlib.testrunner.ITestRunListener;
import com.android.ddmlib.testrunner.TestIdentifier;

import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.Map;

/**
 * Parses the test results from {@link com.android.cts.junit.SingleJUnitTestRunner}
 */
public class SingleJUnitTestResultParser extends MultiLineReceiver {

    private static final String PASSED_TEST_MARKER = "[ PASSED ]";
    private static final String FAILED_TEST_MARKER = "[ FAILED ]";
    private final TestIdentifier mTestId;
    private final Collection<ITestRunListener> mTestListeners;
    private StringBuilder mStackTrace = new StringBuilder();

    public SingleJUnitTestResultParser(TestIdentifier testId, Collection<ITestRunListener> listeners) {
        mTestId = testId;
        mTestListeners = new ArrayList<ITestRunListener>(listeners);
    }

    public SingleJUnitTestResultParser(TestIdentifier testId, ITestRunListener listener) {
        mTestId = testId;
        mTestListeners = new ArrayList<ITestRunListener>(1);
        mTestListeners.add(listener);
    }

    @Override
    public boolean isCancelled() {
        // TODO Auto-generated method stub
        return false;
    }

    @Override
    public void processNewLines(String[] lines) {
        for (String line : lines) {
            parse(line);
        }
    }

    /**
     * Parses a given string.
     * @param line
     */
    private void parse(String line) {
        if (line.startsWith(PASSED_TEST_MARKER)) {
            doTestEnded(true);
        } else if (line.startsWith(FAILED_TEST_MARKER)) {
            doTestEnded(false);
        } else {
            // Store everything in case there is a failure.
            mStackTrace.append("\n");
            mStackTrace.append(line);
        }
    }

    /**
     * Handle cases when test ends.
     * @param testPassed whether or not the test passed.
     */
    private void doTestEnded(boolean testPassed) {
        // If test failed.
        if (!testPassed) {
            for (ITestRunListener listener : mTestListeners) {
                listener.testFailed(mTestId, mStackTrace.toString());
            }
        }
        Map<String, String> emptyMap = Collections.emptyMap();
        for (ITestRunListener listener : mTestListeners) {
            listener.testEnded(mTestId, emptyMap);
        }
        mStackTrace = new StringBuilder();
    }
}
