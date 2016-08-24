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

package com.android.cts.xmlgenerator;

import vogar.Expectation;
import vogar.ExpectationStore;
import vogar.Result;

import com.android.compatibility.common.util.AbiUtils;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.Map;
import java.util.List;
import java.util.Set;

/**
 * Generator of TestPackage XML files for native tests.
 *
 * It takes in an input of the following form:
 *
 * suite: x.y.z
 * case:TestClass1
 * test:testMethod1
 * test:testMethod2
 * case:TestClass2
 * test:testMethod1
 * suite: x.y
 * case:TestClass3
 * test:testMethod2
 */
class XmlGenerator {

    /** Example: com.android.cts.holo */
    private final String mAppNamespace;

    /** Test package name like "android.nativemedia" to group the tests. */
    private final String mAppPackageName;

    /** Name of the native executable. */
    private final String mName;

    /** Test runner */
    private final String mRunner;

    private final String mTargetBinaryName;

    private final String mTargetNameSpace;

    private final String mJarPath;

    private final String mTestType;

    /** Path to output file or null to just dump to standard out. */
    private final String mOutputPath;

    /** ExpectationStore to filter out known failures. */
    private final ExpectationStore mKnownFailures;

    /** ExpectationStore to filter out unsupported abis. */
    private final ExpectationStore mUnsupportedAbis;

    private final String mArchitecture;

    private final Map<String, String> mAdditionalAttributes;

    XmlGenerator(ExpectationStore knownFailures, ExpectationStore unsupportedAbis,
            String architecture, String appNameSpace, String appPackageName, String name,
            String runner, String targetBinaryName, String targetNameSpace, String jarPath,
            String testType, String outputPath, Map<String, String> additionalAttributes) {
        mAppNamespace = appNameSpace;
        mAppPackageName = appPackageName;
        mName = name;
        mRunner = runner;
        mTargetBinaryName = targetBinaryName;
        mTargetNameSpace = targetNameSpace;
        mJarPath = jarPath;
        mTestType = testType;
        mOutputPath = outputPath;
        mKnownFailures = knownFailures;
        mUnsupportedAbis = unsupportedAbis;
        mArchitecture = architecture;
        mAdditionalAttributes = additionalAttributes;
    }

    public void writePackageXml() throws IOException {
        OutputStream output = System.out;
        if (mOutputPath != null) {
            File outputFile = new File(mOutputPath);
            output = new FileOutputStream(outputFile);
        }

        PrintWriter writer = null;
        try {
            writer = new PrintWriter(output);
            writer.println("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
            writeTestPackage(writer);
        } finally {
            if (writer != null) {
                writer.close();
            }
        }
    }

    private void writeTestPackage(PrintWriter writer) {
        writer.append("<TestPackage");
        if (mAppNamespace != null) {
            writer.append(" appNameSpace=\"").append(mAppNamespace).append("\"");
        }

        writer.append(" appPackageName=\"").append(mAppPackageName).append("\"");
        writer.append(" name=\"").append(mName).append("\"");

        if (mRunner != null) {
            writer.append(" runner=\"").append(mRunner).append("\"");
        }

        if (mAppNamespace != null && mTargetNameSpace != null
                && !mAppNamespace.equals(mTargetNameSpace)) {
            writer.append(" targetBinaryName=\"").append(mTargetBinaryName).append("\"");
            writer.append(" targetNameSpace=\"").append(mTargetNameSpace).append("\"");
        }

        if (mTestType != null && !mTestType.isEmpty()) {
            writer.append(" testType=\"").append(mTestType).append("\"");
        }

        if (mJarPath != null) {
            writer.append(" jarPath=\"").append(mJarPath).append("\"");
        }

        for (Map.Entry<String, String> entry : mAdditionalAttributes.entrySet()) {
            writer.append(String.format(" %s=\"%s\"", entry.getKey(), entry.getValue()));
        }

        writer.println(" version=\"1.0\">");

        TestListParser parser = new TestListParser();
        Collection<TestSuite> suites = parser.parse(System.in);
        StringBuilder nameCollector = new StringBuilder();
        writeTestSuites(writer, suites, nameCollector);
        writer.println("</TestPackage>");
    }

    private void writeTestSuites(PrintWriter writer, Collection<TestSuite> suites,
            StringBuilder nameCollector) {
        Collection<TestSuite> sorted = sortCollection(suites);
        for (TestSuite suite : sorted) {
            if (suite.countTests() == 0) {
                continue;
            }
            writer.append("<TestSuite name=\"").append(suite.getName()).println("\">");

            String namePart = suite.getName();
            if (nameCollector.length() > 0) {
                namePart = "." + namePart;
            }
            nameCollector.append(namePart);

            writeTestSuites(writer, suite.getSuites(), nameCollector);
            writeTestCases(writer, suite.getCases(), nameCollector);

            nameCollector.delete(nameCollector.length() - namePart.length(),
                    nameCollector.length());
            writer.println("</TestSuite>");
        }
    }

    private void writeTestCases(PrintWriter writer, Collection<TestCase> cases,
            StringBuilder nameCollector) {
        Collection<TestCase> sorted = sortCollection(cases);
        for (TestCase testCase : sorted) {
            if (testCase.countTests() == 0) {
                continue;
            }
            String name = testCase.getName();
            writer.append("<TestCase name=\"").append(name).println("\">");
            nameCollector.append('.').append(name);

            writeTests(writer, testCase.getTests(), nameCollector);

            nameCollector.delete(nameCollector.length() - name.length() - 1,
                    nameCollector.length());
            writer.println("</TestCase>");
        }
    }

    private void writeTests(PrintWriter writer, Collection<Test> tests,
            StringBuilder nameCollector) {
        Collection<Test> sorted = sortCollection(tests);
        for (Test test : sorted) {
            String className = nameCollector.toString();
            nameCollector.append('#').append(test.getName());
            writer.append("<Test name=\"").append(test.getName()).append("\"");
            String abis = getSupportedAbis(mUnsupportedAbis, mArchitecture,
                    className, nameCollector.toString()).toString();
            writer.append(" abis=\"" + abis.substring(1, abis.length() - 1) + "\"");
            if (isKnownFailure(mKnownFailures, nameCollector.toString())) {
                writer.append(" expectation=\"failure\"");
            }
            if (test.getTimeout() >= 0) {
                writer.append(" timeout=\"" + test.getTimeout() + "\"");
            }
            writer.println(" />");

            nameCollector.delete(nameCollector.length() - test.getName().length() - 1,
                    nameCollector.length());
        }
    }

    private <E extends Comparable<E>> Collection<E> sortCollection(Collection<E> col) {
        List<E> list = new ArrayList<E>(col);
        Collections.sort(list);
        return list;
    }

    public static boolean isKnownFailure(ExpectationStore expectationStore, String testName) {
        return expectationStore != null
            && expectationStore.get(testName).getResult() != Result.SUCCESS;
    }

    // Returns the list of ABIs supported by this TestCase on this architecture.
    public static Set<String> getSupportedAbis(ExpectationStore expectationStore,
            String architecture, String className, String testName) {
        Set<String> supportedAbis = AbiUtils.getAbisForArch(architecture);
        if (expectationStore == null) {
            return supportedAbis;
        }

        removeUnsupportedAbis(expectationStore.get(className), supportedAbis);
        removeUnsupportedAbis(expectationStore.get(testName), supportedAbis);
        return supportedAbis;
    }

    public static void removeUnsupportedAbis(Expectation expectation, Set<String> supportedAbis) {
        if (expectation == null) {
            return;
        }

        String description = expectation.getDescription();
        if (description.isEmpty()) {
            return;
        }

        String[] unsupportedAbis = description.split(":")[1].split(",");
        for (String a : unsupportedAbis) {
            String abi = a.trim();
            if (!AbiUtils.isAbiSupportedByCompatibility(abi)) {
                throw new RuntimeException(
                        String.format("Unrecognised ABI %s in %s", abi, description));
            }
            supportedAbis.remove(abi);
        }
    }

}
