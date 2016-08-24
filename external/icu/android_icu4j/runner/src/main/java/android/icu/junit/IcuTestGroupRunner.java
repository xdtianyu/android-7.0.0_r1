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
import java.util.ArrayList;
import java.util.List;
import org.junit.runner.Description;
import org.junit.runner.Runner;
import org.junit.runner.notification.RunNotifier;
import org.junit.runners.model.RunnerBuilder;

/**
 * A {@link org.junit.runner.Runner} that can be used to run a class that is a
 * {@link TestFmwk.TestGroup} but not a {@link TestFmwk}
 */
public class IcuTestGroupRunner extends IcuTestParentRunner<Runner> {

    private final List<Runner> runners;

    /**
     * The constructor used when this class is used with {@code @RunWith(...)}.
     */
    public IcuTestGroupRunner(Class<?> testClass, RunnerBuilder runnerBuilder)
            throws Exception {
        super(testClass);

        Class<? extends TestFmwk.TestGroup> testGroupClass = checkClass(testClass);

        // Create a TestGroup and make sure that it's initialized properly.
        TestFmwk.TestGroup testGroup = TestFmwkUtils.newTestFmwkInstance(testGroupClass);

        runners = new ArrayList<>();
        List<String> classNames = TestFmwkUtils.getClassNames(testGroup);
        ClassLoader classLoader = testGroupClass.getClassLoader();
        for (String className : classNames) {
            Runner runner;

            try {
                Class<?> childTestClass = Class.forName(className, false, classLoader);
                runner = runnerBuilder.safeRunnerForClass(childTestClass);
            } catch (ClassNotFoundException e) {
                runner = new ErrorReportingRunner(className, e);
            }

            runners.add(runner);
        }
    }

    /**
     * Make sure that the supplied test class is supported by this.
     */
    private static Class<? extends TestFmwk.TestGroup> checkClass(Class<?> testClass) {
        if (!TestFmwk.TestGroup.class.isAssignableFrom(testClass)) {
            if (TestFmwk.class.isAssignableFrom(testClass)) {
                throw new IllegalStateException(
                        "Cannot use " + IcuTestGroupRunner.class + " for running "
                                + testClass + " as it is a " + TestFmwk.class
                                + ": Use @RunWith(" + IcuTestFmwkRunner.class.getSimpleName()
                                + ".class) instead");
            }
            throw new IllegalStateException(
                    "Cannot use " + IcuTestGroupRunner.class + " for running "
                            + testClass + " as it is not a " + TestFmwk.TestGroup.class);
        }

        return testClass.asSubclass(TestFmwk.TestGroup.class);
    }

    @Override
    protected List<Runner> getChildren() {
        return runners;
    }

    @Override
    protected Description describeChild(Runner child) {
        return child.getDescription();
    }

    @Override
    protected void runChild(Runner child, RunNotifier notifier) {
        child.run(notifier);
    }
}
