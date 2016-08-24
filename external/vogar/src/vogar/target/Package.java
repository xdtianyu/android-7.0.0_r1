/*
 * Copyright (C) 2008 The Android Open Source Project
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
import java.util.Collections;
import java.util.HashSet;
import java.util.Set;
import java.util.TreeSet;

/**
 * The class and subpackage contents of a package.
 *
 * <p>Adapted from android.test.ClassPathPackageInfo.
 */
class Package {

    private final ClassPathScanner source;
    private final Set<String> subpackageNames;
    private final Set<Class<?>> topLevelClasses;

    Package(ClassPathScanner source,
            Set<String> subpackageNames, Set<Class<?>> topLevelClasses) {
        this.source = source;
        this.subpackageNames = Collections.unmodifiableSet(subpackageNames);
        this.topLevelClasses = Collections.unmodifiableSet(topLevelClasses);
    }

    public Set<Class<?>> getTopLevelClassesRecursive() throws IOException {
        Set<Class<?>> set = new TreeSet<Class<?>>(ClassPathScanner.ORDER_CLASS_BY_NAME);
        addTopLevelClassesTo(set);
        return set;
    }

    private Set<Package> getSubpackages() throws IOException {
        Set<Package> info = new HashSet<Package>();
        for (String name : subpackageNames) {
            info.add(source.scan(name));
        }
        return info;
    }

    private void addTopLevelClassesTo(Set<Class<?>> set) throws IOException {
        set.addAll(topLevelClasses);
        for (Package info : getSubpackages()) {
            info.addTopLevelClassesTo(set);
        }
    }
}
