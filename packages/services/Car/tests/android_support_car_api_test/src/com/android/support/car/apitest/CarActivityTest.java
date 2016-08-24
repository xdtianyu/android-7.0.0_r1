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

package com.android.support.car.apitest;

import android.support.car.Car;
import android.test.ActivityInstrumentationTestCase2;
import android.test.suitebuilder.annotation.MediumTest;

@MediumTest
public class CarActivityTest extends ActivityInstrumentationTestCase2<TestCarProxyActivity> {
    private static final long DEFAULT_WAIT_TIMEOUT_MS = 3000;

    private TestCarProxyActivity mActivity;

    public CarActivityTest() {
        super(TestCarProxyActivity.class);
    }

    public CarActivityTest(Class<TestCarProxyActivity> activityClass) {
        super(activityClass);
    }

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        TestCarActivity.testCleanup();
    }

    public void testCycle() throws Throwable {
        TestCarActivity.sCreateTestAction = new TestAction<TestCarActivity>() {
            @Override
            public void run(TestCarActivity param) {
                // TODO： Add tests
            }
        };
        TestCarActivity.sStartTestAction = new TestAction<TestCarActivity>() {
            @Override
            public void run(TestCarActivity param) {
                // TODO： Add tests
            }
        };
        TestCarActivity.sResumeTestAction = new TestAction<TestCarActivity>() {
            @Override
            public void run(TestCarActivity param) {
                // TODO： Add tests
            }
        };
        TestCarActivity.sPauseTestAction = new TestAction<TestCarActivity>() {
            @Override
            public void run(TestCarActivity param) {
                // TODO： Add tests
            }
        };
        TestCarActivity.sStopTestAction = new TestAction<TestCarActivity>() {
            @Override
            public void run(TestCarActivity param) {
                // TODO： Add tests
            }
        };
        TestCarActivity.sDestroyTestAction = new TestAction<TestCarActivity>() {
            @Override
            public void run(TestCarActivity param) {
                // TODO： Add tests
            }
        };
        mActivity = getActivity();
        TestCarActivity.sCreateTestAction.assertTestRun(DEFAULT_WAIT_TIMEOUT_MS);
        TestCarActivity.sStartTestAction.assertTestRun(DEFAULT_WAIT_TIMEOUT_MS);
        TestCarActivity.sResumeTestAction.assertTestRun(DEFAULT_WAIT_TIMEOUT_MS);
        mActivity.finish();
        TestCarActivity.sPauseTestAction.assertTestRun(DEFAULT_WAIT_TIMEOUT_MS);
        TestCarActivity.sStopTestAction.assertTestRun(DEFAULT_WAIT_TIMEOUT_MS);
        TestCarActivity.sDestroyTestAction.assertTestRun(DEFAULT_WAIT_TIMEOUT_MS);
    }

}
