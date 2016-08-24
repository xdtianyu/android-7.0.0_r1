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
package vogar.target.junit;

import com.google.common.annotations.VisibleForTesting;
import java.util.Collections;
import java.util.Comparator;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Set;
import java.util.concurrent.atomic.AtomicReference;
import javax.annotation.Nullable;
import junit.framework.AssertionFailedError;
import vogar.monitor.TargetMonitor;
import vogar.target.Runner;
import vogar.target.RunnerFactory;
import vogar.target.TestEnvironment;

/**
 * Creates a Runner for JUnit 3 or JUnit 4 tests.
 */
public class JUnitRunnerFactory implements RunnerFactory {

    @Override @Nullable
    public Runner newRunner(TargetMonitor monitor, String qualification,
            Class<?> klass, AtomicReference<String> skipPastReference,
            TestEnvironment testEnvironment, int timeoutSeconds, boolean profile,
            String[] args) {
        if (supports(klass)) {
            List<VogarTest> tests = createVogarTests(klass, qualification, args);
            return new JUnitRunner(monitor, skipPastReference, testEnvironment, timeoutSeconds,
                    tests);
        } else {
            return null;
        }
    }

    @VisibleForTesting
    public static List<VogarTest> createVogarTests(
            Class<?> testClass, String qualification, String[] args) {

        Set<String> methodNames = new LinkedHashSet<>();
        if (qualification != null) {
            methodNames.add(qualification);
        }
        Collections.addAll(methodNames, args);

        final List<VogarTest> tests;
        if (Junit3.isJunit3Test(testClass)) {
            tests = Junit3.classToVogarTests(testClass, methodNames);
        } else if (Junit4.isJunit4Test(testClass)) {
            tests = Junit4.classToVogarTests(testClass, methodNames);
        } else {
            throw new AssertionFailedError("Unknown JUnit type: " + testClass.getName());
        }

        // Sort the tests to ensure consistent ordering.
        Collections.sort(tests, new Comparator<VogarTest>() {
            @Override
            public int compare(VogarTest o1, VogarTest o2) {
                return o1.toString().compareTo(o2.toString());
            }
        });
        return tests;
    }

    @VisibleForTesting
    boolean supports(Class<?> klass) {
        return Junit3.isJunit3Test(klass) || Junit4.isJunit4Test(klass);
    }
}