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
package com.android.cts.tradefed.result;

import com.android.cts.tradefed.result.TestLog.TestLogType;

import org.kxml2.io.KXmlSerializer;
import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserException;
import org.xmlpull.v1.XmlPullParserFactory;

import junit.framework.TestCase;

import java.io.StringReader;
import java.io.StringWriter;

/** Tests for {@link TestLog}. */
public class TestLogTest extends TestCase {

    public void testTestLogType_fromDataName() {
        assertNull(TestLogType.fromDataName(null));
        assertNull(TestLogType.fromDataName(""));
        assertNull(TestLogType.fromDataName("kmsg-foo_bar_test"));

        assertEquals(TestLogType.LOGCAT,
            TestLogType.fromDataName("logcat-foo_bar_test"));
        assertEquals(TestLogType.BUGREPORT,
            TestLogType.fromDataName("bug-foo_bar_test"));
    }

    public void testTestLogType_getAttrValue() {
        assertEquals("logcat", TestLogType.LOGCAT.getAttrValue());
        assertEquals("bugreport", TestLogType.BUGREPORT.getAttrValue());
    }

    public void testFromDataName() {
        TestLog log = TestLog.fromDataName("logcat-baz_test", "http://logs/baz_test");
        assertEquals(TestLogType.LOGCAT, log.getLogType());
        assertEquals("http://logs/baz_test", log.getUrl());
    }

    public void testFromDataName_unrecognizedDataName() {
        assertNull(TestLog.fromDataName("kmsg-baz_test", null));
    }

    public void testFromDataName_nullDataName() {
        assertNull(TestLog.fromDataName(null, "http://logs/baz_test"));
    }

    public void testFromDataName_nullUrl() {
        assertNull(TestLog.fromDataName("logcat-bar_test", null));
    }

    public void testFromDataName_allNull() {
        assertNull(TestLog.fromDataName(null, null));
    }

    public void testFromXml() throws Exception {
        TestLog log = TestLog.fromXml(newXml("<TestLog type=\"logcat\" url=\"http://logs/baz_test\">"));
        assertEquals(TestLogType.LOGCAT, log.getLogType());
        assertEquals("http://logs/baz_test", log.getUrl());
    }

    public void testFromXml_unrecognizedType() throws Exception {
        assertNull(TestLog.fromXml(newXml("<TestLog type=\"kmsg\" url=\"http://logs/baz_test\">")));
    }

    public void testFromXml_noTypeAttribute() throws Exception {
        assertNull(TestLog.fromXml(newXml("<TestLog url=\"http://logs/baz_test\">")));
    }

    public void testFromXml_noUrlAttribute() throws Exception {
        assertNull(TestLog.fromXml(newXml("<TestLog type=\"bugreport\">")));
    }

    public void testFromXml_allNull() throws Exception {
        assertNull(TestLog.fromXml(newXml("<TestLog>")));
    }

    public void testSerialize() throws Exception {
        KXmlSerializer serializer = new KXmlSerializer();
        StringWriter writer = new StringWriter();
        serializer.setOutput(writer);

        TestLog log = TestLog.of(TestLogType.LOGCAT, "http://logs/foo/bar");
        log.serialize(serializer);
        assertEquals("<TestLog type=\"logcat\" url=\"http://logs/foo/bar\" />", writer.toString());
    }

    public void testIsTag() {
        assertTrue(TestLog.isTag("TestLog"));
        assertFalse(TestLog.isTag("TestResult"));
    }

    private XmlPullParser newXml(String xml) throws Exception {
        XmlPullParserFactory factory = org.xmlpull.v1.XmlPullParserFactory.newInstance();
        XmlPullParser parser = factory.newPullParser();
        parser.setInput(new StringReader(xml));

        // Move the parser from the START_DOCUMENT stage to the START_TAG of the data.
        parser.next();

        return parser;
    }
}
