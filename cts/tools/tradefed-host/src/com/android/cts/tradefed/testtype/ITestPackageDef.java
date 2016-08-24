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

import com.android.ddmlib.testrunner.TestIdentifier;
import com.android.tradefed.targetprep.ITargetPreparer;
import com.android.tradefed.testtype.IAbi;
import com.android.tradefed.testtype.IRemoteTest;

import java.io.File;
import java.util.Collection;
import java.util.List;

/**
 * Container for CTS test info.
 * <p/>
 * Knows how to translate this info into a runnable {@link IRemoteTest}.
 */
public interface ITestPackageDef extends Comparable<ITestPackageDef> {

    /**
     * Get the id of the test package.
     * @return the {@link String} id
     */
    public String getId();

    /**
     * Creates a runnable {@link IRemoteTest} from info stored in this definition.
     *
     * @param testCaseDir {@link File} representing directory of test case data
     * @return a {@link IRemoteTest} with all necessary data populated to run the test or
     *         <code>null</code> if test could not be created
     */
    public IRemoteTest createTest(File testCaseDir);

    /**
     * Get the collection of tests in this test package.
     */
    public Collection<TestIdentifier> getTests();

    /**
     * Return the sha1sum of the binary file for this test package.
     * <p/>
     * Will only return a valid value after {@link #createTest(File)} has been called.
     *
     * @return the sha1sum in {@link String} form
     */
    public String getDigest();

    /**
     * @return the name of this test package.
     */
    public String getName();

    /**
     * @return the ABI of this test package.
     */
    public IAbi getAbi();

    /**
     * Set the filter to use for tests
     *
     * @param testFilter
     */
    public void setTestFilter(TestFilter testFilter);

    /**
     * Restrict this test package to run a specific class and method name
     *
     * @param className the test class to restrict this run to
     * @param methodName the optional test method to restrict this run to, or <code>null</code> to
     *            run all tests in class
     */
    public void setClassName(String className, String methodName);

    /**
     * Return the file name of this package's instrumentation target apk.
     *
     * @return the file name or <code>null</code> if not applicable.
     */
    public String getTargetApkName();

    /**
     * Return the Android package name of this package's instrumentation target, or
     * <code>null</code> if not applicable.
     */
    public String getTargetPackageName();

    /**
     * Return a list of preparers used for setup or teardown of test cases in this package
     * @return
     */
    public List<ITargetPreparer> getPackagePreparers();
}
