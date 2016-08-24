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
package com.android.compatibility.common.util;

import junit.framework.Test;
import junit.framework.TestSuite;

/**
 * A test suite for all util unit tests.
 * <p/>
 * All tests listed here should be self-contained, and do not require any external dependencies.
 */
public class UnitTests extends TestSuite {

    public UnitTests() {
        super();
        addTestSuite(AbiUtilsTest.class);
        addTestSuite(CaseResultTest.class);
        addTestSuite(DynamicConfigTest.class);
        addTestSuite(MetricsXmlSerializerTest.class);
        addTestSuite(ModuleResultTest.class);
        addTestSuite(MultipartFormTest.class);
        addTestSuite(ReportLogTest.class);
        addTestSuite(StatTest.class);
        addTestSuite(TestFilterTest.class);
        addTestSuite(TestResultTest.class);
        addTestSuite(ResultHandlerTest.class);
    }

    public static Test suite() {
        return new UnitTests();
    }
}