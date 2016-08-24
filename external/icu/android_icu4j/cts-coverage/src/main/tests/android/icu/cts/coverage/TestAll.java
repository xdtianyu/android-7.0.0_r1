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
package android.icu.cts.coverage;

import org.junit.runner.RunWith;
import org.junit.runners.Suite;

/**
 * A suite of all the tests in this package and child packages.
 */
@RunWith(Suite.class)
// Add classes in alphabetical order with a trailing comma even if it's the last entry.
@Suite.SuiteClasses({
        android.icu.cts.coverage.lang.TestAll.class,
        android.icu.cts.coverage.text.TestAll.class,
        android.icu.cts.coverage.util.TestAll.class,
})
public class TestAll {
}
