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
import com.android.tradefed.util.xml.AbstractXmlParser.ParseException;

import junit.framework.TestCase;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Iterator;
import java.util.List;
import java.util.Set;

/**
 * Unit tests for {@link TestPlan}.
 */
public class TestPlanTest extends TestCase {

    private static final String TEST_NAME1 = "foo";
    private static final String TEST_NAME2 = "foo2";
    private static final String EXCLUDE_TEST_CLASS = "com.example.FooTest";
    private static final String EXCLUDE_TEST_METHOD = "testFoo";
    private static final String EXCLUDE_TEST_METHOD2 = "testFoo2";

    static final String EMPTY_DATA = "<TestPlan version=\"1.0\" />";

    static final String TEST_DATA =
        "<TestPlan version=\"1.0\">" +
            String.format("<Entry name=\"%s\" />", TEST_NAME1) +
            String.format("<Entry name=\"%s\" />", TEST_NAME2) +
        "</TestPlan>";

    static final String TEST_EXCLUDED_DATA =
        "<TestPlan version=\"1.0\">" +
            String.format("<Entry name=\"%s\" exclude=\"%s#%s\" />", TEST_NAME1, EXCLUDE_TEST_CLASS,
                    EXCLUDE_TEST_METHOD) +
        "</TestPlan>";

    static final String TEST_MULTI_EXCLUDED_DATA =
        "<TestPlan version=\"1.0\">" +
            String.format("<Entry name=\"%s\" exclude=\"%s#%s;%s#%s\" />", TEST_NAME1,
                    EXCLUDE_TEST_CLASS, EXCLUDE_TEST_METHOD, EXCLUDE_TEST_CLASS,
                    EXCLUDE_TEST_METHOD2) +
        "</TestPlan>";

    static final String TEST_CLASS_EXCLUDED_DATA =
        "<TestPlan version=\"1.0\">" +
            String.format("<Entry name=\"%s\" exclude=\"%s\" />", TEST_NAME1,
                    EXCLUDE_TEST_CLASS) +
        "</TestPlan>";

    private TestPlan mPlan;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mPlan = new TestPlan("plan", AbiUtils.getAbisSupportedByCompatibility());
    }

    /**
     * Simple test for parsing a plan containing two names
     */
    public void testParse() throws ParseException  {
        mPlan.parse(getStringAsStream(TEST_DATA));
        assertTestData(mPlan);
    }

    /**
     * Perform checks to ensure TEST_DATA was parsed correctly
     * @param plan
     */
    private void assertTestData(TestPlan plan) {
        Set<String> abis = AbiUtils.getAbisSupportedByCompatibility();
        assertEquals(2 * abis.size(), plan.getTestIds().size());
        List<String> sortedAbis = new ArrayList<String>(abis);
        Collections.sort(sortedAbis);
        Iterator<String> iter = plan.getTestIds().iterator();
        for (String abi : sortedAbis) {
            String test1Id = AbiUtils.createId(abi, TEST_NAME1);
            String test2Id = AbiUtils.createId(abi, TEST_NAME2);
            // assert names in order
            assertEquals(test1Id, iter.next());
            assertEquals(test2Id, iter.next());
            assertFalse(plan.getTestFilter(test1Id).hasExclusion());
            assertFalse(plan.getTestFilter(test2Id).hasExclusion());
        }
    }

    /**
     * Test parsing a plan containing a single excluded test
     */
    public void testParse_exclude() throws ParseException  {
        mPlan.parse(getStringAsStream(TEST_EXCLUDED_DATA));
        Set<String> abis = AbiUtils.getAbisSupportedByCompatibility();
        assertEquals(abis.size(), mPlan.getTestIds().size());

        for (String abi : abis) {
            String test1Id = AbiUtils.createId(abi, TEST_NAME1);
            TestFilter filter = mPlan.getTestFilter(test1Id);
            assertTrue(filter.getExcludedTests().contains(new TestIdentifier(EXCLUDE_TEST_CLASS,
                    EXCLUDE_TEST_METHOD)));
        }
    }

    /**
     * Test parsing a plan containing multiple excluded tests
     */
    public void testParse_multiExclude() throws ParseException  {
        mPlan.parse(getStringAsStream(TEST_MULTI_EXCLUDED_DATA));
        assertMultiExcluded(mPlan);
    }

    /**
     * Perform checks to ensure TEST_MULTI_EXCLUDED_DATA was parsed correctly
     * @param plan
     */
    private void assertMultiExcluded(TestPlan plan) {
        Set<String> abis = AbiUtils.getAbisSupportedByCompatibility();
        assertEquals(abis.size(), plan.getTestIds().size());

        for (String abi : abis) {
            String test1Id = AbiUtils.createId(abi, TEST_NAME1);
            TestFilter filter = plan.getTestFilter(test1Id);
            assertTrue(filter.getExcludedTests().contains(new TestIdentifier(EXCLUDE_TEST_CLASS,
                    EXCLUDE_TEST_METHOD)));
            assertTrue(filter.getExcludedTests().contains(new TestIdentifier(EXCLUDE_TEST_CLASS,
                    EXCLUDE_TEST_METHOD2)));
        }
    }

    /**
     * Test parsing a plan containing an excluded class
     */
    public void testParse_classExclude() throws ParseException  {
        mPlan.parse(getStringAsStream(TEST_CLASS_EXCLUDED_DATA));
        Set<String> abis = AbiUtils.getAbisSupportedByCompatibility();
        assertEquals(abis.size(), mPlan.getTestIds().size());

        for (String abi : abis) {
            String test1Id = AbiUtils.createId(abi, TEST_NAME1);
            TestFilter filter = mPlan.getTestFilter(test1Id);
            assertTrue(filter.getExcludedClasses().contains(EXCLUDE_TEST_CLASS));
        }
    }

    /**
     * Test serializing an empty plan
     * @throws IOException
     */
    public void testSerialize_empty() throws IOException  {
        ByteArrayOutputStream outStream = new ByteArrayOutputStream();
        mPlan.serialize(outStream);
        assertTrue(outStream.toString().contains(EMPTY_DATA));
    }

    /**
     * Test serializing and deserializing plan with two packages
     * @throws IOException
     */
    public void testSerialize_packages() throws ParseException, IOException  {
        Set<String> abis = AbiUtils.getAbisSupportedByCompatibility();
        for (String abi : abis) {
            mPlan.addPackage(AbiUtils.createId(abi, TEST_NAME1));
            mPlan.addPackage(AbiUtils.createId(abi, TEST_NAME2));
        }
        ByteArrayOutputStream outStream = new ByteArrayOutputStream();
        mPlan.serialize(outStream);
        TestPlan parsedPlan = new TestPlan("parsed", AbiUtils.getAbisSupportedByCompatibility());
        parsedPlan.parse(getStringAsStream(outStream.toString()));
        // parsedPlan should contain same contents as TEST_DATA
        assertTestData(parsedPlan);
    }

    /**
     * Test serializing and deserializing plan with multiple excluded tests
     */
    public void testSerialize_multiExclude() throws ParseException, IOException  {
        Set<String> abis = AbiUtils.getAbisSupportedByCompatibility();

        for (String abi : abis) {
            String test1Id = AbiUtils.createId(abi, TEST_NAME1);
            mPlan.addPackage(test1Id);
            mPlan.addExcludedTest(test1Id, new TestIdentifier(EXCLUDE_TEST_CLASS,
                    EXCLUDE_TEST_METHOD));
            mPlan.addExcludedTest(test1Id, new TestIdentifier(EXCLUDE_TEST_CLASS,
                    EXCLUDE_TEST_METHOD2));
        }
        ByteArrayOutputStream outStream = new ByteArrayOutputStream();
        mPlan.serialize(outStream);
        TestPlan parsedPlan = new TestPlan("parsed", AbiUtils.getAbisSupportedByCompatibility());
        parsedPlan.parse(getStringAsStream(outStream.toString()));
        // parsedPlan should contain same contents as TEST_DATA
        assertMultiExcluded(parsedPlan);
    }

    private InputStream getStringAsStream(String input) {
        return new ByteArrayInputStream(input.getBytes());
    }
}
