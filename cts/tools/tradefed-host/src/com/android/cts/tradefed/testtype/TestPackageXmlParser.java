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
import com.android.ddmlib.Log;
import com.android.ddmlib.testrunner.TestIdentifier;
import com.android.tradefed.util.xml.AbstractXmlParser;

import org.xml.sax.Attributes;
import org.xml.sax.helpers.DefaultHandler;

import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.Map;
import java.util.Set;
import java.util.Stack;

/**
 * Parser for CTS test case XML.
 * <p/>
 * Dumb parser that just retrieves data from in the test case xml and stuff it into a
 * {@link TestPackageDef}. Currently performs limited error checking.
 */
public class TestPackageXmlParser extends AbstractXmlParser {

    private static final String LOG_TAG = "TestPackageXmlParser";

    private final boolean mIncludeKnownFailures;

    private Map<String, TestPackageDef> mPackageDefs = new HashMap<String, TestPackageDef>();

    /**
     * @param includeKnownFailures Whether to run tests which are known to fail.
     */
    public TestPackageXmlParser(boolean includeKnownFailures) {
        mIncludeKnownFailures = includeKnownFailures;
    }

    /**
     * SAX callback object. Handles parsing data from the xml tags.
     * <p/>
     * Expected structure:
     * <TestPackage>
     *     <TestSuite ...>
     *        <TestCase>
     *           <Test>
     *             <TestInstance> (optional)
     */
    private class TestPackageHandler extends DefaultHandler {

        private static final String TEST_PACKAGE_TAG = "TestPackage";
        private static final String TEST_SUITE_TAG = "TestSuite";
        private static final String TEST_CASE_TAG = "TestCase";
        private static final String TEST_TAG = "Test";
        private static final String TEST_INSTANCE_TAG = "TestInstance";

        // holds current class name segments
        private Stack<String> mClassNameStack = new Stack<String>();
        private TestIdentifier mTestId;

        @Override
        public void startElement(String uri, String localName, String name, Attributes attributes) {
            if (TEST_PACKAGE_TAG.equals(localName)) {
                final String appPackageName = attributes.getValue("appPackageName");
                final String testPackageNameSpace = attributes.getValue("appNameSpace");
                final String packageName = attributes.getValue("name");
                final String runnerName = attributes.getValue("runner");
                final String jarPath = attributes.getValue("jarPath");
                final String javaPackageFilter = attributes.getValue("javaPackageFilter");
                final String targetBinaryName = attributes.getValue("targetBinaryName");
                final String targetNameSpace = attributes.getValue("targetNameSpace");
                final String runTimeArgs = attributes.getValue("runtimeArgs");
                final String testType = getTestType(attributes);

                for (String abiName : AbiUtils.getAbisSupportedByCompatibility()) {
                    Abi abi = new Abi(abiName, AbiUtils.getBitness(abiName));
                    TestPackageDef packageDef = new TestPackageDef();
                    packageDef.setAppPackageName(appPackageName);
                    packageDef.setAppNameSpace(testPackageNameSpace);
                    packageDef.setName(packageName);
                    packageDef.setRunner(runnerName);
                    packageDef.setTestType(testType);
                    packageDef.setJarPath(jarPath);
                    packageDef.setRunTimeArgs(runTimeArgs);
                    if (!"".equals(javaPackageFilter)) {
                        packageDef.setTestPackageName(javaPackageFilter);
                    }
                    packageDef.setTargetBinaryName(targetBinaryName);
                    packageDef.setTargetNameSpace(targetNameSpace);
                    packageDef.setAbi(abi);
                    mPackageDefs.put(abiName, packageDef);
                }

                // reset the class name
                mClassNameStack = new Stack<String>();
            } else if (TEST_SUITE_TAG.equals(localName)) {
                String packageSegment = attributes.getValue("name");
                if (packageSegment != null) {
                    mClassNameStack.push(packageSegment);
                } else {
                    Log.e(LOG_TAG, String.format("Invalid XML: missing 'name' attribute for '%s'",
                            TEST_SUITE_TAG));
                }
            } else if (TEST_CASE_TAG.equals(localName)) {
                String classSegment = attributes.getValue("name");
                if (classSegment != null) {
                    mClassNameStack.push(classSegment);
                } else {
                    Log.e(LOG_TAG, String.format("Invalid XML: missing 'name' attribute for '%s'",
                            TEST_CASE_TAG));
                }
            } else if (TEST_TAG.equals(localName)) {
                String methodName = attributes.getValue("name");
                if (mPackageDefs.isEmpty()) {
                    Log.e(LOG_TAG, String.format(
                            "Invalid XML: encountered a '%s' tag not enclosed within a '%s' tag",
                            TEST_TAG, TEST_PACKAGE_TAG));
                } else if (methodName == null) {
                    Log.e(LOG_TAG, String.format("Invalid XML: missing 'name' attribute for '%s'",
                            TEST_TAG));
                } else {
                    // build class name from package segments
                    StringBuilder classNameBuilder = new StringBuilder();
                    for (Iterator<String> iter = mClassNameStack.iterator(); iter.hasNext(); ) {
                        classNameBuilder.append(iter.next());
                        if (iter.hasNext()) {
                            classNameBuilder.append(".");
                        }
                    }
                    mTestId = new TestIdentifier(classNameBuilder.toString(), methodName);
                    int timeout = -1;
                    String timeoutStr = attributes.getValue("timeout");
                    if (timeoutStr != null) {
                        timeout = Integer.parseInt(timeoutStr);
                    }
                    boolean isKnownFailure = "failure".equals(attributes.getValue("expectation"));
                    if (!isKnownFailure || mIncludeKnownFailures) {
                        String abiList = attributes.getValue("abis");
                        Set<String> abis = new HashSet<String>();
                        if (abiList == null) {
                            // If no specification, add all supported abis
                            abis.addAll(AbiUtils.getAbisSupportedByCompatibility());
                        } else {
                            for (String abi : abiList.split(",")) {
                                // Else only add the abi which are supported
                                abis.add(abi.trim());
                            }
                        }
                        for (String abi : abis) {
                            mPackageDefs.get(abi).addTest(mTestId, timeout);
                        }
                    }
                }
            } else if (TEST_INSTANCE_TAG.equals(localName)) {
                if (mTestId != null) {
                    final Map<String, String> instanceArguments = genAttributeMap(attributes);
                    for (TestPackageDef packageDef : mPackageDefs.values()) {
                        if (packageDef.getTests().contains(mTestId)) {
                            packageDef.addTestInstance(mTestId, instanceArguments);
                        }
                    }
                } else {
                    Log.e(LOG_TAG, String.format(
                            "Invalid XML: encountered a '%s' tag not enclosed within a '%s' tag",
                            TEST_INSTANCE_TAG, TEST_TAG));
                }
            }
        }

        private String getTestType(Attributes attributes) {
            if (parseBoolean(attributes.getValue("hostSideOnly"))) {
                return TestPackageDef.HOST_SIDE_ONLY_TEST;
            } else if (parseBoolean(attributes.getValue("vmHostTest"))) {
                return TestPackageDef.VM_HOST_TEST;
            } else {
                return attributes.getValue("testType");
            }
        }

        @Override
        public void endElement (String uri, String localName, String qName) {
            if (TEST_SUITE_TAG.equals(localName) || TEST_CASE_TAG.equals(localName)) {
                mClassNameStack.pop();
            } else if (TEST_TAG.equals(localName)) {
                mTestId = null;
            }
        }

        /**
         * Parse a boolean attribute value
         */
        private boolean parseBoolean(final String stringValue) {
            return stringValue != null &&
                    Boolean.parseBoolean(stringValue);
        }

        private Map<String, String> genAttributeMap(Attributes attributes) {
            final Map<String, String> attribMap = new HashMap<String, String>();
            for (int i = 0; i < attributes.getLength(); ++i) {
                final String localName = attributes.getLocalName(i);
                final String namespace = attributes.getURI(i);
                final String fullyQualifiedName =
                        (namespace.isEmpty()) ? (localName) : (namespace + ":" + localName);

                attribMap.put(fullyQualifiedName, attributes.getValue(i));
            }
            return attribMap;
        }
    }

    @Override
    protected DefaultHandler createXmlHandler() {
        return new TestPackageHandler();
    }

    /**
     * @return the set of {@link TestPackageDef} containing data parsed from xml
     */
    public Set<TestPackageDef> getTestPackageDefs() {
        return new HashSet<>(mPackageDefs.values());
    }
}
