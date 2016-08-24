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
import android.icu.util.TimeZone;
import java.lang.reflect.Field;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Locale;

/**
 * Abstracts away reflection code that accesses various hidden fields and methods in the ICU test
 * framework.
 *
 * <p>Assuming that this is integrated into ICU test framework itself then ideally the test
 * framework will itself be modified to remove the need for this reflection at which point this
 * class can be removed.
 */
public class TestFmwkUtils {

    /**
     * The field on TestGroup which has the list of classes in it.
     */
    private static final Field classesToTestField = getField(TestFmwk.TestGroup.class, "names");

    /**
     * The field on TestGroup which has the default package in it.
     */
    private static final Field defaultPackageField =
            getField(TestFmwk.TestGroup.class, "defaultPackage");

    /**
     * The field on TestFmwk which has the {@link android.icu.dev.test.TestFmwk.TestParams} in it.
     */
    private static final Field paramsField = getField(TestFmwk.class, "params");

    private static final Method getTargetsMethod = getTargetsMethod();

    /**
     * The default time zone for all the tests.
     */
    private final static TimeZone defaultTimeZone =
        getStaticFieldValue("defaultTimeZone", TimeZone.class);

    /**
     * The default locale used for all the tests.
     */
    private final static Locale defaultLocale = getStaticFieldValue("defaultLocale", Locale.class);

    private static <T> T getStaticFieldValue(String fieldName, Class<T> fieldClass) {
        Field field = getField(TestFmwk.class, fieldName);
        try {
            return fieldClass.cast(field.get(null));
        } catch (IllegalAccessException e) {
            throw new RuntimeException("Could not get field: " + fieldName, e);
        }
    }

    private static Field getField(Class<?> theClass, String name) {
        // Find the field, and complain if it is not where it's expected to be.
        try {
            Field field = theClass.getDeclaredField(name);
            field.setAccessible(true);  // It's private by default.
            return field;
        } catch (NoSuchFieldException e) {
            throw new RuntimeException("Class structure of ICU tests have changed.", e);
        }
    }

    private static Method getTargetsMethod() {
        try {
            Method method = TestFmwk.class.getDeclaredMethod("getTargets", String.class);
            method.setAccessible(true);
            return method;
        } catch (NoSuchMethodException e) {
            throw new RuntimeException("Class structure of ICU tests have changed.", e);
        }
    }

    public static TestFmwk.Target getTargets(TestFmwk testFmwk) {
        return test_for_TestFmwk_GetTargets(testFmwk);
    }

    /**
     * A special method to avoid the TestFmwk from throwing an InternalError when an error occurs
     * during execution of the test but outside the actual test method, e.g. in a
     * {@link TestFmwk#validate()} method. See http://bugs.icu-project.org/trac/ticket/12183
     *
     * <p>DO NOT CHANGE THE NAME
     */
    private static TestFmwk.Target test_for_TestFmwk_GetTargets(TestFmwk testFmwk) {
        try {
            // Set the default locale and time zone here as it can affect the targets returned.
            // ICU4J relies on the fact that the Target.run() method of a TestGroup based test is
            // called before the getTargets() method is called. Under JUnit we get the targets
            // first.
            Locale.setDefault(defaultLocale);
            TimeZone.setDefault(defaultTimeZone);

            return (TestFmwk.Target) getTargetsMethod.invoke(testFmwk, new Object[] {null});
        } catch (InvocationTargetException | IllegalAccessException e) {
            throw new IllegalStateException(
                    "Problem calling getTargets(null) on " + testFmwk, e);
        }
    }

    public static <T extends TestFmwk> T newTestFmwkInstance(Class<? extends T> testFmwkClass)
            throws IllegalAccessException, InstantiationException {
        // Create a TestFmwk and make sure that it's params field is initialized.
        T testFmwk = testFmwkClass.newInstance();
        TestFmwk.TestParams testParams = TestFmwk.TestParams.create(new String[0], null);
        if (testParams == null) {
            throw new IllegalStateException("Could not create TestParams");
        }
        // Set a dummy stack to avoid NPE if a test logs a known error.
        testParams.stack = testParams.new State(null, "DUMMY", false);
        paramsField.set(testFmwk, testParams);
        return testFmwk;
    }

    public static List<String> getClassNames(TestFmwk.TestGroup testGroup) {
        try {
            String[] classNames = (String[]) classesToTestField.get(testGroup);
            String defaultPackage = (String) defaultPackageField.get(testGroup);

            List<String> list = new ArrayList<>(classNames.length);
            for (String basicName : classNames) {
                // Handle relative class names.
                String fullyQualifiedName;
                if (basicName.contains(".")) {
                    fullyQualifiedName = basicName;
                } else {
                    fullyQualifiedName = defaultPackage + basicName;
                }

                list.add(fullyQualifiedName);
            }

            // Sort to ensure consistent ordering.
            Collections.sort(list);

            return list;
        } catch (IllegalAccessException e) {
            throw new IllegalStateException("Problem getting class names from " + testGroup, e);
        }
    }
}
