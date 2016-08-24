/*
 *  Licensed to the Apache Software Foundation (ASF) under one or more
 *  contributor license agreements.  See the NOTICE file distributed with
 *  this work for additional information regarding copyright ownership.
 *  The ASF licenses this file to You under the Apache License, Version 2.0
 *  (the "License"); you may not use this file except in compliance with
 *  the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

package org.apache.harmony.jpda.tests.jdwp.Events;

import org.apache.harmony.jpda.tests.share.JPDADebuggeeSynchronizer;
import org.apache.harmony.jpda.tests.share.SyncDebuggee;

/**
 * Debuggee for JDWP unit tests for CombinedExceptionEventsTest.
 */
public class CombinedExceptionEventsDebuggee extends SyncDebuggee {
    public static final String TEST_CAUGHT_EXCEPTION_SIGNAL = "CAUGHT";
    public static final String TEST_UNCAUGHT_EXCEPTION_SIGNAL = "UNCAUGHT";

    public static class SubDebuggeeException extends DebuggeeException {
        public SubDebuggeeException(String msg) {
            super(msg);
        }
    }

    public static void main(String[] args) {
        runDebuggee(CombinedExceptionEventsDebuggee.class);
    }

    public void run() {
        logWriter.println("-> CombinedExceptionEventsDebuggee: Starting...");

        // Preload exception classes
        new SubDebuggeeException("dummy");

        synchronizer.sendMessage(JPDADebuggeeSynchronizer.SGNL_READY);

        String signalMessage = synchronizer.receiveMessage();
        if (signalMessage.equals(TEST_CAUGHT_EXCEPTION_SIGNAL)) {
            testCaughtException();
        } else {
            testUncaughtException();
        }

        logWriter.println("-> CombinedExceptionEventsDebuggee: Finishing...");
    }

    private void testCaughtException() {
        Thread t = new Thread(new Runnable() {
            public void run() {
                // Catch a different exception to test catch location is
                // properly reported.
                try {
                    throwDebuggeeException("Caught debuggee exception");
                } catch (DebuggeeException e) {
                    // Expected exception
                }
            }
        });
        t.start();

        try {
            t.join();
        } catch (InterruptedException e) {
            logWriter.printError(e);
        }
    }

    private void testUncaughtException() {
        Thread t = new Thread(new Runnable() {
            public void run() {
                // Catch a different exception to test catch location is
                // properly reported.
                try {
                    throwDebuggeeException("Uncaught debuggee exception");
                } catch (NullPointerException e) {
                    // Unexpect exception
                    logWriter.printError("Unexpected exception", e);
                }
            }
        });
        t.start();

        try {
            t.join();
        } catch (InterruptedException e) {
            logWriter.printError(e);
        }
    }

    private static void throwDebuggeeException(String msg) {
        throw new SubDebuggeeException(msg);
    }
}
