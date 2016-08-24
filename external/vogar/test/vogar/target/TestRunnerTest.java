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

package vogar.target;

import com.google.caliper.Benchmark;
import com.google.caliper.runner.UserCodeException;
import junit.framework.TestCase;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import vogar.target.junit.JUnitRunner;
import vogar.testing.InterceptOutputStreams;
import vogar.testing.InterceptOutputStreams.Stream;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

/**
 * Tests for {@link TestRunner}
 */
@RunWith(JUnit4.class)
public class TestRunnerTest {

    @Rule public InterceptOutputStreams ios = new InterceptOutputStreams(Stream.OUT);

    @Rule public TestRunnerRule testRunnerRule = new TestRunnerRule();

    @TestRunnerProperties(testClass = JUnit3Test.class)
    @Test
    public void testConstructor_JUnit3Test() throws Exception {
        TestRunner runner = testRunnerRule.createTestRunner();
        runner.run();

        assertEquals(""
                + "//00xx{\"outcome\":\"" + JUnit3Test.class.getName() + "#testMethodName\","
                + "\"runner\":\"" + JUnitRunner.class.getName() + "\"}\n"
                + "//00xx{\"result\":\"SUCCESS\"}\n"
                + "//00xx{\"outcome\":\"" + JUnit3Test.class.getName() + "#testOtherName\","
                + "\"runner\":\"" + JUnitRunner.class.getName() + "\"}\n"
                + "//00xx{\"result\":\"SUCCESS\"}\n"
                + "//00xx{\"completedNormally\":true}\n", ios.contents(Stream.OUT));
    }

    /**
     * Make sure that the {@code --monitorPort <port>} command line option overrides the default
     * specified in the properties.
     */
    @TestRunnerProperties(testClassOrPackage = "vogar.DummyTest", monitorPort = 2345)
    @Test
    public void testConstructor_MonitorPortOverridden() throws Exception {
        TestRunner runner = testRunnerRule.createTestRunner();
        assertEquals(2345, (int) runner.monitorPort);

        runner = testRunnerRule.createTestRunner("--monitorPort", "10");
        assertEquals(10, (int) runner.monitorPort);
    }

    @TestRunnerProperties(testClass = JUnit3Test.class)
    @Test
    public void testConstructor_SkipPastJUnitRunner() throws Exception {
        String failingTestName = JUnit3Test.class.getName() + "#testMethodName";
        TestRunner runner = testRunnerRule.createTestRunner("--skipPast", failingTestName);
        String skipPast = runner.skipPastReference.get();
        assertEquals(failingTestName, skipPast);

        runner.run();
        assertEquals(""
                + "//00xx{\"outcome\":\"" + JUnit3Test.class.getName() + "#testOtherName\","
                + "\"runner\":\"" + JUnitRunner.class.getName() + "\"}\n"
                + "//00xx{\"result\":\"SUCCESS\"}\n"
                + "//00xx{\"completedNormally\":true}\n", ios.contents(Stream.OUT));
    }

    public static class JUnit3Test extends TestCase {
        public void testMethodName() {
        }

        public void testOtherName() {
        }
    }

    @TestRunnerProperties(testClass = CaliperBenchmark.class)
    @Test
    public void testConstructor_CaliperBenchmark() throws Exception {
        TestRunner runner = testRunnerRule.createTestRunner("-i", "runtime");
        runner.run();

        String out = ios.contents(Stream.OUT);
        // Remove stack trace from output.
        out = out.replaceAll("\t[^\n]+\\n", "");
        assertEquals(""
                + "//00xx{\"outcome\":\"" + CaliperBenchmark.class.getName() + "\","
                + "\"runner\":\"" + CaliperRunner.class.getName() + "\"}\n"
                + "Experiment selection: \n"
                + "  Benchmark Methods:   [timeMethod]\n"
                + "  Instruments:   [runtime]\n"
                + "  User parameters:   {}\n"
                + "  Virtual machines:  [default]\n"
                + "  Selection type:    Full cartesian product\n"
                + "\n"
                + "This selection yields 1 experiments.\n"
                + UserCodeException.class.getName()
                + ": An exception was thrown from the benchmark code\n"
                + "Caused by: " + IllegalStateException.class.getName()
                + ": " + CaliperBenchmark.CALIPER_BENCHMARK_MESSAGE + "\n"
                + "//00xx{\"result\":\"SUCCESS\"}\n"
                + "//00xx{\"completedNormally\":true}\n", out);
    }

    /**
     * Ensure that requesting profiling doesn't send an invalid option to Caliper.
     *
     * <p>Cannot check that profiling works because it will only work on Android and these tests
     * do not run on android yet.
     */
    @TestRunnerProperties(testClass = CaliperBenchmark.class, profile = true)
    @Test
    public void testConstructor_CaliperBenchmark_Profile() throws Exception {
        TestRunner runner = testRunnerRule.createTestRunner("-i", "runtime");
        runner.run();

        String out = ios.contents(Stream.OUT);

        // Make sure that profiling is requested (even though it's not supported).
        assertTrue(out.startsWith("Profiling is disabled: "));

        // Remove warning about profiling being disabled.
        out = out.replaceAll("^Profiling is disabled:[^\n]+\\n", "");

        // Remove stack trace from output.
        out = out.replaceAll("\t[^\n]+\\n", "");

        assertEquals(""
                + "//00xx{\"outcome\":\"" + CaliperBenchmark.class.getName() + "\","
                + "\"runner\":\"" + CaliperRunner.class.getName() + "\"}\n"
                + "Experiment selection: \n"
                + "  Benchmark Methods:   [timeMethod]\n"
                + "  Instruments:   [runtime]\n"
                + "  User parameters:   {}\n"
                + "  Virtual machines:  [default]\n"
                + "  Selection type:    Full cartesian product\n"
                + "\n"
                + "This selection yields 1 experiments.\n"
                + UserCodeException.class.getName()
                + ": An exception was thrown from the benchmark code\n"
                + "Caused by: " + IllegalStateException.class.getName()
                + ": " + CaliperBenchmark.CALIPER_BENCHMARK_MESSAGE + "\n"
                + "//00xx{\"result\":\"SUCCESS\"}\n"
                + "//00xx{\"completedNormally\":true}\n", out);
    }

    public static class CaliperBenchmark {

        static final String CALIPER_BENCHMARK_MESSAGE = "Aborting test to save time";

        @Benchmark
        public long timeMethod(long reps) {
            throw new IllegalStateException(CALIPER_BENCHMARK_MESSAGE);
        }
    }

    @TestRunnerProperties(testClass = Main.class)
    @Test
    public void testConstructor_Main() throws Exception {
        TestRunner runner = testRunnerRule.createTestRunner();
        runner.run();

        assertEquals(""
                + "//00xx{\"outcome\":\"" + Main.class.getName() + "\","
                + "\"runner\":\"" + MainRunner.class.getName() + "\"}\n"
                + "//00xx{\"result\":\"SUCCESS\"}\n"
                + "//00xx{\"completedNormally\":true}\n", ios.contents(Stream.OUT));
    }

    public static class Main {
        public static void main(String[] args) {
        }
    }

    @TestRunnerProperties(testClass = JUnit3Test.class)
    @Test
    public void testConstructor_WithMethodName() throws Exception {
        String methodName = "testMethodName";
        TestRunner runner = testRunnerRule.createTestRunner(methodName);
        runner.run();

        assertEquals(""
                + "Warning: Arguments are invalid for Caliper: "
                + "Extra stuff, did not expect non-option arguments: [" + methodName + "]\n"
                + "//00xx{\"outcome\":\"" + JUnit3Test.class.getName() + "#" + methodName + "\","
                + "\"runner\":\"" + JUnitRunner.class.getName() + "\"}\n"
                + "//00xx{\"result\":\"SUCCESS\"}\n"
                + "//00xx{\"completedNormally\":true}\n", ios.contents(Stream.OUT));
    }
}
