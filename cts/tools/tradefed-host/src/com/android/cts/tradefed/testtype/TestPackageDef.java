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
import com.android.ddmlib.testrunner.TestIdentifier;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.targetprep.ITargetPreparer;
import com.android.tradefed.testtype.IAbi;
import com.android.tradefed.testtype.IRemoteTest;
import com.android.tradefed.testtype.InstrumentationTest;
import com.android.tradefed.util.StreamUtil;

import java.io.BufferedInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;
import java.security.DigestInputStream;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.Collection;
import java.util.LinkedHashMap;
import java.util.LinkedHashSet;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;

/**
 * Container for CTS test info.
 * <p/>
 * Knows how to translate this info into a runnable {@link IRemoteTest}.
 */
class TestPackageDef implements ITestPackageDef {

    public static final String HOST_SIDE_ONLY_TEST = "hostSideOnly";
    public static final String NATIVE_TEST = "native";
    public static final String WRAPPED_NATIVE_TEST = "wrappednative";
    public static final String VM_HOST_TEST = "vmHostTest";
    public static final String DEQP_TEST = "deqpTest";
    public static final String JUNIT_DEVICE_TEST = "jUnitDeviceTest";
    public static final String TESTNG_DEVICE_TEST = "testNGDeviceTest";

    private String mAppPackageName = null;
    private String mAppNameSpace = null;
    private String mName = null;
    private String mRunner = null;
    private String mTestType = null;
    private String mJarPath = null;
    private String mRunTimeArgs = null;
    private String mTestPackageName = null;
    private String mDigest = null;
    private IAbi mAbi = null;
    private List<ITargetPreparer> mPreparers = null;

    // use a LinkedHashSet for predictable iteration insertion-order, and fast
    // lookups
    private Collection<TestIdentifier> mTests = new LinkedHashSet<TestIdentifier>();
    // also maintain an index of known test classes
    private Collection<String> mTestClasses = new LinkedHashSet<String>();
    // store instance arguments in order too for consistency
    private Map<TestIdentifier, List<Map<String, String>>> mTestInstanceArguments =
            new LinkedHashMap<>();

    // dynamic options, not parsed from package xml
    private String mClassName;
    private String mMethodName;
    private TestFilter mTestFilter = new TestFilter();
    private String mTargetBinaryName;
    private String mTargetNameSpace;
    // only timeout per package is supported. To change this to method granularity,
    // test invocation should be done in method level.
    // So for now, only max timeout for the package is used.
    private int mTimeoutInMins = -1;

    @Override
    public IAbi getAbi() {
        return mAbi;
    }

    /**
     * @param abi the ABI to run this package on
     */
    public void setAbi(IAbi abi) {
        mAbi = abi;
    }

    /**
     * @return unique id representing this test package for this ABI.
     */
    @Override
    public String getId() {
        return AbiUtils.createId(getAbi().getName(), getAppPackageName());
    }

    void setAppPackageName(String appPackageName) {
        mAppPackageName = appPackageName;
    }

    String getAppPackageName() {
        return mAppPackageName;
    }

    void setRunTimeArgs(String runTimeArgs) {
        mRunTimeArgs = runTimeArgs;
    }

    void setAppNameSpace(String appNameSpace) {
        mAppNameSpace = appNameSpace;
    }

    String getAppNameSpace() {
        return mAppNameSpace;
    }

    void setName(String name) {
        mName = name;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String getName() {
        return mName;
    }

    void setRunner(String runnerName) {
        mRunner = runnerName;
    }

    String getRunner() {
        return mRunner;
    }

    void setTestType(String testType) {
        mTestType = testType;
    }

    String getTestType() {
        return mTestType;
    }

    void setJarPath(String jarPath) {
        mJarPath = jarPath;
    }

    String getJarPath() {
        return mJarPath;
    }

    void setTestPackageName(String testPackageName) {
        mTestPackageName = testPackageName;
    }

    void setTargetBinaryName(String targetBinaryName) {
        mTargetBinaryName = targetBinaryName;
    }

    void setTargetNameSpace(String targetNameSpace) {
        mTargetNameSpace = targetNameSpace;
    }

    @Override
    public String getTargetApkName() {
       if (mTargetBinaryName != null && !mTargetBinaryName.isEmpty()) {
           return String.format("%s.apk", mTargetBinaryName);
       }
       return null;
    }

    @Override
    public String getTargetPackageName() {
        if (mTargetNameSpace != null && mTargetNameSpace.isEmpty()) {
            return null;
        }
        return mTargetNameSpace;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setTestFilter(TestFilter testFilter) {
        mTestFilter = testFilter;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setClassName(String className, String methodName) {
        mClassName = className;
        mMethodName = methodName;
    }

    /**
     * Setter for injecting a list of {@link ITargetPreparer}s as configured in module test config.
     * @param preparers
     */
    void setPackagePreparers(List<ITargetPreparer> preparers) {
        mPreparers = preparers;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public List<ITargetPreparer> getPackagePreparers() {
        return mPreparers;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public IRemoteTest createTest(File testCaseDir) {
        mTestFilter.setTestInclusion(mClassName, mMethodName);
        mTests = filterTests();

        if (HOST_SIDE_ONLY_TEST.equals(mTestType)) {
            CLog.d("Creating host test for %s", mName);
            JarHostTest hostTest = new JarHostTest();
            if (mTimeoutInMins >= 0) {
                CLog.d("Setting new timeout to " + mTimeoutInMins + " mins");
                hostTest.setTimeout(mTimeoutInMins * 60 * 1000);
            }
            hostTest.setRunName(getId());
            hostTest.setJarFileName(mJarPath);
            hostTest.setTests(mTests);
            hostTest.setAbi(mAbi);
            mDigest = generateDigest(testCaseDir, mJarPath);
            return hostTest;
        } else if (VM_HOST_TEST.equals(mTestType)) {
            CLog.d("Creating vm host test for %s", mName);
            VMHostTest vmHostTest = new VMHostTest();
            vmHostTest.setRunName(getId());
            vmHostTest.setJarFileName(mJarPath);
            vmHostTest.setTests(mTests);
            vmHostTest.setAbi(mAbi);
            mDigest = generateDigest(testCaseDir, mJarPath);
            return vmHostTest;
        } else if (DEQP_TEST.equals(mTestType)) {
            DeqpTestRunner deqpTest =
                    new DeqpTestRunner(mAppPackageName, mName, mTests, mTestInstanceArguments);
            deqpTest.setAbi(mAbi);
            return deqpTest;
        } else if (NATIVE_TEST.equals(mTestType)) {
            GeeTest geeTest = new GeeTest(mAppPackageName, mName);
            geeTest.setAbi(mAbi);
            return geeTest;
        } else if (WRAPPED_NATIVE_TEST.equals(mTestType)) {
            CLog.d("Creating new wrapped native test for %s", mName);
            WrappedGTest wrappedGeeTest = new WrappedGTest(mAppNameSpace, mAppPackageName, mName, mRunner);
            wrappedGeeTest.setAbi(mAbi);
            return wrappedGeeTest;
        } else if (JUNIT_DEVICE_TEST.equals(mTestType)){
            CLog.d("Creating JUnit device test %s", mName);
            JUnitDeviceTest jUnitDeviceTest = new JUnitDeviceTest();
            jUnitDeviceTest.setRunName(getId());
            jUnitDeviceTest.addTestJarFileName(mJarPath);
            jUnitDeviceTest.addRunTimeArgs(mRunTimeArgs);
            jUnitDeviceTest.setTests(mTests);
            jUnitDeviceTest.setAbi(mAbi);
            mDigest = generateDigest(testCaseDir, mJarPath);
            return jUnitDeviceTest;
        } else if (TESTNG_DEVICE_TEST.equals(mTestType)){
            CLog.d("Creating TestNG device test %s", mName);
            TestNGDeviceTest testNGDeviceTest = new TestNGDeviceTest();
            testNGDeviceTest.setRunName(getId());
            testNGDeviceTest.addTestJarFileName(mJarPath);
            testNGDeviceTest.addRunTimeArgs(mRunTimeArgs);
            testNGDeviceTest.setTests(mTests);
            testNGDeviceTest.setAbi(mAbi);
            mDigest = generateDigest(testCaseDir, mJarPath);
            return testNGDeviceTest;
        }
        else {
            CLog.d("Creating instrumentation test for %s", mName);
            CtsInstrumentationApkTest instrTest = new CtsInstrumentationApkTest();
            if (mTimeoutInMins >= 0) {
                // as timeout cannot be set for each test,
                // increase the time-out of the whole package
                CLog.d("Setting new timeout to " + mTimeoutInMins + " mins");
                instrTest.setTestTimeout(mTimeoutInMins * 60 * 1000);
            }
            return setInstrumentationTest(instrTest, testCaseDir);
        }
    }

    /**
     * Populates given {@link CtsInstrumentationApkTest} with data from the package xml.
     *
     * @param testCaseDir
     * @param instrTest
     * @return the populated {@link InstrumentationTest} or <code>null</code>
     */
    private InstrumentationTest setInstrumentationTest(CtsInstrumentationApkTest instrTest,
            File testCaseDir) {
        instrTest.setRunName(getId());
        instrTest.setPackageName(mAppNameSpace);
        instrTest.setRunnerName(mRunner);
        instrTest.setAbi(mAbi);
        instrTest.setTestsToRun(mTests, false
            /* force batch mode off to always run using testFile */);
        instrTest.setReRunUsingTestFile(true);
        // mName means 'apk file name' for instrumentation tests
        instrTest.addInstallApk(String.format("%s.apk", mName), mAppNameSpace);
        mDigest = generateDigest(testCaseDir, String.format("%s.apk", mName));
        return instrTest;
    }

    /**
     * Filter the tests to run based on list of included/excluded tests, class and method name.
     *
     * @return the filtered collection of tests
     */
    private Collection<TestIdentifier> filterTests() {
        mTestFilter.setTestInclusion(mClassName, mMethodName);
        return mTestFilter.filter(mTests);
    }

    boolean isKnownTestClass(String className) {
        return mTestClasses.contains(className);
    }

    /**
     * Add a {@link TestIdentifier} to the list of tests in this package.
     *
     * @param testDef
     * @param timeout in mins
     */
    void addTest(TestIdentifier testDef, int timeout) {
        mTests.add(testDef);
        mTestClasses.add(testDef.getClassName());
        mTestInstanceArguments.put(testDef, new LinkedList<Map<String, String>>());
        // 0 means no timeout, so keep 0 if already is.
        if ((timeout > mTimeoutInMins) && (mTimeoutInMins != 0)) {
            mTimeoutInMins = timeout;
        }
    }

    /**
     * Add a test instance to an existing {@link TestIdentifier}.
     */
    void addTestInstance(TestIdentifier testDef, Map<String, String> instanceArguments) {
        if (!mTestInstanceArguments.containsKey(testDef)) {
            throw new IllegalStateException("test id does not name an existing test");
        }
        mTestInstanceArguments.get(testDef).add(instanceArguments);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public Collection<TestIdentifier> getTests() {
        return mTests;
    }

    /**
     * Get the instance argument map for tests.
     * <p/>
     * Exposed for unit testing.
     */
    public Map<TestIdentifier, List<Map<String, String>>> getTestInstanceArguments() {
        return mTestInstanceArguments;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String getDigest() {
        return mDigest;
    }

    /**
     * Generate a sha1sum digest for a file.
     * <p/>
     * Exposed for unit testing.
     *
     * @param fileDir the directory of the file
     * @param fileName the name of the file
     * @return a hex {@link String} of the digest
     */
    String generateDigest(File fileDir, String fileName) {
        final String algorithm = "SHA-1";
        InputStream fileStream = null;
        DigestInputStream d = null;
        try {
            fileStream = getFileStream(fileDir, fileName);
            MessageDigest md = MessageDigest.getInstance(algorithm);
            d = new DigestInputStream(fileStream, md);
            byte[] buffer = new byte[8196];
            while (d.read(buffer) != -1) {
            }
            return toHexString(md.digest());
        } catch (NoSuchAlgorithmException e) {
            return algorithm + " not found";
        } catch (IOException e) {
            CLog.e(e);
        } finally {
            StreamUtil.close(d);
            StreamUtil.close(fileStream);
        }
        return "failed to generate digest";
    }

    /**
     * Retrieve an input stream for given file
     * <p/>
     * Exposed so unit tests can mock.
     */
    InputStream getFileStream(File fileDir, String fileName) throws FileNotFoundException {
        InputStream fileStream;
        fileStream = new BufferedInputStream(new FileInputStream(new File(fileDir, fileName)));
        return fileStream;
    }

    /**
     * Convert the given byte array into a lowercase hex string.
     *
     * @param arr The array to convert.
     * @return The hex encoded string.
     */
    private String toHexString(byte[] arr) {
        StringBuilder buf = new StringBuilder(arr.length * 2);
        for (byte b : arr) {
            buf.append(String.format("%02x", b & 0xFF));
        }
        return buf.toString();
    }

    @Override
    public int compareTo(ITestPackageDef testPackageDef) {
        return getId().compareTo(testPackageDef.getId());
    }
}
