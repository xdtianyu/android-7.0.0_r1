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

package vogar;

import com.google.common.collect.ImmutableSet;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileReader;
import java.io.IOException;
import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Collection;
import java.util.Date;
import java.util.HashMap;
import java.util.Map;
import java.util.Set;
import java.util.TimeZone;
import org.kxml2.io.KXmlParser;
import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserException;

public final class XmlReportReader {

    private final static Set<String> resultTagNames = ImmutableSet.of(
        XmlReportConstants.FAILURE,
        XmlReportConstants.ERROR,
        XmlReportConstants.SUCCESS);

    public Collection<Outcome> readSuiteReport(File xmlReport) {
        FileInputStream stream = null;
        try {
            stream = new FileInputStream(xmlReport);

            KXmlParser parser = new KXmlParser();
            try {
                parser.setInput(stream, "UTF-8");
                parser.setInput(new FileReader(xmlReport));
                return readTestSuite(parser);
            } catch (XmlPullParserException e1) {
                throw new RuntimeException(e1);
            }
        } catch (IOException e) {
            throw new RuntimeException(e);
        } finally {
            if (stream != null) {
                try {
                    stream.close();
                } catch(IOException ignored) {
                }
            }
        }
    }

    private Collection<Outcome> readTestSuite(KXmlParser parser)
            throws XmlPullParserException, IOException {
        Collection<Outcome> outcomes = new ArrayList<Outcome>();

        parser.nextTag();
        parser.require(XmlPullParser.START_TAG, null, XmlReportConstants.TESTSUITE);
        Map<String, String> testSuiteAttributes = createAttributeMap(parser);
        String timestamp = testSuiteAttributes.get(XmlReportConstants.TIMESTAMP);

        parser.nextTag();
        parser.require(XmlPullParser.START_TAG, null, XmlReportConstants.PROPERTIES);
        parser.nextTag();
        parser.require(XmlPullParser.END_TAG, null, XmlReportConstants.PROPERTIES);
        while (parser.nextTag() == XmlPullParser.START_TAG) {
            parser.require(XmlPullParser.START_TAG, null, XmlReportConstants.TESTCASE);

            Map<String, String> testCaseAttributes = createAttributeMap(parser);
            String name = testCaseAttributes.get(XmlReportConstants.ATTR_NAME);
            String classname = testCaseAttributes.get(XmlReportConstants.ATTR_CLASSNAME);

            Result result = Result.SUCCESS;
            String resultOutput = null;
            parser.nextTag();
            String tagName = parser.getName();
            if (resultTagNames.contains(tagName)) {
                parser.require(XmlPullParser.START_TAG, null, tagName);

                Map<String, String> resultAttributes = createAttributeMap(parser);
                String type = resultAttributes.get(XmlReportConstants.ATTR_TYPE);
                result = Result.valueOf(type);

                resultOutput = parser.nextText();

                parser.require(XmlPullParser.END_TAG, null, tagName);
                parser.nextTag();
            }

            // create outcome!
            SimpleDateFormat dateFormat = new SimpleDateFormat(XmlReportConstants.DATEFORMAT);
            dateFormat.setTimeZone(TimeZone.getTimeZone("GMT"));
            dateFormat.setLenient(true);
            Date date;
            try {
                date = dateFormat.parse(timestamp);
            } catch (ParseException e) {
                throw new RuntimeException(e);
            }
            outcomes.add(new Outcome(classname + "#" + name, result, resultOutput, date));

            parser.require(XmlPullParser.END_TAG, null, XmlReportConstants.TESTCASE);
        }
        parser.require(XmlPullParser.END_TAG, null, XmlReportConstants.TESTSUITE);

        return outcomes;
    }

    private Map<String, String> createAttributeMap(KXmlParser parser) {
        Map<String, String> attributeMap = new HashMap<String, String>();
        int attributeCount = parser.getAttributeCount();
        for (int i = 0; i < attributeCount; i++) {
            String attributeName = parser.getAttributeName(i);
            String attributeValue = parser.getAttributeValue(i);
            attributeMap.put(attributeName, attributeValue);
        }
        return attributeMap;
    }
}