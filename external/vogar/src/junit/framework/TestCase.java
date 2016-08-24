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

package junit.framework;

// Note: this class was written without inspecting the junit.framework code

import java.lang.reflect.Method;

public class TestCase extends Assert implements Test {

    private static final Method runTest;
    static {
        try {
            runTest = TestCase.class.getDeclaredMethod("runTest");
            runTest.setAccessible(true);
        } catch (NoSuchMethodException e) {
            throw new AssertionError();
        }
    }

    private String name;
    private Method method;

    public TestCase() {
        this("test");
    }

    public TestCase(String name) {
        this.name = name;
    }

    public String getName() {
        return name;
    }

    public void setName(String name) {
        this.name = name;
    }

    public final void setMethod(Method method) {
        this.method = method;
        this.name = method.getName();
    }

    public final Method getMethod() {
        if (method != null) {
            return method;
        }
        try {
            /*
             * Be careful to use the name field, which may differ from the
             * result of getName() if that method is overridden.
             */
            return getClass().getMethod(name);
        } catch (NoSuchMethodException e) {
            return runTest;
        }
    }

    protected void setUp() throws Exception {}

    protected void runTest() throws Throwable {
        fail("Expected runTest() to be overridden in " + getClass().getName());
    }

    protected void tearDown() throws Exception {}
}
