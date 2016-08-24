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
package com.android.cts.core.runner.support;

import android.support.test.internal.runner.AndroidLogOnlyBuilder;
import android.support.test.internal.util.AndroidRunnerParams;
import android.support.test.runner.AndroidJUnit4;
import junit.framework.TestCase;
import org.junit.runner.RunWith;
import org.junit.runner.Runner;
import org.junit.runners.Parameterized;
import org.junit.runners.BlockJUnit4ClassRunner;
import org.junit.runners.JUnit4;
import org.junit.runners.model.RunnerBuilder;
import org.junit.runners.Suite;

import static android.support.test.internal.util.AndroidRunnerBuilderUtil.isJUnit3Test;
import static android.support.test.internal.util.AndroidRunnerBuilderUtil.isJUnit3TestSuite;

/**
 * Extends {@link AndroidLogOnlyBuilder} to add support for passing the
 * {@link AndroidRunnerParams} object to the constructor of any {@link RunnerBuilder}
 * implementation that is not a {@link BlockJUnit4ClassRunner}.
 *
 * <p>If {@link AndroidRunnerParams#isSkipExecution()} is {@code true} the super class will create
 * a {@link RunnerBuilder} that will fire appropriate events as if the tests are being run but will
 * not actually run the test. Unfortunately, when it does that it appears to assume that the runner
 * extends {@link BlockJUnit4ClassRunner}, returns a skipping {@link RunnerBuilder} appropriate for
 * that and ignores the actual {@code runnerClass}. That is a problem because it will not work for
 * custom {@link RunnerBuilder} instances that do not extend {@link BlockJUnit4ClassRunner}.
 *
 * <p>Therefore, when skipping execution this does some additional checks to make sure that the
 * {@code runnerClass} does extend {@link BlockJUnit4ClassRunner} before calling the overridden
 * method.
 *
 * <p>It then attempts to construct a {@link RunnerBuilder} by calling the constructor with the
 * signature {@code <init>(Class, AndroidRunnerParams)}. If that doesn't exist it falls back to
 * the overridden behavior.
 *
 * <p>Another unfortunate behavior of the {@link AndroidLogOnlyBuilder} is that it assumes the
 * {@link Parameterized} class has a field called DEFAULT_FACTORY. That field is only present
 * in JUnit 4.12, and not JUnit 4.10 used by tests run by CoreTestRunner.
 * Therefore these tests are actually skipped by this class and executed even though
 * isSkipExecution is true.
 */
class ExtendedAndroidLogOnlyBuilder extends AndroidLogOnlyBuilder {

    private final AndroidRunnerParams runnerParams;

    public ExtendedAndroidLogOnlyBuilder(AndroidRunnerParams runnerParams) {
        super(runnerParams);
        this.runnerParams = runnerParams;
    }

    @Override
    public Runner runnerForClass(Class<?> testClass) throws Throwable {
        if (!runnerParams.isSkipExecution() ||
                isJUnit3Test(testClass) || isJUnit3TestSuite(testClass)) {
            return super.runnerForClass(testClass);
        }

        RunWith annotation = testClass.getAnnotation(RunWith.class);

        if (annotation != null) {
            Class<? extends Runner> runnerClass = annotation.value();
            if (runnerClass == AndroidJUnit4.class || runnerClass == JUnit4.class
                    || TestCase.class.isAssignableFrom(testClass)) {
                return super.runnerForClass(testClass);
            }
            try {
                // b/28606746 try to build an AndroidJUnit4 runner to avoid BlockJUnit4ClassRunner
                return runnerClass.getConstructor(Class.class, AndroidRunnerParams.class).newInstance(
                    testClass, runnerParams);
            } catch (NoSuchMethodException e) {
                // Let the super class handle the error for us and throw an InitializationError
                // exception. Returning null means that this test will be executed.
                // b/28606746 Some parameterized tests that cannot be skipped fall through this
                return null;
            }
        }
        return super.runnerForClass(testClass);
    }
}
