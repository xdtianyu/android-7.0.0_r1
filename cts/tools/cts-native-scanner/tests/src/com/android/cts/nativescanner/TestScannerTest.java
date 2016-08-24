/*
 * Copyright 2012 The Android Open Source Project
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
package com.android.cts.nativescanner;

import com.android.cts.nativescanner.TestScanner;

import junit.framework.TestCase;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.StringReader;
import java.util.List;
import java.util.Iterator;

/**
 * Unit tests for {@link TestScanner}.
 */
public class TestScannerTest extends TestCase {

    public void testSingleTestNamesCase() throws Exception {
        StringReader singleTestString = new StringReader("FakeTestCase.\n  FakeTestName\n");
        BufferedReader reader = new BufferedReader(singleTestString);

        TestScanner testScanner = new TestScanner(reader, "TestSuite");

        List<String> names = testScanner.getTestNames();
        Iterator it = names.iterator();
        assertEquals("suite:TestSuite", it.next());
        assertEquals("case:FakeTestCase", it.next());
        assertEquals("test:FakeTestName", it.next());
        assertFalse(it.hasNext());
    }

    public void testMultipleTestNamesCase() throws Exception {
        StringReader singleTestString = new StringReader(
          "Case1.\n  Test1\n  Test2\nCase2.\n  Test3\n Test4\n");
        BufferedReader reader = new BufferedReader(singleTestString);

        TestScanner testScanner = new TestScanner(reader, "TestSuite");

        List<String> names = testScanner.getTestNames();
        Iterator it = names.iterator();
        assertEquals("suite:TestSuite", it.next());
        assertEquals("case:Case1", it.next());
        assertEquals("test:Test1", it.next());
        assertEquals("test:Test2", it.next());
        assertEquals("case:Case2", it.next());
        assertEquals("test:Test3", it.next());
        assertEquals("test:Test4", it.next());
        assertFalse(it.hasNext());
    }

    public void testMissingTestCaseNameCase() {
        StringReader singleTestString = new StringReader("  Test1\n");
        BufferedReader reader = new BufferedReader(singleTestString);

        TestScanner testScanner = new TestScanner(reader, "TestSuite");

        try {
          List<String> names = testScanner.getTestNames();
          fail();
        } catch (IOException expected) {
        }
    }
}
