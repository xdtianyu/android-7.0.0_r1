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

import org.junit.runners.ParentRunner;
import org.junit.runners.model.InitializationError;

/**
 * Extends {@link ParentRunner} to prevent it from trying to create an instance of
 * {@link org.junit.runners.model.TestClass} for the supplied {@code testClass} because that
 * requires that the {@code testClass} has only a single constructor and at least one ICU test
 * ({@code android.icu.dev.test.serializable.CoverageTest}) has more than one constructor.
 *
 * <p>This provides a dummy class and overrides the {@link #getName()} method to return the
 * correct name. The consequence of this is that it is not possible to use JUnit 4 annotations
 * related to the class, like {@link org.junit.BeforeClass}, {@link org.junit.ClassRule}, etc.
 */
abstract class IcuTestParentRunner<T> extends ParentRunner<T> {

    private final Class<?> testClass;

    IcuTestParentRunner(Class<?> testClass) throws InitializationError {
        super(DummyTestClass.class);
        this.testClass = testClass;
    }

    @Override
    protected String getName() {
        return testClass.getName();
    }

    /**
     * A dummy test class to pass to {@link ParentRunner} for it to validate and check for
     * annotations.
     *
     * <p>Must be public.
     */
    public static class DummyTestClass {

    }
}
