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

import org.kxml2.io.KXmlSerializer;
import org.xmlpull.v1.XmlPullParser;

import java.io.IOException;

import javax.annotation.Nullable;

/**
 * TestLog describes a log for a test. It corresponds to the "TestLog" XML element.
 */
class TestLog {

    private static final String TAG = "TestLog";
    private static final String TYPE_ATTR = "type";
    private static final String URL_ATTR = "url";

    /** Type of log. */
    public enum TestLogType {
        LOGCAT("logcat-"),
        BUGREPORT("bug-"),

        ;

        // This enum restricts the type of logs reported back to the server,
        // because we do not intend to support them all. Using an enum somewhat
        // assures that we will only see these expected types on the server side.

        /**
         * Returns the TestLogType from an ILogSaver data name or null
         * if the data name is not supported.
         */
        @Nullable
        static TestLogType fromDataName(String dataName) {
            if (dataName == null) {
                return null;
            }

            for (TestLogType type : values()) {
                if (dataName.startsWith(type.dataNamePrefix)) {
                    return type;
                }
            }
            return null;
        }

        private final String dataNamePrefix;

        private TestLogType(String dataNamePrefix) {
            this.dataNamePrefix = dataNamePrefix;
        }

        String getAttrValue() {
            return name().toLowerCase();
        }
    }

    /** Type of the log like LOGCAT or BUGREPORT. */
    private final TestLogType mLogType;

    /** Url pointing to the log file. */
    private final String mUrl;

    /** Create a TestLog from an ILogSaver callback. */
    @Nullable
    static TestLog fromDataName(String dataName, String url) {
        TestLogType logType = TestLogType.fromDataName(dataName);
        if (logType == null) {
            return null;
        }

        if (url == null) {
            return null;
        }

        return new TestLog(logType, url);
    }

    /** Create a TestLog from XML given a XmlPullParser positioned at the TestLog tag. */
    @Nullable
    static TestLog fromXml(XmlPullParser parser) {
        String type = parser.getAttributeValue(null, TYPE_ATTR);
        if (type == null) {
            return null;
        }

        String url = parser.getAttributeValue(null, URL_ATTR);
        if (url == null) {
            return null;
        }

        try {
            TestLogType logType = TestLogType.valueOf(type.toUpperCase());
            return new TestLog(logType, url);
        } catch (IllegalArgumentException e) {
            return null;
        }
    }

    /** Create a TestLog directly given a log type and url. */
    public static TestLog of(TestLogType logType, String url) {
        return new TestLog(logType, url);
    }

    private TestLog(TestLogType logType, String url) {
        this.mLogType = logType;
        this.mUrl = url;
    }

    /** Returns this TestLog's log type. */
    TestLogType getLogType() {
        return mLogType;
    }

    /** Returns this TestLog's URL. */
    String getUrl() {
        return mUrl;
    }

    /** Serialize the TestLog to XML. */
    public void serialize(KXmlSerializer serializer) throws IOException {
        serializer.startTag(CtsXmlResultReporter.ns, TAG);
        serializer.attribute(CtsXmlResultReporter.ns, TYPE_ATTR, mLogType.getAttrValue());
        serializer.attribute(CtsXmlResultReporter.ns, URL_ATTR, mUrl);
        serializer.endTag(CtsXmlResultReporter.ns, TAG);
    }

    /** Returns true if the tag is a TestLog tag. */
    public static boolean isTag(String tagName) {
        return TAG.equals(tagName);
    }
}
