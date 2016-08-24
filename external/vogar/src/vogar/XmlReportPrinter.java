/*
 * Copyright (C) 2009 The Android Open Source Project
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

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Collection;
import java.util.Date;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.TimeZone;
import org.kxml2.io.KXmlSerializer;


/**
 * Writes JUnit results to a series of XML files in a format consistent with
 * Ant's XMLJUnitResultFormatter.
 *
 * <p>Unlike Ant's formatter, this class does not report the execution time of
 * tests.
 *
 * TODO: unify this and com.google.coretests.XmlReportPrinter
 */
public class XmlReportPrinter {

    /** the XML namespace */
    private static final String ns = null;

    private final File directory;
    private final ExpectationStore expectationStore;
    private final Date date;

    public XmlReportPrinter(File directory, ExpectationStore expectationStore, Date date) {
        this.directory = directory;
        this.expectationStore = expectationStore;
        this.date = date;
    }

    /**
     * Returns true if this XML Report printer can be used to emit XML.
     */
    public boolean isReady() {
        return directory != null;
    }

    private String getGMTTimestamp() {
        SimpleDateFormat dateFormat = new SimpleDateFormat(XmlReportConstants.DATEFORMAT);
        TimeZone gmt = TimeZone.getTimeZone("GMT");
        dateFormat.setTimeZone(gmt);
        dateFormat.setLenient(true);
        return dateFormat.format(date);
    }

    /**
     * Populates the directory with the report data from the completed tests.
     */
    public int generateReports(Collection<Outcome> results) {
        Map<String, Suite> suites = testsToSuites(results);

        String timestamp = getGMTTimestamp();

        for (Suite suite : suites.values()) {
            String fileName = "TEST-" + suite.name + ".xml";
            suite.printReport(timestamp, fileName);
        }

        return suites.size();
    }

    private Map<String, Suite> testsToSuites(Collection<Outcome> outcomes) {
        Map<String, Suite> result = new LinkedHashMap<String, Suite>();
        for (Outcome outcome : outcomes) {
            if (outcome.getResult() == Result.UNSUPPORTED) {
                continue;
            }

            String suiteName = outcome.getSuiteName();
            Suite suite = result.get(suiteName);
            if (suite == null) {
                suite = new Suite(suiteName);
                result.put(suiteName, suite);
            }

            suite.outcomes.add(outcome);

            Expectation expectation = expectationStore.get(outcome);
            if (!expectation.matches(outcome)) {
                if (outcome.getResult() == Result.EXEC_FAILED) {
                    suite.failuresCount++;
                } else {
                    suite.errorsCount++;
                }
            }
        }
        return result;
    }

    class Suite {
        private final String name;
        private final List<Outcome> outcomes = new ArrayList<Outcome>();
        private int failuresCount;
        private int errorsCount;

        Suite(String name) {
            this.name = name;
        }

        private void print(KXmlSerializer serializer, String timestamp) throws IOException {
            serializer.startTag(ns, XmlReportConstants.TESTSUITE);
            serializer.attribute(ns, XmlReportConstants.ATTR_NAME, name);
            serializer.attribute(ns, XmlReportConstants.ATTR_TESTS, Integer.toString(outcomes.size()));
            serializer.attribute(ns, XmlReportConstants.ATTR_FAILURES, Integer.toString(failuresCount));
            serializer.attribute(ns, XmlReportConstants.ATTR_ERRORS, Integer.toString(errorsCount));
            serializer.attribute(ns, XmlReportConstants.ATTR_TIME, "0");
            serializer.attribute(ns, XmlReportConstants.TIMESTAMP, timestamp);
            serializer.attribute(ns, XmlReportConstants.HOSTNAME, "localhost");
            serializer.startTag(ns, XmlReportConstants.PROPERTIES);
            serializer.endTag(ns, XmlReportConstants.PROPERTIES);

            for (Outcome outcome : outcomes) {
                print(serializer, outcome);
            }

            serializer.endTag(ns, XmlReportConstants.TESTSUITE);
        }

        private void print(KXmlSerializer serializer, Outcome outcome) throws IOException {
            serializer.startTag(ns, XmlReportConstants.TESTCASE);
            serializer.attribute(ns, XmlReportConstants.ATTR_NAME, outcome.getTestName());
            serializer.attribute(ns, XmlReportConstants.ATTR_CLASSNAME, outcome.getSuiteName());
            serializer.attribute(ns, XmlReportConstants.ATTR_TIME, "0");

            Expectation expectation = expectationStore.get(outcome);
            if (!expectation.matches(outcome)) {
                String result;
                switch (outcome.getResult()) {
                    case EXEC_FAILED:
                        result = XmlReportConstants.FAILURE;
                        break;
                    case SUCCESS:
                        result = XmlReportConstants.SUCCESS;
                        break;
                    default:
                        result = XmlReportConstants.ERROR;
                        break;
                }
                serializer.startTag(ns, result);
                serializer.attribute(ns, XmlReportConstants.ATTR_TYPE, outcome.getResult().toString());
                String text = outcome.getOutput();
                serializer.text(text);
                serializer.endTag(ns, result);
            }

            serializer.endTag(ns, XmlReportConstants.TESTCASE);
        }

        void printReport(String timestamp, String fileName) {
            FileOutputStream stream = null;
            try {
                stream = new FileOutputStream(new File(directory, fileName));

                KXmlSerializer serializer = new KXmlSerializer();
                serializer.setOutput(stream, "UTF-8");
                serializer.startDocument("UTF-8", null);
                serializer.setFeature(
                        "http://xmlpull.org/v1/doc/features.html#indent-output", true);
                print(serializer, timestamp);
                serializer.endDocument();
            } catch (IOException e) {
                throw new RuntimeException(e);
            } finally {
                if (stream != null) {
                    try {
                        stream.close();
                    } catch (IOException ignored) {
                    }
                }
            }
        }
    }
}
