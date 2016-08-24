/*
 * Copyright (C) 2010 The Android Open Source Project
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

package vogar.target;

import java.io.IOException;
import java.util.Arrays;
import java.util.Collections;
import java.util.Set;

class ClassFinder {
    /**
     * Returns either a Set with the class represented by classOrPackageName as its only element, if
     * classOrPackageName represents a class, or a Set containing all of the classes contained
     * within the package represented by classOrPackageName, if it represents a package.
     *
     * Throws an exception if it represents neither a class nor a package with at least one class.
     */
    public Set<Class<?>> find(String classOrPackageName) {
        try {
            // if no exception thrown, classOrPackageName must represent a class
            return Collections.<Class<?>>singleton(Class.forName(classOrPackageName));
        } catch (ClassNotFoundException e) {
        }
        // classOrPackageName might represent a package
        try {
            Package aPackage = new ClassPathScanner().scan(classOrPackageName);
            Set<Class<?>> classes = aPackage.getTopLevelClassesRecursive();
            if (classes.isEmpty()) {
                throw new IllegalArgumentException("No classes in package: " + classOrPackageName +
                        "; classpath is " + Arrays.toString(ClassPathScanner.getClassPath()));
            }
            return classes;
        } catch (IOException eIO) {
            throw new RuntimeException(eIO);
        }
    }
}
