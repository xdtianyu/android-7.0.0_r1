/*
 * Copyright (C) 2010 The Android Open Source Project
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

import com.android.compatibility.common.util.AbiUtils;
import com.android.cts.tradefed.UnitTests;
import com.android.cts.tradefed.build.StubCtsBuildHelper;
import com.android.cts.tradefed.result.PlanCreator;
import com.android.ddmlib.testrunner.TestIdentifier;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.targetprep.ITargetPreparer;
import com.android.tradefed.testtype.IRemoteTest;
import com.android.tradefed.util.xml.AbstractXmlParser.ParseException;

import junit.framework.TestCase;

import org.easymock.EasyMock;

import java.io.ByteArrayInputStream;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.InputStream;
import java.util.Arrays;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

/**
 * Unit tests for {@link CtsTest}.
 */
public class CtsTestTest extends TestCase {

    private static final String PLAN_NAME = "CTS";
    private static final String PACKAGE_NAME = "test-name";
    private static final String ID = AbiUtils.createId(UnitTests.ABI.getName(), PACKAGE_NAME);
    private static final TestIdentifier TEST_IDENTIFIER =
            new TestIdentifier("CLASS_NAME", "TEST_NAME");
    private static final List<String> NAMES = new ArrayList<>();
    private static final List<String> IDS = new ArrayList<>();
    private static final List<TestIdentifier> TEST_IDENTIFIER_LIST = new ArrayList<>();

    static {
        NAMES.add(PACKAGE_NAME);
        IDS.add(ID);
        TEST_IDENTIFIER_LIST.add(TEST_IDENTIFIER);
    }

    /** the test fixture under test, with all external dependencies mocked out */
    private CtsTest mCtsTest;
    private ITestPackageRepo mMockRepo;
    private ITestPlan mMockPlan;
    private ITestDevice mMockDevice;
    private ITestInvocationListener mMockListener;
    private StubCtsBuildHelper mStubBuildHelper;
    private ITestPackageDef mMockPackageDef;
    private Set<ITestPackageDef> mMockPackageDefs;
    private IRemoteTest mMockTest;

    /**
     * {@inheritDoc}
     */
    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mMockRepo = EasyMock.createMock(ITestPackageRepo.class);
        mMockPlan = EasyMock.createMock(ITestPlan.class);
        mMockDevice = EasyMock.createMock(ITestDevice.class);
        mMockListener = EasyMock.createNiceMock(ITestInvocationListener.class);
        mStubBuildHelper = new StubCtsBuildHelper();
        mMockPackageDefs = new HashSet<>();
        mMockPackageDef = EasyMock.createMock(ITestPackageDef.class);
        mMockPackageDefs.add(mMockPackageDef);
        EasyMock.expect(mMockPackageDef.getTargetApkName()).andStubReturn(null);
        EasyMock.expect(mMockPackageDef.getTargetPackageName()).andStubReturn(null);
        mMockTest = EasyMock.createMock(IRemoteTest.class);

        mCtsTest = new CtsTest() {
            @Override
            ITestPackageRepo createTestCaseRepo() {
                return mMockRepo;
            }

            @Override
            ITestPlan createPlan(String planName) {
                return mMockPlan;
            }

            @Override
            ITestPlan createPlan(PlanCreator planCreator) {
                return mMockPlan;
            }

            @Override
            InputStream createXmlStream(File xmlFile) throws FileNotFoundException {
                // return empty stream, not used
                return new ByteArrayInputStream(new byte[0]);
            }
        };
        mCtsTest.setDevice(mMockDevice);
        mCtsTest.setBuildHelper(mStubBuildHelper);
        // turn off device collection for simplicity
        mCtsTest.setSkipDeviceInfo(true);
        // only run tests on one ABI
        EasyMock.expect(mMockDevice.getProperty("ro.product.cpu.abilist"))
                .andReturn(UnitTests.ABI.getName()).anyTimes();
    }

    /**
     * Test normal case {@link CtsTest#run(ITestInvocationListener)} when running a plan.
     */
    @SuppressWarnings("unchecked")
    public void testRun_plan() throws DeviceNotAvailableException, ParseException {
        setParsePlanExpectations();

        setCreateAndRunTestExpectations();

        replayMocks();
        mCtsTest.run(mMockListener);
        verifyMocks();
    }

    /**
     * Test normal case {@link CtsTest#run(ITestInvocationListener)} when running a package.
     */
    @SuppressWarnings("unchecked")
    public void testRun_package() throws DeviceNotAvailableException {
        mCtsTest.addPackageName(PACKAGE_NAME);
        Map<String, List<ITestPackageDef>> nameMap = new HashMap<>();
        List<ITestPackageDef> testPackageDefList = new ArrayList<>();
        testPackageDefList.add(mMockPackageDef);
        nameMap.put(PACKAGE_NAME, testPackageDefList);

        EasyMock.expect(mMockRepo.getTestPackageDefsByName()).andReturn(nameMap);

        setCreateAndRunTestExpectations();

        replayMocks();
        mCtsTest.run(mMockListener);
        verifyMocks();
    }

    /**
     * Test a resumed run
     */
    @SuppressWarnings("unchecked")
    public void testRun_resume() throws DeviceNotAvailableException {
        mCtsTest.addPackageName(PACKAGE_NAME);
        Map<String, List<ITestPackageDef>> nameMap = new HashMap<>();
        List<ITestPackageDef> testPackageDefList = new ArrayList<>();
        testPackageDefList.add(mMockPackageDef);
        nameMap.put(PACKAGE_NAME, testPackageDefList);

        EasyMock.expect(mMockRepo.getTestPackageDefsByName()).andReturn(nameMap);
        setCreateAndRunTestExpectations();
        // abort the first run
        EasyMock.expectLastCall().andThrow(new DeviceNotAvailableException());

        // now expect test to be resumed
        mMockTest.run((ITestInvocationListener)EasyMock.anyObject());

        replayMocks();
        try {
            mCtsTest.run(mMockListener);
            fail("Did not throw DeviceNotAvailableException");
        } catch (DeviceNotAvailableException e) {
            // expected
        }
        // now resume, and expect same test's run method to be called again
        mCtsTest.run(mMockListener);
        verifyMocks();
    }

    /**
     * Test normal case {@link CtsTest#run(ITestInvocationListener)} when running a class.
     */
    @SuppressWarnings("unchecked")
    public void testRun_class() throws DeviceNotAvailableException {
        final String className = "className";
        final String methodName = "methodName";
        mCtsTest.setClassName(className);
        mCtsTest.setMethodName(methodName);

        EasyMock.expect(mMockRepo.findPackageIdsForTest(className)).andReturn(IDS);
        mMockPackageDef.setClassName(className, methodName);

        setCreateAndRunTestExpectations();

        replayMocks();
        mCtsTest.run(mMockListener);
        verifyMocks();
    }

    /**
     * Test normal case {@link CtsTest#run(ITestInvocationListener)} when running a class.
     */
    @SuppressWarnings("unchecked")
    public void testRun_test() throws DeviceNotAvailableException {
        final String className = "className";
        final String methodName = "methodName";
        final String testName = String.format("%s#%s", className, methodName);
        mCtsTest.setTestName(testName);

        EasyMock.expect(mMockRepo.findPackageIdsForTest(className)).andReturn(IDS);
        mMockPackageDef.setClassName(className, methodName);

        setCreateAndRunTestExpectations();

        replayMocks();
        mCtsTest.run(mMockListener);
        verifyMocks();
    }

    /**
     * Test {@link CtsTest#run(ITestInvocationListener)} when --excluded-package is specified
     */
    public void testRun_excludedPackage() throws DeviceNotAvailableException, ParseException {
        mCtsTest.setPlanName(PLAN_NAME);
        mMockPlan.parse((InputStream) EasyMock.anyObject());
        EasyMock.expect(mMockPlan.getTestIds()).andReturn(IDS);

        mCtsTest.addExcludedPackageName(PACKAGE_NAME);

        // PACKAGE_NAME would normally be run, but it has been excluded. Expect nothing to happen
        replayMocks();
        mCtsTest.run(mMockListener);
        verifyMocks();
    }

    /**
     * Test {@link CtsTest#run(ITestInvocationListener)} when --continue-session is specified
     */
    public void testRun_continueSession() throws DeviceNotAvailableException {
        mCtsTest.setContinueSessionId(1);
        EasyMock.expect(mMockPlan.getTestIds()).andReturn(IDS);
        TestFilter filter = new TestFilter();
        EasyMock.expect(mMockPlan.getTestFilter(ID)).andReturn(filter);

        mMockPackageDef.setTestFilter(filter);

        setCreateAndRunTestExpectations();

        replayMocks();
        mCtsTest.run(mMockListener);
        verifyMocks();
    }

    /**
     * Set EasyMock expectations for parsing {@link #PLAN_NAME}
     */
    private void setParsePlanExpectations() throws ParseException {
        mCtsTest.setPlanName(PLAN_NAME);
        mMockPlan.parse((InputStream) EasyMock.anyObject());
        EasyMock.expect(mMockPlan.getTestIds()).andReturn(IDS);
        TestFilter filter = new TestFilter();
        EasyMock.expect(mMockPlan.getTestFilter(ID)).andReturn(filter);
        mMockPackageDef.setTestFilter(filter);
    }

    /**
     * Set EasyMock expectations for creating and running a package with PACKAGE_NAME
     */
    private void setCreateAndRunTestExpectations() throws DeviceNotAvailableException {
        EasyMock.expect(mMockRepo.getPackageNames()).andReturn(NAMES).anyTimes();
        EasyMock.expect(mMockRepo.getPackageIds()).andReturn(IDS).anyTimes();
        EasyMock.expect(mMockRepo.getTestPackage(ID)).andReturn(mMockPackageDef).anyTimes();
        EasyMock.expect(mMockPackageDef.createTest((File) EasyMock.anyObject())).andReturn(mMockTest);
        EasyMock.expect(mMockPackageDef.getTests()).andReturn(TEST_IDENTIFIER_LIST).times(2);
        EasyMock.expect(mMockPackageDef.getName()).andReturn(PACKAGE_NAME).atLeastOnce();
        EasyMock.expect(mMockPackageDef.getAbi()).andReturn(UnitTests.ABI).atLeastOnce();
        EasyMock.expect(mMockPackageDef.getId()).andReturn(ID).atLeastOnce();
        EasyMock.expect(mMockPackageDef.getDigest()).andReturn("digest").atLeastOnce();
        EasyMock.expect(mMockPackageDef.getPackagePreparers()).andReturn(
                    new ArrayList<ITargetPreparer>()).atLeastOnce();
        mMockTest.run((ITestInvocationListener) EasyMock.anyObject());
    }

    /**
     * Test {@link CtsTest#run(ITestInvocationListener)} when --plan and --package options have not
     * been specified
     */
    public void testRun_nothingToRun() throws DeviceNotAvailableException {
        replayMocks();
        try {
            mCtsTest.run(mMockListener);
            fail("IllegalArgumentException not thrown");
        } catch (IllegalArgumentException e) {
            // expected
        }
    }

    /**
     * Test {@link CtsTest#run(ITestInvocationListener)} when --plan and --package options have
     * been specified.
     */
    public void testRun_packagePlan() throws DeviceNotAvailableException {
        mCtsTest.setPlanName(PLAN_NAME);
        mCtsTest.addPackageName(PACKAGE_NAME);
        replayMocks();
        try {
            mCtsTest.run(mMockListener);
            fail("IllegalArgumentException not thrown");
        } catch (IllegalArgumentException e) {
            // expected
        }
    }

    /**
     * Test {@link CtsTest#run(ITestInvocationListener)} when --plan and --class options have been
     * specified
     */
    public void testRun_planClass() throws DeviceNotAvailableException {
        mCtsTest.setPlanName(PLAN_NAME);
        mCtsTest.setClassName("class");
        replayMocks();
        try {
            mCtsTest.run(mMockListener);
            fail("IllegalArgumentException not thrown");
        } catch (IllegalArgumentException e) {
            // expected
        }
    }

    /**
     * Test {@link CtsTest#run(ITestInvocationListener)} when --package and --class options have
     * been specified
     */
    public void testRun_packageClass() throws DeviceNotAvailableException {
        mCtsTest.addPackageName(PACKAGE_NAME);
        mCtsTest.setClassName("class");
        replayMocks();
        try {
            mCtsTest.run(mMockListener);
            fail("IllegalArgumentException not thrown");
        } catch (IllegalArgumentException e) {
            // expected
        }
    }

    /**
     * Test {@link CtsTest#run(ITestInvocationListener)} when --plan, --package and --class options
     * have been specified
     */
    public void testRun_planPackageClass() throws DeviceNotAvailableException {
        mCtsTest.setPlanName(PLAN_NAME);
        mCtsTest.addPackageName(PACKAGE_NAME);
        mCtsTest.setClassName("class");
        replayMocks();
        try {
            mCtsTest.run(mMockListener);
            fail("IllegalArgumentException not thrown");
        } catch (IllegalArgumentException e) {
            // expected
        }
    }

    /**
     * Test {@link CtsTest#run(ITestInvocationListener)} when --plan, --continue-option options
     * have been specified
     */
    public void testRun_planContinue() throws DeviceNotAvailableException {
        mCtsTest.setPlanName(PLAN_NAME);
        mCtsTest.setContinueSessionId(1);
        replayMocks();
        try {
            mCtsTest.run(mMockListener);
            fail("IllegalArgumentException not thrown");
        } catch (IllegalArgumentException e) {
            // expected
        }
    }

    /**
     * Test {@link CtsTestTest#join} works.
     * @throws DeviceNotAvailableException
     */
    public void testJoin() throws DeviceNotAvailableException {
        String expected = "a@b@c";
        String actual = mCtsTest.join(new ArrayList<String>(Arrays.asList("a", "b", "c")), "@");
        assertEquals(expected, actual);
    }

    /**
     * Test {@link CtsTestTest#join} for a single element list.
     * @throws DeviceNotAvailableException
     */
    public void testSingleJoin() throws DeviceNotAvailableException {
        String actual = mCtsTest.join(new ArrayList<String>(Arrays.asList("foo")), "@");
        assertEquals("foo", actual);
    }

    /**
     * Test {@link CtsTestTest#join} for an empty list.
     * @throws DeviceNotAvailableException
     */
    public void testEmptyJoin() throws DeviceNotAvailableException {
        String actual = mCtsTest.join(new ArrayList<String>(), "@");
        assertEquals("", actual);
    }

    private void replayMocks(Object... mocks) {
        EasyMock.replay(mMockRepo, mMockPlan, mMockDevice, mMockPackageDef, mMockListener, mMockTest);
        EasyMock.replay(mocks);
    }

    private void verifyMocks(Object... mocks) {
        EasyMock.verify(mMockRepo, mMockPlan, mMockDevice, mMockPackageDef, mMockListener, mMockTest);
        EasyMock.verify(mocks);
    }
}
