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
package android.icu.cts.coverage.rules;

import android.icu.util.ULocale;
import org.junit.rules.MethodRule;
import org.junit.runners.model.FrameworkMethod;
import org.junit.runners.model.Statement;

/**
 * Add this as a rule to a JUnit 4 class to support switching the {@link ULocale#getDefault()} for
 * the duration of the test method.
 *
 * <p>Only affects test methods that are annotated with {@link ULocaleDefault}.
 *
 * @see ULocaleDefault
 */
@SuppressWarnings("deprecation")
public class ULocaleDefaultRule implements MethodRule {

    @Override
    public Statement apply(final Statement base, FrameworkMethod method, Object target) {
        final ULocaleDefault annotation = method.getAnnotation(ULocaleDefault.class);
        if (annotation == null) {
            return base;
        } else {
            return new Statement() {
                @Override
                public void evaluate() throws Throwable {
                    ULocale previousDefault = ULocale.getDefault();
                    try {
                        ULocale.setDefault(ULocale.forLanguageTag(annotation.languageTag()));
                        base.evaluate();
                    } finally {
                        ULocale.setDefault(previousDefault);
                    }
                }
            };
        }
    }
}
