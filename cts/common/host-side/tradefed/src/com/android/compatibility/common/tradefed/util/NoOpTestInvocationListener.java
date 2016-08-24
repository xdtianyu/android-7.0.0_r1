/*
 * Copyright (C) 2015 The Android Open Source Project
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
package com.android.compatibility.common.tradefed.util;

import com.android.ddmlib.testrunner.TestIdentifier;
import com.android.ddmlib.testrunner.ITestRunListener;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.result.InputStreamSource;
import com.android.tradefed.result.LogDataType;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.TestSummary;

import java.util.Map;

/**
 * Implementation of ITestInvocationListener that does nothing or returns null for all methods.
 * Extend this class to implement some, but not all methods of ITestInvocationListener.
 */
public class NoOpTestInvocationListener implements ITestInvocationListener {

    /**
     * {@inheritDoc}
     */
    @Override
    public void invocationStarted(IBuildInfo buildInfo) {}

     /**
     * {@inheritDoc}
     */
    @Override
    public void testLog(String dataName, LogDataType dataType, InputStreamSource dataStream) {}

    /**
     * {@inheritDoc}
     */
    @Override
    public void invocationEnded(long elapsedTime) {}

    /**
     * {@inheritDoc}
     */
    @Override
    public void invocationFailed(Throwable cause) {}

    /**
     * {@inheritDoc}
     */
    @Override
    public TestSummary getSummary() {
        return null;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void testRunStarted(String runName, int testCount) {}

    /**
     * {@inheritDoc}
     */
    @Override
    public void testStarted(TestIdentifier test) {}

    /**
     * {@inheritDoc}
     */
    @Override
    public void testFailed(TestIdentifier test, String trace) {}

    /**
     * {@inheritDoc}
     */
    @Override
    public void testAssumptionFailure(TestIdentifier test, String trace) {}

    /**
     * {@inheritDoc}
     */
    @Override
    public void testIgnored(TestIdentifier test) {}

    /**
     * {@inheritDoc}
     */
    @Override
    public void testEnded(TestIdentifier test, Map<String, String> testMetrics) {}

    /**
     * {@inheritDoc}
     */
    @Override
    public void testRunFailed(String errorMessage) {}

    /**
     * {@inheritDoc}
     */
    @Override
    public void testRunStopped(long elapsedTime) {}

    /**
     * {@inheritDoc}
     */
    @Override
    public void testRunEnded(long elapsedTime, Map<String, String> runMetrics) {}

}
