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

/**
 * TestProperties is a common class of constants shared between the
 * Vogar on the host and TestRunner classes potentially running
 * on other devices.
 */
final public class TestProperties {

    /**
     * The name of the test properties file within the {@code .jar} file.
     */
    public static final String FILE = "test.properties";

    /**
     * Name of the property giving the class name or package name of the test to be performed.
     */
    public static final String TEST_CLASS_OR_PACKAGE = "testClassOrPackage";

    /**
     * Name of the property giving the test's name, such as {@code
     * java.math.BigDecimal.PowTests}.
     */
    public static final String QUALIFIED_NAME = "qualifiedName";

    /**
     * Port to accept monitor connections on.
     */
    public static final String MONITOR_PORT = "monitorPort";

    /**
     * Integer timeout in seconds
     */
    public static final String TIMEOUT = "timeout";

    /**
     * Profile enabled?
     */
    public static final String PROFILE = "profile";

    /**
     * Profile frame depth
     */
    public static final String PROFILE_DEPTH = "profileDepth";

    /**
     * Profile interval in milliseconds
     */
    public static final String PROFILE_INTERVAL = "profileInterval";

    /**
     * Profile output file
     */
    public static final String PROFILE_FILE = "profileFile";

    /**
     * Profile thread group instead of just single thread (where supported)
     */
    public static final String PROFILE_THREAD_GROUP = "profileThreadGroup";

    /**
     * Only run JUnit tests?
     */
    public static final String TEST_ONLY = "testOnly";

    private TestProperties() {}
}
