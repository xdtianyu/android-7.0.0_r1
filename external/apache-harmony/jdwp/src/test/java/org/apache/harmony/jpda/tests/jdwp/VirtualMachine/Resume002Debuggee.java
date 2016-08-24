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

package org.apache.harmony.jpda.tests.jdwp.VirtualMachine;

import org.apache.harmony.jpda.tests.share.JPDADebuggeeSynchronizer;
import org.apache.harmony.jpda.tests.share.SyncDebuggee;

/**
 * Debuggee for Resume002Test.
 */
public class Resume002Debuggee extends SyncDebuggee {
    public static final int THREAD_COUNT = 5;

    // The method used to suspend threads on breakpoint.
    public static void breakpointMethod() {
        String threadName = Thread.currentThread().getName();
        System.out.println(threadName + " enters breakpointMethod");
    }

    class ResumeThread extends Thread {
        public ResumeThread(int i) {
            super("ResumeThread-" + i);
        }

        @Override
        public void run() {
            String threadName = Thread.currentThread().getName();
            logWriter.println("Thread \"" + threadName + "\" starts");
            breakpointMethod();
            logWriter.println("Thread \"" + threadName + "\" ends");
        }

    }

    @Override
    public void run() {
        // Tell the debugger we're ready.
        synchronizer.sendMessage(JPDADebuggeeSynchronizer.SGNL_READY);

        // Wait for the debugger to install breakpoint.
        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_CONTINUE);

        // Create tested threads.
        Thread[] threads = new Thread[THREAD_COUNT];
        for (int i = 0; i < threads.length; ++i) {
            threads[i] = new ResumeThread(i);
        }

        // Start threads.
        logWriter.println("Starting threads");
        for (Thread t : threads) {
            t.start();
        }

        // Wait for all tested threads to finish.
        logWriter.println("Waiting end of threads");
        for (Thread t : threads) {
            try {
                t.join();
            } catch (InterruptedException e) {
                logWriter.printError("Join failed", e);
            }
        }
        logWriter.println("All threads terminated");

        // Tell the debugger all threads terminated.
        synchronizer.sendMessage(JPDADebuggeeSynchronizer.SGNL_READY);
    }

    public static void main(String[] args) {
        runDebuggee(Resume002Debuggee.class);
    }

}
