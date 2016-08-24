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

import java.util.ArrayList;
import java.util.Collections;
import java.util.Enumeration;
import java.util.List;
import java.util.ListIterator;
import vogar.target.junit.Junit3;

public class TestSuite implements Test {
    /** A heterogeneous list containing of tests and test classes. */
    private final List<Object> testsAndSuites = new ArrayList<Object>();
    private final String name;

    public TestSuite() {
        this.name = null;
    }

    public TestSuite(String name) {
        this.name = name;
    }

    public TestSuite(Class<?> suite) {
        this(suite, null);
    }

    public TestSuite(Class<?> suite, String name) {
        if (suite == null) {
            throw new IllegalArgumentException("suite == null");
        }
        testsAndSuites.add(suite);
        this.name = name;
    }

    public String getName() {
        return name;
    }

    public void addTest(Test test) {
        if (!(test instanceof TestCase) && !(test instanceof TestSuite)) {
            throw new IllegalArgumentException("Unexpected test: " + test);
        }
        testsAndSuites.add(test);
    }
    
    public void addTestSuite(Class<?> suite) {
        testsAndSuites.add(suite);
    }
    
    public int countTestCases() {
        return testsAndSuites.size();
    }
    
    /**
     * The official JUnit framework creates test instances eagerly and holds
     * them for the duration of the test run. We prefer to create tests lazily,
     * and release them after use. Unfortunately, calls to this method require
     * us to fall back to JUnit-style eager creation. This method should only be
     * used by framework code.
     */
    public Enumeration<?> tests() {
        for (ListIterator<Object> i = testsAndSuites.listIterator(); i.hasNext(); ) {
            Object o = i.next();
            if (o instanceof Class) {
                i.remove();
                for (Test test : Junit3.classToJunitTests((Class<?>) o)) {
                    i.add(test);
                }
            }
        }
        return Collections.enumeration(testsAndSuites);
    }

    public final List<Object> getTestsAndSuites() {
        return testsAndSuites;
    } 
}
