/*
 * Copyright (C) 2011 The Android Open Source Project
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
package com.android.cts.tradefed.result;

import com.android.compatibility.common.util.AbiUtils;
import com.android.compatibility.common.util.MetricsStore;
import com.android.compatibility.common.util.ReportLog;
import com.android.cts.tradefed.testtype.CtsTest;
import com.android.ddmlib.testrunner.TestIdentifier;
import com.android.tradefed.log.LogUtil.CLog;

import org.kxml2.io.KXmlSerializer;
import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserException;

import java.io.IOException;
import java.util.Collection;
import java.util.Collections;
import java.util.Deque;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;

/**
 * Data structure for a CTS test package result.
 * <p/>
 * Provides methods to serialize to XML.
 */
class TestPackageResult extends AbstractXmlPullParser {

    static final String TAG = "TestPackage";

    public static final String RESULT_KEY = "COMPATIBILITY_TEST_RESULT";

    private static final String DIGEST_ATTR = "digest";
    private static final String APP_PACKAGE_NAME_ATTR = "appPackageName";
    private static final String NAME_ATTR = "name";
    private static final String ABI_ATTR = "abi";
    private static final String ns = CtsXmlResultReporter.ns;
    private static final String SIGNATURE_TEST_PKG = "android.tests.sigtest";

    private String mDeviceSerial;
    private String mAppPackageName;
    private String mName;
    private String mAbi;
    private String mDigest;

    private Map<String, String> mMetrics = new HashMap<String, String>();
    private Map<TestIdentifier, Map<String, String>> mTestMetrics = new HashMap<TestIdentifier, Map<String, String>>();

    private TestSuite mSuiteRoot = new TestSuite(null);

    public void setDeviceSerial(String deviceSerial) {
        mDeviceSerial = deviceSerial;
    }

    public String getDeviceSerial() {
        return mDeviceSerial;
    }

    public String getId() {
        return AbiUtils.createId(getAbi(), getAppPackageName());
    }

    public void setAppPackageName(String appPackageName) {
        mAppPackageName = appPackageName;
    }

    public String getAppPackageName() {
        return mAppPackageName;
    }

    public void setName(String name) {
        mName = name;
    }

    public String getName() {
        return mName;
    }

    public void setAbi(String abi) {
        mAbi = abi;
    }

    public String getAbi() {
        return mAbi;
    }

    public void setDigest(String digest) {
        mDigest = digest;
    }

    public String getDigest() {
        return mDigest;
    }

    /**
     * Return the {@link TestSuite}s
     */
    public Collection<TestSuite> getTestSuites() {
        return mSuiteRoot.getTestSuites();
    }

    /**
     * Adds a test result to this test package
     *
     * @param testId
     */
    public Test insertTest(TestIdentifier testId) {
        return findTest(testId, true);
    }

    private Test findTest(TestIdentifier testId, boolean insertIfMissing) {
        List<String> classNameSegments = new LinkedList<String>();
        Collections.addAll(classNameSegments, testId.getClassName().split("\\."));
        if (classNameSegments.size() <= 0) {
            CLog.e("Unrecognized package name format for test class '%s'",
                    testId.getClassName());
            // should never happen
            classNameSegments.add("UnknownTestClass");
        }
        String testCaseName = classNameSegments.remove(classNameSegments.size() - 1);
        return mSuiteRoot.findTest(classNameSegments, testCaseName, testId.getTestName(), insertIfMissing);
    }


    /**
     * Find the test result for given {@link TestIdentifier}.
     * @param testId
     * @return the {@link Test} or <code>null</code>
     */
    public Test findTest(TestIdentifier testId) {
        return findTest(testId, false);
    }

    /**
     * Serialize this object and all its contents to XML.
     *
     * @param serializer
     * @throws IOException
     */
    public void serialize(KXmlSerializer serializer) throws IOException {
        serializer.startTag(ns, TAG);
        serializeAttribute(serializer, NAME_ATTR, mName);
        serializeAttribute(serializer, APP_PACKAGE_NAME_ATTR, mAppPackageName);
        serializeAttribute(serializer, ABI_ATTR, mAbi);
        serializeAttribute(serializer, DIGEST_ATTR, getDigest());
        if (SIGNATURE_TEST_PKG.equals(mName)) {
            serializer.attribute(ns, "signatureCheck", "true");
        }
        mSuiteRoot.serialize(serializer);
        serializer.endTag(ns, TAG);
    }

    /**
     * Helper method to serialize attributes.
     * Can handle null values. Useful for cases where test package has not been fully populated
     * such as when unit testing.
     *
     * @param attrName
     * @param attrValue
     * @throws IOException
     */
    private void serializeAttribute(KXmlSerializer serializer, String attrName, String attrValue)
            throws IOException {
        attrValue = attrValue == null ? "" : attrValue;
        serializer.attribute(ns, attrName, attrValue);
    }

    /**
     * Populates this class with package result data parsed from XML.
     *
     * @param parser the {@link XmlPullParser}. Expected to be pointing at start
     *            of TestPackage tag
     */
    @Override
    void parse(XmlPullParser parser) throws XmlPullParserException, IOException {
        if (!parser.getName().equals(TAG)) {
            throw new XmlPullParserException(String.format(
                    "invalid XML: Expected %s tag but received %s", TAG, parser.getName()));
        }
        setAppPackageName(getAttribute(parser, APP_PACKAGE_NAME_ATTR));
        setName(getAttribute(parser, NAME_ATTR));
        setAbi(getAttribute(parser, ABI_ATTR));
        setDigest(getAttribute(parser, DIGEST_ATTR));
        int eventType = parser.getEventType();
        while (eventType != XmlPullParser.END_DOCUMENT) {
            if (eventType == XmlPullParser.START_TAG && parser.getName().equals(TestSuite.TAG)) {
                TestSuite suite = new TestSuite();
                suite.parse(parser);
                mSuiteRoot.insertSuite(suite);
            }
            if (eventType == XmlPullParser.END_TAG && parser.getName().equals(TAG)) {
                return;
            }
            eventType = parser.next();
        }
    }

    /**
     * Return a list of {@link TestIdentifier}s contained in this result with the given status
     *
     * @param resultFilter the {@link CtsTestStatus} to filter by
     * @return a collection of {@link TestIdentifier}s
     */
    public Collection<TestIdentifier> getTestsWithStatus(CtsTestStatus resultFilter) {
        Collection<TestIdentifier> tests = new LinkedList<TestIdentifier>();
        Deque<String> suiteNames = new LinkedList<String>();
        mSuiteRoot.addTestsWithStatus(tests, suiteNames, resultFilter);
        return tests;
    }

    /**
     * Populate values in this package result from run metrics
     * @param metrics A map of metrics from the completed test run.
     */
    public void populateMetrics(Map<String, String> metrics) {
        String name = metrics.get(CtsTest.PACKAGE_NAME_METRIC);
        if (name != null) {
            setName(name);
        }
        String abi = metrics.get(CtsTest.PACKAGE_ABI_METRIC);
        if (abi != null) {
            setAbi(abi);
        }
        String digest = metrics.get(CtsTest.PACKAGE_DIGEST_METRIC);
        if (digest != null) {
            setDigest(digest);
        }
        mMetrics.putAll(metrics);

        // Collect performance results
        for (TestIdentifier test : mTestMetrics.keySet()) {
            // device test can have performance results in test metrics
            String perfResult = mTestMetrics.get(test).get(RESULT_KEY);
            ReportLog report = null;
            if (perfResult != null) {
                try {
                    report = ReportLog.parse(perfResult);
                } catch (XmlPullParserException | IOException e) {
                    e.printStackTrace();
                }
            }
            Test result = findTest(test);
            if (report != null && !result.getResult().equals(CtsTestStatus.FAIL)) {
                result.setResultStatus(CtsTestStatus.PASS);
                result.setReportLog(report);
            }
        }
    }

    /**
     * Report the given test as a failure.
     *
     * @param test
     * @param status
     * @param trace
     */
    public void reportTestFailure(TestIdentifier test, CtsTestStatus status, String trace) {
        Test result = findTest(test);
        result.setResultStatus(status);
        result.setStackTrace(trace);
    }

    /**
     * Report that the given test has completed.
     *
     * @param test The {@link TestIdentifier} of the completed test.
     * @param testMetrics A map holding metrics about the completed test, if any.
     */
    public void reportTestEnded(TestIdentifier test, Map<String, String> testMetrics) {
        Test result = findTest(test);
        if (!result.getResult().equals(CtsTestStatus.FAIL)) {
            result.setResultStatus(CtsTestStatus.PASS);
        }
        result.updateEndTime();
        if (mTestMetrics.containsKey(test)) {
            CLog.e("Test metrics already contains key: " + test);
        }
        mTestMetrics.put(test, testMetrics);
        CLog.i("Test metrics:" + testMetrics);
    }

    /**
     * Return the number of tests with given status
     *
     * @param status
     * @return the total number of tests with given status
     */
    public int countTests(CtsTestStatus status) {
        return mSuiteRoot.countTests(status);
    }

    /**
     * @return A map holding the metrics from the test run.
     */
    public Map<String, String> getMetrics() {
        return mMetrics;
    }

}
