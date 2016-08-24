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

package android.icu.cts;

import android.os.Bundle;
import com.android.cts.core.runner.CoreTestRunner;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;

/**
 * Customize {@link CoreTestRunner} to hard code information in the runner.
 *
 * <p>CTSv2 allows parameters to be specified in the AndroidTest.xml file but CTSv1 does not so
 * they have to be hard coded into the runner itself.
 */
public final class IcuTestRunnerForCtsV1 extends CoreTestRunner {

    private static final List<String> EXPECTATIONS_PATHS =
            Collections.singletonList("expectations/icu-known-failures.txt");

    @Override
    protected List<String> getExpectationResourcePaths(Bundle args) {
        return EXPECTATIONS_PATHS;
    }

    @Override
    protected List<String> getRootClassNames(Bundle args) {
        return Arrays.asList(
                "android.icu.cts.coverage.TestAll",
                "android.icu.dev.test.TestAll");
    }
}
