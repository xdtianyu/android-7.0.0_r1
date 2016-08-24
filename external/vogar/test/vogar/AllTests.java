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

package vogar;

import org.junit.runner.RunWith;
import org.junit.runners.Suite;
import org.junit.runners.Suite.SuiteClasses;
import vogar.android.DeviceRuntimeAdbTargetTest;
import vogar.android.DeviceRuntimeSshTargetTest;
import vogar.android.HostRuntimeLocalTargetTest;
import vogar.target.AssertTest;
import vogar.target.JUnitRunnerTest;
import vogar.target.TestRunnerTest;

/**
 * Run the selection of tests that we know work.
 *
 * <p>This is needed as some of the tests do not run on the host. The tests will be removed if/when
 * we switch over to use standard JUnit.
 */
@SuiteClasses({
        AssertTest.class,
        DeviceRuntimeAdbTargetTest.class,
        DeviceRuntimeSshTargetTest.class,
        HostRuntimeLocalTargetTest.class,
        JUnitRunnerTest.class,
        ScriptBuilderEscapingTest.class,
        TestRunnerTest.class,
})
@RunWith(Suite.class)
public class AllTests {
}
