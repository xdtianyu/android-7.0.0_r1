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

package vogar.target;

import java.io.ByteArrayOutputStream;
import java.io.PrintStream;
import java.util.List;
import java.util.concurrent.atomic.AtomicReference;
import junit.framework.TestCase;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import vogar.Result;
import vogar.monitor.TargetMonitor;
import vogar.target.junit.JUnitRunner;
import vogar.target.junit.JUnitRunnerFactory;
import vogar.target.junit.VogarTest;
import vogar.target.junit3.FailTest;
import vogar.target.junit3.LongTest;
import vogar.target.junit3.LongTest2;
import vogar.target.junit3.SimpleTest;
import vogar.target.junit3.SimpleTest2;
import vogar.target.junit3.SuiteTest;

/**
 * Test of {@link JUnitRunner}
 */
public class JUnitRunnerTest extends TestCase {
    private static final String[] EMPTY_ARGS = {};
    private TargetMonitor monitor;
    private TestEnvironment testEnvironment = new TestEnvironment();
    private final AtomicReference<String> skipPastReference = new AtomicReference<>();

    public void setUp() {
        monitor = mock(TargetMonitor.class);
    }

    public void test_run_for_SimpleTest_should_perform_test() {
        Class<?> target = SimpleTest.class;
        List<VogarTest> tests = JUnitRunnerFactory.createVogarTests(target, null, EMPTY_ARGS);
        Runner runner = new JUnitRunner(monitor, skipPastReference, testEnvironment, 0, tests);
        runner.run(null);

        verify(monitor).outcomeStarted(JUnitRunner.class,
                target.getName() + "#testSimple");
        verify(monitor).outcomeFinished(Result.SUCCESS);
    }

    public void test_run_for_SuiteTest_should_perform_tests() {
        Class<?> target = SuiteTest.class;
        List<VogarTest> tests = JUnitRunnerFactory.createVogarTests(target, null, EMPTY_ARGS);
        Runner runner = new JUnitRunner(monitor, skipPastReference, testEnvironment, 0, tests);
        runner.run(null);

        verify(monitor).outcomeStarted(JUnitRunner.class,
                "vogar.target.junit3.SimpleTest#testSimple");
        verify(monitor).outcomeStarted(JUnitRunner.class,
                "vogar.target.junit3.SimpleTest2#testSimple1");
        verify(monitor).outcomeStarted(JUnitRunner.class,
                "vogar.target.junit3.SimpleTest2#testSimple2");
        verify(monitor).outcomeStarted(JUnitRunner.class,
                "vogar.target.junit3.SimpleTest2#testSimple3");
        verify(monitor, times(4)).outcomeFinished(Result.SUCCESS);
    }

    public void test_run_for_SimpleTest2_with_ActionName_should_perform_test() {
        Class<?> target = SimpleTest2.class;
        List<VogarTest> tests = JUnitRunnerFactory.createVogarTests(target, null, EMPTY_ARGS);
        Runner runner = new JUnitRunner(monitor, skipPastReference, testEnvironment, 0, tests);
        runner.run(null);

        verify(monitor).outcomeStarted(JUnitRunner.class,
                target.getName() + "#testSimple1");
        verify(monitor).outcomeStarted(JUnitRunner.class,
                target.getName() + "#testSimple2");
        verify(monitor).outcomeStarted(JUnitRunner.class,
                target.getName() + "#testSimple3");
        verify(monitor, times(3)).outcomeFinished(Result.SUCCESS);
    }

    public void test_run_for_SimpleTest2_limiting_to_1method_should_perform_test() {
        Class<?> target = SimpleTest2.class;
        List<VogarTest> tests =
                JUnitRunnerFactory.createVogarTests(target, null,  new String[] { "testSimple2" });
        Runner runner = new JUnitRunner(monitor, skipPastReference, testEnvironment, 0, tests);
        runner.run(null);

        verify(monitor).outcomeStarted(JUnitRunner.class,
                target.getName() + "#testSimple2");
        verify(monitor).outcomeFinished(Result.SUCCESS);
    }

    public void test_run_for_SimpleTest2_limiting_to_2methods_should_perform_test() {
        Class<?> target = SimpleTest2.class;
        List<VogarTest> tests = JUnitRunnerFactory.createVogarTests(target, null,
                new String[] { "testSimple2", "testSimple3" });
        Runner runner = new JUnitRunner(monitor, skipPastReference, testEnvironment, 0, tests);
        runner.run(null);

        verify(monitor).outcomeStarted(JUnitRunner.class,
                target.getName() + "#testSimple2");
        verify(monitor).outcomeStarted(JUnitRunner.class,
                target.getName() + "#testSimple3");
        verify(monitor, times(2)).outcomeFinished(Result.SUCCESS);
    }

    public void test_limiting_to_1method_and_run_for_SimpleTest2_should_perform_test() {
        Class<?> target = SimpleTest2.class;
        List<VogarTest> tests =
                JUnitRunnerFactory.createVogarTests(target, "testSimple2", EMPTY_ARGS);
        Runner runner = new JUnitRunner(monitor, skipPastReference, testEnvironment, 0, tests);
        runner.run(null);

        verify(monitor).outcomeStarted(JUnitRunner.class,
                target.getName() + "#testSimple2");
        verify(monitor).outcomeFinished(Result.SUCCESS);
    }

    public void test_limiting_to_wrong_1method_and_run_for_SimpleTest2_should_fail_test() {
        Class<?> target = SimpleTest2.class;
        ByteArrayOutputStream baos = new ByteArrayOutputStream();
        System.setOut(new PrintStream(baos));

        List<VogarTest> tests =
                JUnitRunnerFactory.createVogarTests(target, "testSimple5", EMPTY_ARGS);
        Runner runner = new JUnitRunner(monitor, skipPastReference, testEnvironment, 0, tests);
        runner.run(null);

        verify(monitor).outcomeStarted(JUnitRunner.class,
                target.getName() + "#testSimple5");
        verify(monitor).outcomeFinished(Result.EXEC_FAILED);

        String outStr = baos.toString();
        assertTrue(outStr
                .contains("junit.framework.AssertionFailedError: Method " + '"'
                        + "testSimple5" + '"' + " not found"));
    }

    public void test_run_for_SimpleTest2_limiting_to_1method_with_both_run_should_perform_test() {
        Class<?> target = SimpleTest2.class;
        List<VogarTest> tests = JUnitRunnerFactory.createVogarTests(target, "testSimple3",
                new String[]{"testSimple2"});
        Runner runner = new JUnitRunner(monitor, skipPastReference, testEnvironment, 0, tests);
        runner.run(null);

        verify(monitor).outcomeStarted(JUnitRunner.class,
                target.getName() + "#testSimple2");
        verify(monitor).outcomeStarted(JUnitRunner.class,
                target.getName() + "#testSimple3");
        verify(monitor, times(2)).outcomeFinished(Result.SUCCESS);
    }

    public void test_run_for_FailTest_should_perform_test() {
        Class<?> target = FailTest.class;

        ByteArrayOutputStream baos = new ByteArrayOutputStream();
        System.setOut(new PrintStream(baos));

        List<VogarTest> tests = JUnitRunnerFactory.createVogarTests(target, null, EMPTY_ARGS);
        Runner runner = new JUnitRunner(monitor, skipPastReference, testEnvironment, 0, tests);
        runner.run(null);

        verify(monitor).outcomeStarted(JUnitRunner.class,
                target.getName() + "#testSuccess");
        verify(monitor).outcomeStarted(JUnitRunner.class, target.getName() + "#testFail");
        verify(monitor).outcomeStarted(JUnitRunner.class,
                target.getName() + "#testThrowException");
        verify(monitor).outcomeFinished(Result.SUCCESS);
        verify(monitor, times(2)).outcomeFinished(Result.EXEC_FAILED);

        String outStr = baos.toString();
        assertTrue(outStr
                .contains("junit.framework.AssertionFailedError: failed."));
        assertTrue(outStr.contains("java.lang.RuntimeException: exception"));
    }

    public void test_run_for_LongTest_with_time_limit_should_report_time_out() {
        Class<?> target = LongTest.class;

        ByteArrayOutputStream baos = new ByteArrayOutputStream();
        System.setOut(new PrintStream(baos));

        List<VogarTest> tests = JUnitRunnerFactory.createVogarTests(target, null, EMPTY_ARGS);
        Runner runner = new JUnitRunner(monitor, skipPastReference, testEnvironment, 1, tests);
        runner.run(null);

        verify(monitor).outcomeStarted(JUnitRunner.class, target.getName() + "#test");
        verify(monitor).outcomeFinished(Result.EXEC_FAILED);

        String outStr = baos.toString();
        assertTrue(outStr.contains("java.util.concurrent.TimeoutException"));
    }

    public void test_run_for_LongTest2_without_time_limit_should_not_report_time_out() {
        Class<?> target = LongTest2.class;

        List<VogarTest> tests = JUnitRunnerFactory.createVogarTests(target, null, EMPTY_ARGS);
        Runner runner = new JUnitRunner(monitor, skipPastReference, testEnvironment, 0, tests);
        runner.run(null);

        verify(monitor, times(8)).outcomeFinished(Result.SUCCESS);
    }
}
