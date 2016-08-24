/*
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

package org.apache.harmony.jpda.tests.jdwp.share.debuggee;

import org.apache.harmony.jpda.tests.jdwp.share.JDWPInvokeMethodWithSuspensionTestCase;
import org.apache.harmony.jpda.tests.share.JPDADebuggeeSynchronizer;
import org.apache.harmony.jpda.tests.share.SyncDebuggee;

/**
 * Debuggee for subclasses of {@link JDWPInvokeMethodWithSuspensionTestCase}.
 */
public class InvokeMethodWithSuspensionDebuggee extends SyncDebuggee {
    // Information for the test.
    public static final String STATIC_METHOD_NAME = "invokedStaticMethod";
    public static final String INSTANCE_METHOD_NAME = "invokedInstanceMethod";
    public static final String BREAKPOINT_EVENT_THREAD_METHOD_NAME = "breakpointEventThread";
    public static final String BREAKPOINT_ALL_THREADS_METHOD_NAME = "breakpointAllThreads";

    private static volatile boolean testThreadFinished = false;
    private static Thread testThread = null;
    private static boolean enableThreadSuspensionForTesting = false;

    private class TestThread extends Thread {
        public TestThread() {
            super("TestThread");
        }

        @Override
        public void run() {
            logWriter.println("TestThread starts");

            // We're going to suspend all threads in the method below.
            logWriter.println("Breakpoint for event thread #2");
            breakpointAllThreads();

            // The test needs to resume us so the invoke in progress in event thread #1 can
            // complete.
            testThreadFinished = true;

            logWriter.println("TestThread ends");
        }
    }

    // Invoked to suspend main thread (event thread #1) on a breakpoint.
    public void breakpointEventThread() {
    }

    // Invoked to suspend test thread (event thread #2) and all others threads on a breakpoint.
    public void breakpointAllThreads() {
    }

    // Invoked in event thread #1. This will unblock event thread #2 that will suspend all threads
    // including event thread #1. This helps us check that the debugger is not blocked waiting for
    // this invoke.
    private static void causeEventThreadSuspension() {
        if (enableThreadSuspensionForTesting) {
            // Start event thread #2. It's going to hit a breakpoint and suspend us.
            testThread.start();

            // We don't use wait/notify pattern to be sure our loop is active.
            while (!testThreadFinished) {
                try {
                    Thread.sleep(100);
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
            }
        }
    }

    // Static method to test ClassType.InvokeMethod.
    public static void invokedStaticMethod() {
        causeEventThreadSuspension();
    }

    // Constructor to test ClassType.NewInstance.
    public InvokeMethodWithSuspensionDebuggee() {
        causeEventThreadSuspension();
    }

    // Instance method to test ObjectReference.InvokeMethod.
    public void invokedInstanceMethod() {
        causeEventThreadSuspension();
    }

    @Override
    public void run() {
        logWriter.println("InvokeMethodWithThreadSuspensionDebuggee starts");

        synchronizer.sendMessage(JPDADebuggeeSynchronizer.SGNL_READY);

        // Create test thread but do not start it now. It will be started by the invoke from
        // the test through JDWP.
        testThread = new TestThread();

        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_CONTINUE);

        enableThreadSuspensionForTesting = true;

        // We want to suspend the main thread on a breakpoint.
        logWriter.println("Breakpoint for event thread #1");
        breakpointEventThread();

        // Ensure tested thread is finished.
        try {
            testThread.join();
        } catch (InterruptedException e) {
            logWriter.printError("Failed to join tested thread", e);
        }
        testThread = null;

        logWriter.println("InvokeMethodWithThreadSuspensionDebuggee ends");
    }

    public static void main(String[] args) {
        runDebuggee(InvokeMethodWithSuspensionDebuggee.class);
    }
}
