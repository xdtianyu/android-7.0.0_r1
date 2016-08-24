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
import com.android.tradefed.util.ArrayUtil;
import com.android.tradefed.util.xml.AbstractXmlParser;

import org.kxml2.io.KXmlSerializer;
import org.xml.sax.Attributes;
import org.xml.sax.SAXException;
import org.xml.sax.helpers.DefaultHandler;

import java.io.IOException;
import java.io.OutputStream;
import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.TreeSet;

/**
 * Implementation of {@link TestPlan}.
 */
public class TestPlan extends AbstractXmlParser implements ITestPlan {

    /**
     * Map of ids found in plan, and their filters
     */
    private Map<String, TestFilter> mIdFilterMap;

    private static final String ENTRY_TAG = "Entry";
    private static final String TEST_DELIM = ";";
    private static final String METHOD_DELIM = "#";
    private static final String EXCLUDE_ATTR = "exclude";
    private static final String INCLUDE_ATTR = "include";
    private static final String ABI_ATTR = "abi";
    private static final String NAME_ATTR = "name";

    private final String mName;
    private final Set<String> mAbis;

    /**
     * SAX callback object. Handles parsing data from the xml tags.
     */
    private class EntryHandler extends DefaultHandler {

        @Override
        public void startElement(String uri, String localName, String name, Attributes attributes)
                throws SAXException {
            if (ENTRY_TAG.equals(localName)) {
                TestFilter filter = parseTestList(
                        attributes.getValue(EXCLUDE_ATTR), attributes.getValue(INCLUDE_ATTR));
                final String entryNameValue = attributes.getValue(NAME_ATTR);
                final String entryAbiValue = attributes.getValue(ABI_ATTR);
                if (entryAbiValue != null) {
                    mIdFilterMap.put(AbiUtils.createId(entryAbiValue, entryNameValue), filter);
                } else {
                    for (String abi : mAbis) {
                        mIdFilterMap.put(AbiUtils.createId(abi, entryNameValue), filter);
                    }
                }
            }
        }

        /**
         * Parse a semicolon separated list of tests.
         * <p/>
         * Expected format:
         * testClassName[#testMethodName][;testClassName2...]
         *
         * @param excludedString the excluded string list
         * @param includedString the included string list
         * @return
         */
        private TestFilter parseTestList(String excludedString, String includedString) {
            TestFilter filter = new TestFilter();
            if (excludedString != null) {
                String[] testStrings = excludedString.split(TEST_DELIM);
                for (String testString : testStrings) {
                    String[] classMethodPair = testString.split(METHOD_DELIM);
                    if (classMethodPair.length == 2) {
                        filter.addExcludedTest(new TestIdentifier(classMethodPair[0],
                                classMethodPair[1]));
                    } else {
                        filter.addExcludedClass(testString);
                    }
                }
            }
            if (includedString != null) {
                String[] testStrings = includedString.split(TEST_DELIM);
                for (String testString : testStrings) {
                    String[] classMethodPair = testString.split(METHOD_DELIM);
                    if (classMethodPair.length == 2) {
                        filter.addIncludedTest(new TestIdentifier(classMethodPair[0],
                                classMethodPair[1]));
                    } else {
                        filter.addIncludedClass(testString);
                    }
                }
            }

            return filter;
        }
    }

    public TestPlan(String name, Set<String> abis) {
        mName = name;
        mAbis = abis;
        // Uses a LinkedHashMap to have predictable iteration order
        mIdFilterMap = new LinkedHashMap<String, TestFilter>();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String getName() {
        return mName;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public Collection<String> getTestIds() {
        List<String> ids = new ArrayList<String>(mIdFilterMap.keySet());
        Collections.sort(ids);
        return ids;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public List<String> getTestNames() {
        TreeSet<String> testNameSet = new TreeSet<>();
        for (String id : mIdFilterMap.keySet()) {
            testNameSet.add(AbiUtils.parseTestName(id));
        }
        return new ArrayList<>(testNameSet);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public TestFilter getTestFilter(String id) {
        return mIdFilterMap.get(id);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void addPackage(String id) {
        mIdFilterMap.put(id, new TestFilter());
    }

    /**
     * {@inheritDoc}
     */
    @Override
    protected DefaultHandler createXmlHandler() {
        return new EntryHandler();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void addExcludedTest(String id, TestIdentifier testToExclude) {
        TestFilter filter = mIdFilterMap.get(id);
        if (filter != null) {
            filter.addExcludedTest(testToExclude);
        } else {
            throw new IllegalArgumentException(String.format("Could not find package %s", id));
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void addExcludedTests(String id, Collection<TestIdentifier> excludedTests) {
        TestFilter filter = mIdFilterMap.get(id);
        if (filter != null) {
            filter.getExcludedTests().addAll(excludedTests);
        } else {
            throw new IllegalArgumentException(String.format("Could not find package %s", id));
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void serialize(OutputStream stream) throws IOException {
        KXmlSerializer serializer = new KXmlSerializer();
        serializer.setOutput(stream, "UTF-8");
        serializer.startDocument("UTF-8", false);
        serializer.setFeature(
                "http://xmlpull.org/v1/doc/features.html#indent-output", true);
        serializer.startTag(null, "TestPlan");
        serializer.attribute(null, "version", "1.0");
        for (Map.Entry<String, TestFilter> packageEntry : mIdFilterMap.entrySet()) {
            serializer.startTag(null, ENTRY_TAG);
            String[] parts = AbiUtils.parseId(packageEntry.getKey());
            serializer.attribute(null, ABI_ATTR, parts[0]);
            serializer.attribute(null, NAME_ATTR, parts[1]);
            serializeFilter(serializer, packageEntry.getValue());
            serializer.endTag(null, ENTRY_TAG);
        }
        serializer.endTag(null, "TestPlan");
        serializer.endDocument();
    }

    /**
     * Adds an xml attribute containing {@link TestFilter} contents.
     * <p/>
     * If {@link TestFilter} is empty, no data will be output.
     *
     * @param serializer
     * @param testFilter
     * @throws IOException
     */
    private void serializeFilter(KXmlSerializer serializer, TestFilter testFilter)
            throws IOException {
        if (testFilter.hasExclusion()) {
            List<String> exclusionStrings = new ArrayList<String>();
            exclusionStrings.addAll(testFilter.getExcludedClasses());
            for (TestIdentifier test : testFilter.getExcludedTests()) {
                // TODO: this relies on TestIdentifier.toString() using METHOD_DELIM.
                exclusionStrings.add(test.toString());
            }
            String exclusionAttrValue = ArrayUtil.join(TEST_DELIM, exclusionStrings);
            serializer.attribute(null, EXCLUDE_ATTR, exclusionAttrValue);
        }

        if (testFilter.hasInclusion()) {
            List<String> inclusionStrings = new ArrayList<String>();
            inclusionStrings.addAll(testFilter.getIncludedClasses());
            for (TestIdentifier test : testFilter.getIncludedTests()) {
                // TODO: this relies on TestIdentifier.toString() using METHOD_DELIM.
                inclusionStrings.add(test.toString());
            }
            String exclusionAttrValue = ArrayUtil.join(TEST_DELIM, inclusionStrings);
            serializer.attribute(null, INCLUDE_ATTR, exclusionAttrValue);
        }

    }
}
