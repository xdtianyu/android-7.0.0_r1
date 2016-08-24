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
import java.io.InputStream;
import java.util.Iterator;
import java.util.List;
import java.util.Map;

/**
 * Unit tests for {@link TestPackageXmlParser}.
 */
public class TestPackageXmlParserTest extends TestCase {

    private static final String INSTR_TEST_DATA =
        "<TestPackage AndroidFramework=\"Android 1.0\" appNameSpace=\"com.example\" " +
        "appPackageName=\"android.example\" name=\"CtsExampleTestCases\" " +
        "runner=\"android.test.InstrumentationTestRunner\" version=\"1.0\">" +
        "</TestPackage>";

    private static final String HOST_TEST_DATA =
        "<TestPackage hostSideOnly=\"true\" >\n" +
        "    <TestSuite name=\"com\" >\n" +
        "        <TestSuite name=\"example\" >\n" +
        "            <TestCase name=\"ExampleTest\" >\n" +
        "                <Test name=\"testFoo\" />\n" +
        "                <Test name=\"testFoo2\" expectation=\"failure\" />\n" +
        "            </TestCase>\n" +
        "        </TestSuite>\n" +
        "        <TestSuite name=\"example2\" >\n" +
        "            <TestCase name=\"Example2Test\" >\n" +
        "                <Test name=\"testFoo\" />\n" +
        "            </TestCase>\n" +
        "        </TestSuite>\n" +
        "    </TestSuite>\n" +
        "</TestPackage>";

    private static final String BAD_HOST_TEST_DATA =
        "<TestPackage hostSideOnly=\"blah\" >" +
        "</TestPackage>";

    private static final String VM_HOST_TEST_XML =
            "<TestPackage vmHostTest=\"true\"></TestPackage>";

    private static final String NATIVE_TEST_XML = "<TestPackage testType=\"native\"></TestPackage>";

    private static final String NO_TEST_DATA = "<invalid />";

    private static final String INSTANCED_TEST_DATA =
        "<TestPackage>\n" +
        "    <TestSuite name=\"com\" >\n" +
        "        <TestSuite name=\"example\" >\n" +
        "            <TestCase name=\"ExampleTest\" >\n" +
        "                <Test name=\"testMultiInstanced\" >\n" +
        "                    <TestInstance foo=\"bar\" />\n" +
        "                    <TestInstance foo=\"baz\" foo2=\"baz2\"/>\n" +
        "                </Test>\n" +
        "                <Test name=\"testSingleInstanced\" >\n" +
        "                    <TestInstance foo=\"bar\" />\n" +
        "                </Test>\n" +
        "                <Test name=\"testEmptyInstances\" >\n" +
        "                    <TestInstance />\n" +
        "                    <TestInstance />\n" +
        "                </Test>\n" +
        "                <Test name=\"testNotInstanced\" >\n" +
        "                </Test>\n" +
        "            </TestCase>\n" +
        "        </TestSuite>\n" +
        "    </TestSuite>\n" +
        "</TestPackage>";

    /**
     * Test parsing test case xml containing an instrumentation test definition.
     */
    public void testParse_instrPackage() throws ParseException  {
        TestPackageXmlParser parser = new TestPackageXmlParser(true);
        parser.parse(getStringAsStream(INSTR_TEST_DATA));
        for (TestPackageDef def : parser.getTestPackageDefs()) {
            assertEquals("com.example", def.getAppNameSpace());
            assertEquals("android.example", def.getAppPackageName());
            assertEquals("android.test.InstrumentationTestRunner", def.getRunner());
            assertTrue(AbiUtils.isAbiSupportedByCompatibility(def.getAbi().getName()));
        }
    }

    /**
     * Test parsing test case xml containing an host test attribute and test data.
     */
    public void testParse_hostTest() throws ParseException  {
        TestPackageXmlParser parser = new TestPackageXmlParser(true);
        parser.parse(getStringAsStream(HOST_TEST_DATA));
        for (TestPackageDef def : parser.getTestPackageDefs()) {
            assertEquals(TestPackageDef.HOST_SIDE_ONLY_TEST, def.getTestType());
            assertEquals(3, def.getTests().size());
            Iterator<TestIdentifier> iterator = def.getTests().iterator();

            TestIdentifier firstTest = iterator.next();
            assertEquals("com.example.ExampleTest", firstTest.getClassName());
            assertEquals("testFoo", firstTest.getTestName());

            TestIdentifier secondTest = iterator.next();
            assertEquals("com.example.ExampleTest", secondTest.getClassName());
            assertEquals("testFoo2", secondTest.getTestName());

            TestIdentifier thirdTest = iterator.next();
            assertEquals("com.example2.Example2Test", thirdTest.getClassName());
            assertEquals("testFoo", thirdTest.getTestName());

            assertFalse(iterator.hasNext());
        }
    }

    public void testParse_hostTest_noKnownFailures() throws ParseException  {
        TestPackageXmlParser parser = new TestPackageXmlParser(false);
        parser.parse(getStringAsStream(HOST_TEST_DATA));
        for (TestPackageDef def : parser.getTestPackageDefs()) {
            assertEquals(TestPackageDef.HOST_SIDE_ONLY_TEST, def.getTestType());
            assertEquals(2, def.getTests().size());
            Iterator<TestIdentifier> iterator = def.getTests().iterator();

            TestIdentifier firstTest = iterator.next();
            assertEquals("com.example.ExampleTest", firstTest.getClassName());
            assertEquals("testFoo", firstTest.getTestName());

            TestIdentifier thirdTest = iterator.next();
            assertEquals("com.example2.Example2Test", thirdTest.getClassName());
            assertEquals("testFoo", thirdTest.getTestName());

            assertFalse(iterator.hasNext());
        }
    }

    /**
     * Test parsing test case xml containing an invalid host test attribute.
     */
    public void testParse_badHostTest() throws ParseException  {
        TestPackageXmlParser parser = new TestPackageXmlParser(true);
        parser.parse(getStringAsStream(BAD_HOST_TEST_DATA));
        for (TestPackageDef def : parser.getTestPackageDefs()) {
            assertFalse(TestPackageDef.HOST_SIDE_ONLY_TEST.equals(def.getTestType()));
        }
    }

    public void testParse_vmHostTest() throws ParseException  {
        assertTestType(TestPackageDef.VM_HOST_TEST, VM_HOST_TEST_XML);
    }

    public void testParse_nativeTest() throws ParseException  {
        assertTestType(TestPackageDef.NATIVE_TEST, NATIVE_TEST_XML);
    }

    private void assertTestType(String expectedType, String xml) throws ParseException {
        TestPackageXmlParser parser = new TestPackageXmlParser(true);
        parser.parse(getStringAsStream(xml));
        for (TestPackageDef def : parser.getTestPackageDefs()) {
            assertEquals(expectedType, def.getTestType());
        }
    }

    /**
     * Test parsing a test case xml with no test package data.
     */
    public void testParse_noData() throws ParseException  {
        TestPackageXmlParser parser = new TestPackageXmlParser(true);
        parser.parse(getStringAsStream(NO_TEST_DATA));
        assertTrue(parser.getTestPackageDefs().isEmpty());
    }

    /**
     * Test parsing a test case xml with multiple test instances
     */
    public void testParse_instancedMultiple() throws ParseException  {
        TestPackageXmlParser parser = new TestPackageXmlParser(true);
        parser.parse(getStringAsStream(INSTANCED_TEST_DATA));
        for (TestPackageDef def : parser.getTestPackageDefs()) {
            final TestIdentifier testId =
                    new TestIdentifier("com.example.ExampleTest", "testMultiInstanced");
            final List<Map<String, String>> targetInstances =
                    def.getTestInstanceArguments().get(testId);
            assertNotNull(targetInstances);
            assertEquals(2, targetInstances.size());

            final Iterator<Map<String, String>> iterator = targetInstances.iterator();
            final Map<String, String> firstInstance = iterator.next();
            final Map<String, String> secondInstance = iterator.next();

            assertEquals("bar", firstInstance.get("foo"));
            assertEquals("baz", secondInstance.get("foo"));
            assertEquals("baz2", secondInstance.get("foo2"));
        }
    }

    /**
     * Test parsing a test case xml with single test instance
     */
    public void testParse_instancedSingle() throws ParseException  {
        TestPackageXmlParser parser = new TestPackageXmlParser(true);
        parser.parse(getStringAsStream(INSTANCED_TEST_DATA));
        for (TestPackageDef def : parser.getTestPackageDefs()) {
            final TestIdentifier testId =
                    new TestIdentifier("com.example.ExampleTest", "testSingleInstanced");
            final List<Map<String, String>> targetInstances =
                    def.getTestInstanceArguments().get(testId);
            assertNotNull(targetInstances);
            assertEquals(1, targetInstances.size());

            final Iterator<Map<String, String>> iterator = targetInstances.iterator();
            final Map<String, String> firstInstance = iterator.next();

            assertEquals("bar", firstInstance.get("foo"));
        }
    }

    /**
     * Test parsing a test case xml with multiple test instances with no data
     */
    public void testParse_instancedEmptys() throws ParseException  {
        TestPackageXmlParser parser = new TestPackageXmlParser(true);
        parser.parse(getStringAsStream(INSTANCED_TEST_DATA));
        for (TestPackageDef def : parser.getTestPackageDefs()) {
            final TestIdentifier testId =
                    new TestIdentifier("com.example.ExampleTest", "testEmptyInstances");
            final List<Map<String, String>> targetInstances =
                    def.getTestInstanceArguments().get(testId);
            assertNotNull(targetInstances);
            assertEquals(2, targetInstances.size());

            final Iterator<Map<String, String>> iterator = targetInstances.iterator();
            final Map<String, String> firstInstance = iterator.next();
            final Map<String, String> secondInstance = iterator.next();

            assertTrue(firstInstance.isEmpty());
            assertTrue(secondInstance.isEmpty());
        }
    }

    /**
     * Test parsing a test case xml with no test instances
     */
    public void testParse_instancedNoInstances() throws ParseException  {
        TestPackageXmlParser parser = new TestPackageXmlParser(true);
        parser.parse(getStringAsStream(INSTANCED_TEST_DATA));
        for (TestPackageDef def : parser.getTestPackageDefs()) {
            final TestIdentifier testId =
                    new TestIdentifier("com.example.ExampleTest", "testNotInstanced");
            final List<Map<String, String>> targetInstances =
                    def.getTestInstanceArguments().get(testId);
            assertNotNull(targetInstances);
            assertTrue(targetInstances.isEmpty());
        }
    }

    private InputStream getStringAsStream(String input) {
        return new ByteArrayInputStream(input.getBytes());
    }
}
