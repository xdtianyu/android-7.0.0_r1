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
import android.support.test.internal.util.AndroidRunnerParams;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import org.junit.runner.Description;
import org.junit.runner.notification.RunNotifier;
import org.junit.runners.model.Statement;

/**
 * A {@link org.junit.runner.Runner} that can be used to run a class that is a {@link TestFmwk}
 * but not a {@link android.icu.dev.test.TestFmwk.TestGroup}
 */
public class IcuTestFmwkRunner extends IcuTestParentRunner<IcuFrameworkTest> {

    /**
     * A {@link Statement} that does nothing, used when skipping execution.
     */
    private static final Statement EMPTY_STATEMENT = new Statement() {
        @Override
        public void evaluate() throws Throwable {
        }
    };

    private final boolean skipExecution;

    private final List<IcuFrameworkTest> tests;

    /**
     * The constructor used when this class is used with {@code @RunWith(...)}.
     */
    public IcuTestFmwkRunner(Class<?> testClass)
            throws Exception {
        this(checkClass(testClass), null);
    }

    /**
     * Make sure that the supplied test class is supported by this.
     */
    private static Class<? extends TestFmwk> checkClass(Class<?> testClass) {
        if (!TestFmwk.class.isAssignableFrom(testClass)) {
            throw new IllegalStateException(
                    "Cannot use " + IcuTestFmwkRunner.class + " for running "
                            + testClass + " as it is not a " + TestFmwk.class);
        }
        if (TestFmwk.TestGroup.class.isAssignableFrom(testClass)) {
            throw new IllegalStateException(
                    "Cannot use " + IcuTestFmwkRunner.class + " for running "
                            + testClass + " as it is a " + TestFmwk.TestGroup.class
                            + ": Use @RunWith(" + IcuTestGroupRunner.class.getSimpleName()
                            + ".class) instead");
        }

        return testClass.asSubclass(TestFmwk.class);
    }

    public IcuTestFmwkRunner(Class<? extends TestFmwk> testFmwkClass,
            AndroidRunnerParams runnerParams)
            throws Exception {
        super(testFmwkClass);

        this.skipExecution = runnerParams != null && runnerParams.isSkipExecution();

        // Create a TestFmwk and make sure that it's initialized properly.
        TestFmwk testFmwk = TestFmwkUtils.newTestFmwkInstance(testFmwkClass);

        tests = new ArrayList<>();

        TestFmwk.Target target = TestFmwkUtils.getTargets(testFmwk);
        while (target != null) {
            String name = target.name;
            // Just ignore targets that do not have a name, they are do nothing place holders.
            if (name != null) {
                tests.add(new IcuFrameworkTest(testFmwk, target, name));
            }
            target = target.getNext();
        }

        // If the class has no tests then fail.
        if (tests.isEmpty()) {
            throw new IllegalStateException("Cannot find any tests for " + testFmwkClass);
        }

        // Sort the methods to ensure consistent ordering.
        Collections.sort(tests);
    }

    @Override
    protected List<IcuFrameworkTest> getChildren() {
        return tests;
    }

    @Override
    protected Description describeChild(IcuFrameworkTest child) {
        return child.getDescription();
    }

    @Override
    protected void runChild(final IcuFrameworkTest child, RunNotifier notifier) {
        Description description = describeChild(child);
        Statement statement;
        if (skipExecution) {
            statement = EMPTY_STATEMENT;
        } else {
            statement = new Statement() {
                @Override
                public void evaluate() throws Throwable {
                    child.run();
                }
            };
        }
        runLeaf(statement, description, notifier);
    }
}
