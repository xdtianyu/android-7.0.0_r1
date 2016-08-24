/*
 * Copyright (C) 2015 The Android Open Source Project
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

package com.android.cts.core.runner;

import android.app.Activity;
import android.app.Instrumentation;
import android.os.Bundle;
import android.os.Debug;
import android.support.test.internal.runner.listener.InstrumentationResultPrinter;
import android.support.test.internal.runner.listener.InstrumentationRunListener;
import android.support.test.internal.util.AndroidRunnerParams;
import android.util.Log;
import com.android.cts.core.runner.support.ExtendedAndroidRunnerBuilder;
import com.google.common.base.Splitter;
import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.Collections;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Set;
import java.util.HashSet;
import javax.annotation.Nullable;
import org.junit.runner.Computer;
import org.junit.runner.JUnitCore;
import org.junit.runner.Request;
import org.junit.runner.Runner;
import org.junit.runner.manipulation.Filter;
import org.junit.runner.manipulation.Filterable;
import org.junit.runner.manipulation.NoTestsRemainException;
import org.junit.runner.notification.RunListener;
import org.junit.runners.model.InitializationError;
import org.junit.runners.model.RunnerBuilder;
import vogar.ExpectationStore;
import vogar.ModeId;

import static com.android.cts.core.runner.AndroidJUnitRunnerConstants.ARGUMENT_COUNT;
import static com.android.cts.core.runner.AndroidJUnitRunnerConstants.ARGUMENT_DEBUG;
import static com.android.cts.core.runner.AndroidJUnitRunnerConstants.ARGUMENT_LOG_ONLY;
import static com.android.cts.core.runner.AndroidJUnitRunnerConstants.ARGUMENT_NOT_TEST_CLASS;
import static com.android.cts.core.runner.AndroidJUnitRunnerConstants.ARGUMENT_NOT_TEST_FILE;
import static com.android.cts.core.runner.AndroidJUnitRunnerConstants.ARGUMENT_NOT_TEST_PACKAGE;
import static com.android.cts.core.runner.AndroidJUnitRunnerConstants.ARGUMENT_TEST_CLASS;
import static com.android.cts.core.runner.AndroidJUnitRunnerConstants.ARGUMENT_TEST_FILE;
import static com.android.cts.core.runner.AndroidJUnitRunnerConstants.ARGUMENT_TEST_PACKAGE;
import static com.android.cts.core.runner.AndroidJUnitRunnerConstants.ARGUMENT_TIMEOUT;

/**
 * A drop-in replacement for AndroidJUnitTestRunner, which understands the same arguments, and has
 * similar functionality, but can filter by expectations and allows a custom runner-builder to be
 * provided.
 */
public class CoreTestRunner extends Instrumentation {

    public static final String TAG = "LibcoreTestRunner";

    private static final java.lang.String ARGUMENT_ROOT_CLASSES = "core-root-classes";

    private static final String ARGUMENT_EXPECTATIONS = "core-expectations";

    private static final String ARGUMENT_CORE_LISTENER = "core-listener";

    private static final Splitter CLASS_LIST_SPLITTER = Splitter.on(',').trimResults();

    /** The args for the runner. */
    private Bundle args;

    /** Only count the number of tests, and not run them. */
    private boolean testCountOnly;

    /** Only log the number of tests, and not run them. */
    private boolean logOnly;

    /** The amount of time in millis to wait for a single test to complete. */
    private long testTimeout;

    /**
     * The container for any test expectations.
     */
    @Nullable
    private ExpectationStore expectationStore;

    /**
     * The list of tests to run.
     */
    private TestList testList;

    /**
     * The list of {@link RunListener} classes to create.
     */
    private List<Class<? extends RunListener>> listenerClasses;

    @Override
    public void onCreate(final Bundle args) {
        super.onCreate(args);
        this.args = args;

        boolean debug = "true".equalsIgnoreCase(args.getString(ARGUMENT_DEBUG));
        if (debug) {
            Log.i(TAG, "Waiting for debugger to connect...");
            Debug.waitForDebugger();
            Log.i(TAG, "Debugger connected.");
        }

        // Log the message only after getting a value from the args so that the args are
        // unparceled.
        Log.d(TAG, "In OnCreate: " + args);

        this.logOnly = "true".equalsIgnoreCase(args.getString(ARGUMENT_LOG_ONLY));
        this.testCountOnly = args.getBoolean(ARGUMENT_COUNT);
        this.testTimeout = parseUnsignedLong(args.getString(ARGUMENT_TIMEOUT), ARGUMENT_TIMEOUT);

        try {
            // Get the set of resource names containing the expectations.
            Set<String> expectationResources = new LinkedHashSet<>(
                    getExpectationResourcePaths(args));
            expectationStore = ExpectationStore.parseResources(
                    getClass(), expectationResources, ModeId.DEVICE);
        } catch (IOException e) {
            Log.e(TAG, "Could not initialize ExpectationStore: ", e);
        }

        // The test can be run specifying a list of tests to run, or as cts-tradefed does it,
        // by passing a fileName with a test to run on each line.
        Set<String> testNameSet = new HashSet<>();
        String arg;
        if ((arg = args.getString(ARGUMENT_TEST_FILE)) != null) {
            // The tests are specified in a file.
            try {
                testNameSet.addAll(readTestsFromFile(arg));
            } catch (IOException err) {
                finish(Activity.RESULT_CANCELED, new Bundle());
                return;
            }
        } else if ((arg = args.getString(ARGUMENT_TEST_CLASS)) != null) {
            // The tests are specified in a String passed in the bundle.
            String[] tests = arg.split(",");
            testNameSet.addAll(Arrays.asList(tests));
        }

        // Tests may be excluded from the run by passing a list of tests not to run,
        // or by passing a fileName with a test not to run on each line.
        Set<String> notTestNameSet = new HashSet<>();
        if ((arg = args.getString(ARGUMENT_NOT_TEST_FILE)) != null) {
            // The tests are specified in a file.
            try {
                notTestNameSet.addAll(readTestsFromFile(arg));
            } catch (IOException err) {
                finish(Activity.RESULT_CANCELED, new Bundle());
                return;
            }
        } else if ((arg = args.getString(ARGUMENT_NOT_TEST_CLASS)) != null) {
            // The classes are specified in a String passed in the bundle
            String[] tests = arg.split(",");
            notTestNameSet.addAll(Arrays.asList(tests));
        }

        Set<String> packageNameSet = new HashSet<>();
        if ((arg = args.getString(ARGUMENT_TEST_PACKAGE)) != null) {
            // The packages are specified in a String passed in the bundle
            String[] packages = arg.split(",");
            packageNameSet.addAll(Arrays.asList(packages));
        }

        Set<String> notPackageNameSet = new HashSet<>();
        if ((arg = args.getString(ARGUMENT_NOT_TEST_PACKAGE)) != null) {
            // The packages are specified in a String passed in the bundle
            String[] packages = arg.split(",");
            notPackageNameSet.addAll(Arrays.asList(packages));
        }

        List<String> roots = getRootClassNames(args);
        if (roots == null) {
            // Find all test classes
            Collection<Class<?>> classes = TestClassFinder.getClasses(
                Collections.singletonList(getContext().getPackageCodePath()),
                getClass().getClassLoader());
            testList = new TestList(classes);
        } else {
            testList = TestList.rootList(roots);
        }

        testList.addIncludeTestPackages(packageNameSet);
        testList.addExcludeTestPackages(notPackageNameSet);
        testList.addIncludeTests(testNameSet);
        testList.addExcludeTests(notTestNameSet);

        listenerClasses = new ArrayList<>();
        String listenerArg = args.getString(ARGUMENT_CORE_LISTENER);
        if (listenerArg != null) {
            List<String> listenerClassNames = CLASS_LIST_SPLITTER.splitToList(listenerArg);
            for (String listenerClassName : listenerClassNames) {
                try {
                    Class<? extends RunListener> listenerClass = Class.forName(listenerClassName)
                            .asSubclass(RunListener.class);
                    listenerClasses.add(listenerClass);
                } catch (ClassNotFoundException e) {
                    Log.e(TAG, "Could not load listener class: " + listenerClassName, e);
                }
            }
        }

        start();
    }

    protected List<String> getExpectationResourcePaths(Bundle args) {
        return CLASS_LIST_SPLITTER.splitToList(args.getString(ARGUMENT_EXPECTATIONS));
    }

    protected List<String> getRootClassNames(Bundle args) {
        String rootClasses = args.getString(ARGUMENT_ROOT_CLASSES);
        List<String> roots;
        if (rootClasses == null) {
            roots = null;
        } else {
            roots = CLASS_LIST_SPLITTER.splitToList(rootClasses);
        }
        return roots;
    }

    @Override
    public void onStart() {
        if (logOnly || testCountOnly) {
            Log.d(TAG, "Counting/logging tests only");
        } else {
            Log.d(TAG, "Running tests");
        }

        AndroidRunnerParams runnerParams = new AndroidRunnerParams(this, args,
                logOnly || testCountOnly, testTimeout, false /*ignoreSuiteMethods*/);

        JUnitCore core = new JUnitCore();

        Request request;
        try {
            RunnerBuilder runnerBuilder = new ExtendedAndroidRunnerBuilder(runnerParams);
            Class[] classes = testList.getClassesToRun();
            for (Class cls : classes) {
              Log.d(TAG, "Found class to run: " + cls.getName());
            }
            Runner suite = new Computer().getSuite(runnerBuilder, classes);

            if (suite instanceof Filterable) {
                Filterable filterable = (Filterable) suite;

                // Filter out all the tests that are expected to fail.
                Filter filter = new TestFilter(testList, expectationStore);

                try {
                    filterable.filter(filter);
                } catch (NoTestsRemainException e) {
                    // Sometimes filtering will remove all tests but we do not care about that.
                }
            }

            request = Request.runner(suite);

        } catch (InitializationError e) {
            throw new RuntimeException("Could not create a suite", e);
        }

        InstrumentationResultPrinter instrumentationResultPrinter =
                new InstrumentationResultPrinter();
        instrumentationResultPrinter.setInstrumentation(this);
        core.addListener(instrumentationResultPrinter);

        for (Class<? extends RunListener> listenerClass : listenerClasses) {
            try {
                RunListener runListener = listenerClass.newInstance();
                if (runListener instanceof InstrumentationRunListener) {
                    ((InstrumentationRunListener) runListener).setInstrumentation(this);
                }
                core.addListener(runListener);
            } catch (InstantiationException | IllegalAccessException e) {
                Log.e(TAG, "Could not create instance of listener: " + listenerClass, e);
            }
        }

        Bundle results = new Bundle();
        try {
            core.run(request);
        } catch (RuntimeException e) {
            final String msg = "Fatal exception when running tests";
            Log.e(TAG, msg, e);
            // report the exception to instrumentation out
            results.putString(Instrumentation.REPORT_KEY_STREAMRESULT,
                    msg + "\n" + Log.getStackTraceString(e));
        }

        Log.d(TAG, "Finished");
        finish(Activity.RESULT_OK, results);
    }

    /**
     * Read tests from a specified file.
     *
     * @return class names of tests. If there was an error reading the file, null is returned.
     */
    private static List<String> readTestsFromFile(String fileName) throws IOException {
        try (BufferedReader br = new BufferedReader(new FileReader(fileName))) {
            List<String> tests = new ArrayList<>();
            String line;
            while ((line = br.readLine()) != null) {
                tests.add(line);
            }
            return tests;
        } catch (IOException err) {
            Log.e(TAG, "There was an error reading the test class list: " + err.getMessage());
            throw err;
        }
    }

    /**
     * Parse long from given value - except either Long or String.
     *
     * @return the value, -1 if not found
     * @throws NumberFormatException if value is negative or not a number
     */
    private static long parseUnsignedLong(Object value, String name) {
        if (value != null) {
            long longValue = Long.parseLong(value.toString());
            if (longValue < 0) {
                throw new NumberFormatException(name + " can not be negative");
            }
            return longValue;
        }
        return -1;
    }

}
