/*
 * Copyright (C) 2016 The Android Open Source Project
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

package android.icu.junit;

import android.icu.dev.test.TestFmwk;
import java.io.PrintWriter;
import java.io.StringWriter;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import junit.framework.AssertionFailedError;
import org.junit.runner.Description;

/**
 * Represents a test within a {@link TestFmwk} class.
 */
final class IcuFrameworkTest implements Comparable<IcuFrameworkTest> {

    private static final String[] EMPTY_ARGS = new String[0];

    private static final Pattern EXTRACT_ERROR_INFO = Pattern.compile(
            "^[A-Za-z0-9_]+ \\{(\n.*)\n\\}.*", Pattern.DOTALL);

    /**
     * The {@link TestFmwk} instance on which the tests will be run.
     */
    private final TestFmwk testFmwk;

    private final TestFmwk.Target target;

    /**
     * The name of the individual target to run.
     */
    private final String methodName;

    IcuFrameworkTest(TestFmwk testFmwk, TestFmwk.Target target, String methodName) {
        this.testFmwk = testFmwk;
        this.target = target;
        this.methodName = methodName;
    }

    public String getMethodName() {
        return methodName;
    }

    /**
     * Runs the target.
     */
    public void run() {
        test_for_TestFmwk_Run();
    }

    /**
     * A special method to avoid the TestFmwk from throwing an InternalError when an error occurs
     * during execution of the test but outside the actual test method, e.g. in a
     * {@link TestFmwk#validate()} method. See http://bugs.icu-project.org/trac/ticket/12183
     *
     * <p>DO NOT CHANGE THE NAME
     */
    private void test_for_TestFmwk_Run() {
        StringWriter stringWriter = new StringWriter();
        PrintWriter log = new PrintWriter(stringWriter);

        TestFmwk.TestParams localParams = TestFmwk.TestParams.create(EMPTY_ARGS, log);
        if (localParams == null) {
            throw new IllegalStateException("Could not create params");
        }

        // We don't want an error summary as we are only running one test.
        localParams.errorSummary = null;

        try {
            // Make sure that the TestFmwk is initialized with the correct parameters. This method
            // is being called solely for its side effect of updating the TestFmwk.params field.
            testFmwk.resolveTarget(localParams);

            // Run the target.
            target.run();
        } catch (Exception e) {
            // Output the exception to the log and make sure it is treated as an error.
            e.printStackTrace(log);
            localParams.errorCount++;
        }

        // Treat warnings as errors.
        int errorCount = localParams.errorCount + localParams.warnCount;

        // Ensure that all data is written to the StringWriter.
        log.flush();

        // Treat warnings as errors.
        String information = stringWriter.toString();
        if (errorCount != 0) {
            // Remove unnecessary formatting.
            Matcher matcher = EXTRACT_ERROR_INFO.matcher(information);
            if (matcher.matches()) {
                information = matcher.group(1)/*.replace("\n    ", "\n")*/;
            }

            // Also append the logs to the console output.
            String output = "Failure: " + getDescription() + ", due to "
                    + errorCount + " error(s)\n" + information;

            throw new AssertionFailedError(output);
        }
    }

    /**
     * Get the JUnit {@link Description}
     */
    public Description getDescription() {
        // Get a description for the specific method within the class.
        return Description.createTestDescription(testFmwk.getClass(), methodName);
    }

    @Override
    public int compareTo(IcuFrameworkTest o) {
        return methodName.compareTo(o.methodName);
    }
}
