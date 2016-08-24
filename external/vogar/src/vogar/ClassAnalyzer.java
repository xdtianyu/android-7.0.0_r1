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

package vogar;

import java.lang.reflect.Method;
import java.lang.reflect.Modifier;

public final class ClassAnalyzer {
    private final Class<?> klass;

    public ClassAnalyzer(Class<?> klass) {
        this.klass = klass;
    }

    public boolean hasMethod(boolean isStatic, Class<?> returnType, String name,
            Class<?>... parameters) {
        try {
            Method candidate = klass.getMethod(name, parameters);
            int modifier = candidate.getModifiers();
            Class<?> actualReturnType = candidate.getReturnType();
            boolean satisfiesStatic = isStatic == Modifier.isStatic(modifier);
            return satisfiesStatic && returnType.equals(actualReturnType);
        } catch (NoSuchMethodException e) {
            return false;
        }
    }
}
