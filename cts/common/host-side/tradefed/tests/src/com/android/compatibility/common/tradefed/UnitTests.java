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
package com.android.compatibility.common.tradefed;

import com.android.compatibility.common.tradefed.build.CompatibilityBuildHelperTest;
import com.android.compatibility.common.tradefed.command.CompatibilityConsoleTest;
import com.android.compatibility.common.tradefed.result.ConsoleReporterTest;
import com.android.compatibility.common.tradefed.result.ResultReporterTest;
import com.android.compatibility.common.tradefed.targetprep.PropertyCheckTest;
import com.android.compatibility.common.tradefed.targetprep.SettingsPreparerTest;
import com.android.compatibility.common.tradefed.testtype.CompatibilityTestTest;
import com.android.compatibility.common.tradefed.testtype.ModuleDefTest;
import com.android.compatibility.common.tradefed.testtype.ModuleRepoTest;
import com.android.compatibility.common.tradefed.util.OptionHelperTest;

import junit.framework.Test;
import junit.framework.TestSuite;

/**
 * A test suite for all compatibility tradefed unit tests.
 * <p/>
 * All tests listed here should be self-contained, and do not require any external dependencies.
 */
public class UnitTests extends TestSuite {

    public UnitTests() {
        super();
        addTestSuite(CompatibilityBuildHelperTest.class);
        addTestSuite(CompatibilityConsoleTest.class);
        addTestSuite(CompatibilityTestTest.class);
        addTestSuite(ConsoleReporterTest.class);
        addTestSuite(ResultReporterTest.class);
        addTestSuite(CompatibilityTestTest.class);
        addTestSuite(OptionHelperTest.class);
        addTestSuite(ModuleDefTest.class);
        addTestSuite(ModuleRepoTest.class);
        addTestSuite(PropertyCheckTest.class);
        addTestSuite(SettingsPreparerTest.class);
    }

    public static Test suite() {
        return new UnitTests();
    }
}
