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

package com.android.compatibility.common.tradefed.result;

import com.android.compatibility.common.tradefed.build.CompatibilityBuildHelper;
import com.android.compatibility.common.util.AbiUtils;
import com.android.compatibility.common.util.ICaseResult;
import com.android.compatibility.common.util.IInvocationResult;
import com.android.compatibility.common.util.IModuleResult;
import com.android.compatibility.common.util.ITestResult;
import com.android.compatibility.common.util.TestStatus;
import com.android.ddmlib.testrunner.TestIdentifier;
import com.android.tradefed.build.BuildInfo;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.config.OptionSetter;
import com.android.tradefed.util.FileUtil;

import junit.framework.TestCase;

import java.io.File;
import java.io.FileFilter;
import java.util.HashMap;
import java.util.List;

public class ConsoleReporterTest extends TestCase {

    private static final String TESTCASES = "testcases";
    private static final String NAME = "ModuleName";
    private static final String NAME2 = "ModuleName2";
    private static final String ABI = "mips64";
    private static final String ID = AbiUtils.createId(ABI, NAME);
    private static final String ID2 = AbiUtils.createId(ABI, NAME2);
    private static final String CLASS = "android.test.FoorBar";
    private static final String METHOD_1 = "testBlah1";
    private static final String METHOD_2 = "testBlah2";
    private static final String METHOD_3 = "testBlah3";
    private static final String TEST_1 = String.format("%s#%s", CLASS, METHOD_1);
    private static final String TEST_2 = String.format("%s#%s", CLASS, METHOD_2);
    private static final String TEST_3 = String.format("%s#%s", CLASS, METHOD_3);
    private static final String STACK_TRACE = "Something small is not alright\n " +
            "at four.big.insects.Marley.sing(Marley.java:10)";

    private ConsoleReporter mReporter;
    private IBuildInfo mBuildInfo;
    private CompatibilityBuildHelper mBuildHelper;

    private File mRoot = null;
    private File mBase = null;
    private File mTests = null;

    @Override
    public void setUp() throws Exception {
        mReporter = new ConsoleReporter();
        OptionSetter setter = new OptionSetter(mReporter);
        setter.setOptionValue("quiet-output", "true");
    }

    @Override
    public void tearDown() throws Exception {
        mReporter = null;
    }

    public void testResultReporting_singleModule() throws Exception {
        mReporter.invocationStarted(mBuildInfo);
        mReporter.testRunStarted(ID, 3);
        runTests();

        mReporter.testRunEnded(10, new HashMap<String, String>());
        mReporter.invocationEnded(10);

        assertEquals(ID, mReporter.getModuleId());
        assertEquals(2, mReporter.getFailedTests());
        assertEquals(1, mReporter.getPassedTests());
        assertEquals(3, mReporter.getCurrentTestNum());
        assertEquals(3, mReporter.getTotalTestsInModule());
    }

    public void testResultReporting_multipleModules() throws Exception {
        mReporter.invocationStarted(mBuildInfo);
        mReporter.testRunStarted(ID, 3);
        runTests();

        assertEquals(ID, mReporter.getModuleId());
        assertEquals(2, mReporter.getFailedTests());
        assertEquals(1, mReporter.getPassedTests());
        assertEquals(3, mReporter.getCurrentTestNum());
        assertEquals(3, mReporter.getTotalTestsInModule());

        // Should reset counters
        mReporter.testRunStarted(ID2, 3);
        assertEquals(ID2, mReporter.getModuleId());
        assertEquals(0, mReporter.getFailedTests());
        assertEquals(0, mReporter.getPassedTests());
        assertEquals(0, mReporter.getCurrentTestNum());
        assertEquals(3, mReporter.getTotalTestsInModule());

        runTests();
        // Same id, should not reset test counters, but aggregate total tests
        mReporter.testRunStarted(ID2, 5);
        assertEquals(ID2, mReporter.getModuleId());
        assertEquals(2, mReporter.getFailedTests());
        assertEquals(1, mReporter.getPassedTests());
        assertEquals(3, mReporter.getCurrentTestNum());
        assertEquals(8, mReporter.getTotalTestsInModule());
    }

    /** Run 4 test, but one is ignored */
    private void runTests() {
        TestIdentifier test1 = new TestIdentifier(CLASS, METHOD_1);
        mReporter.testStarted(test1);
        mReporter.testEnded(test1, new HashMap<String, String>());
        assertFalse(mReporter.getTestFailed());

        TestIdentifier test2 = new TestIdentifier(CLASS, METHOD_2);
        mReporter.testStarted(test2);
        assertFalse(mReporter.getTestFailed());
        mReporter.testFailed(test2, STACK_TRACE);
        assertTrue(mReporter.getTestFailed());

        TestIdentifier test3 = new TestIdentifier(CLASS, METHOD_3);
        mReporter.testStarted(test3);
        assertFalse(mReporter.getTestFailed());
        mReporter.testFailed(test3, STACK_TRACE);
        assertTrue(mReporter.getTestFailed());

        TestIdentifier test4 = new TestIdentifier(CLASS, METHOD_3);
        mReporter.testStarted(test4);
        assertFalse(mReporter.getTestFailed());
        mReporter.testIgnored(test4);
        assertFalse(mReporter.getTestFailed());
    }
}
