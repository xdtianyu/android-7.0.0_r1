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

package vogar.target.junit;

import java.util.List;
import java.util.concurrent.Callable;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;
import java.util.concurrent.atomic.AtomicReference;

import vogar.Result;
import vogar.monitor.TargetMonitor;
import vogar.target.Profiler;
import vogar.target.Runner;
import vogar.target.TestEnvironment;
import vogar.util.Threads;

/**
 * Adapts a JUnit3 test for use by vogar.
 */
public final class JUnitRunner implements Runner {

    private final TargetMonitor monitor;
    private final AtomicReference<String> skipPastReference;

    private final TestEnvironment testEnvironment;
    private final int timeoutSeconds;
    private boolean vmIsUnstable;
    private final List<VogarTest> tests;

    private final ExecutorService executor = Executors.newCachedThreadPool(
            Threads.daemonThreadFactory("testrunner"));

    public JUnitRunner(TargetMonitor monitor, AtomicReference<String> skipPastReference,
                       TestEnvironment testEnvironment, int timeoutSeconds, List<VogarTest> tests) {
        this.monitor = monitor;
        this.skipPastReference = skipPastReference;
        this.testEnvironment = testEnvironment;
        this.timeoutSeconds = timeoutSeconds;
        this.tests = tests;
    }

    public boolean run(Profiler profiler) {
        for (VogarTest test : tests) {
            String skipPast = skipPastReference.get();
            if (skipPast != null) {
                if (skipPast.equals(test.toString())) {
                    skipPastReference.set(null);
                }
                continue;
            }

            runWithTimeout(profiler, test);

            if (vmIsUnstable) {
                return false;
            }
        }

        return true;
    }

    /**
     * Runs the test on another thread. If the test completes before the
     * timeout, this reports the result normally. But if the test times out,
     * this reports the timeout stack trace and begins the process of killing
     * this no-longer-trustworthy process.
     */
    private void runWithTimeout(final Profiler profiler, final VogarTest test) {
        testEnvironment.reset();
        monitor.outcomeStarted(getClass(), test.toString());

        // Start the test on a background thread.
        final AtomicReference<Thread> executingThreadReference = new AtomicReference<Thread>();
        Future<Throwable> result = executor.submit(new Callable<Throwable>() {
            public Throwable call() throws Exception {
                executingThreadReference.set(Thread.currentThread());
                try {
                    if (profiler != null) {
                        profiler.start();
                    }
                    test.run();
                    return null;
                } catch (Throwable throwable) {
                    return throwable;
                } finally {
                    if (profiler != null) {
                        profiler.stop();
                    }
                }
            }
        });

        // Wait until either the result arrives or the test times out.
        Throwable thrown;
        try {
            thrown = timeoutSeconds == 0
                    ? result.get()
                    : result.get(timeoutSeconds, TimeUnit.SECONDS);
        } catch (TimeoutException e) {
            vmIsUnstable = true;
            Thread executingThread = executingThreadReference.get();
            if (executingThread != null) {
                executingThread.interrupt();
                e.setStackTrace(executingThread.getStackTrace());
            }
            thrown = e;
        } catch (Exception e) {
            thrown = e;
        }

        if (thrown != null) {
            prepareForDisplay(thrown);
            thrown.printStackTrace(System.out);
            monitor.outcomeFinished(Result.EXEC_FAILED);
        } else {
            monitor.outcomeFinished(Result.SUCCESS);
        }
    }

    /**
     * Strip vogar's lines from the stack trace. For example, we'd strip the
     * first two Assert lines and everything after the testFoo() line in this
     * stack trace:
     *
     *   at junit.framework.Assert.fail(Assert.java:198)
     *   at junit.framework.Assert.assertEquals(Assert.java:56)
     *   at junit.framework.Assert.assertEquals(Assert.java:61)
     *   at libcore.java.net.FooTest.testFoo(FooTest.java:124)
     *   at java.lang.reflect.Method.invokeNative(Native Method)
     *   at java.lang.reflect.Method.invoke(Method.java:491)
     *   at vogar.target.junit.Junit$JUnitTest.run(Junit.java:214)
     *   at vogar.target.junit.JUnitRunner$1.call(JUnitRunner.java:112)
     *   at vogar.target.junit.JUnitRunner$1.call(JUnitRunner.java:105)
     *   at java.util.concurrent.FutureTask$Sync.innerRun(FutureTask.java:305)
     *   at java.util.concurrent.FutureTask.run(FutureTask.java:137)
     *   at java.util.concurrent.ThreadPoolExecutor.runWorker(ThreadPoolExecutor.java:1076)
     *   at java.util.concurrent.ThreadPoolExecutor$Worker.run(ThreadPoolExecutor.java:569)
     *   at java.lang.Thread.run(Thread.java:863)
     */
    public void prepareForDisplay(Throwable t) {
        StackTraceElement[] stackTraceElements = t.getStackTrace();
        boolean foundVogar = false;

        int last = stackTraceElements.length - 1;
        for (; last >= 0; last--) {
            String className = stackTraceElements[last].getClassName();
            if (className.startsWith("vogar.target")) {
                foundVogar = true;
            } else if (foundVogar
                    && !className.startsWith("java.lang.reflect")
                    && !className.startsWith("sun.reflect")
                    && !className.startsWith("junit.framework")) {
                if (last < stackTraceElements.length) {
                    last++;
                }
                break;
            }
        }

        int first = 0;
        for (; first < last; first++) {
            String className = stackTraceElements[first].getClassName();
            if (!className.startsWith("junit.framework")) {
                break;
            }
        }

        if (first > 0) {
            first--; // retain one assertSomething() line in the trace
        }

        if (first < last) {
            // Arrays.copyOfRange() didn't exist on Froyo
            StackTraceElement[] copyOfRange = new StackTraceElement[last - first];
            System.arraycopy(stackTraceElements, first, copyOfRange, 0, last - first);
            t.setStackTrace(copyOfRange);
        }
    }
}
