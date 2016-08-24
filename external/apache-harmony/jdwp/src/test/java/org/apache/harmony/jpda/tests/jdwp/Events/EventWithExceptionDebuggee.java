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

package org.apache.harmony.jpda.tests.jdwp.Events;

import org.apache.harmony.jpda.tests.share.JPDADebuggeeSynchronizer;
import org.apache.harmony.jpda.tests.share.SyncDebuggee;

/**
 * Debuggee for {@link EventWithExceptionTest} class.
 */
public class EventWithExceptionDebuggee extends SyncDebuggee {
    // Field used to test FIELD_ACCESS and FIELD_WATCHPOINT events.
    public static int watchedField = 0;

    // Exception class for EXCEPTION event.
    static final class TestException extends Exception {
    }

    public int catchMethod() {
        try {
            syncAndThrow();
        } catch (TestException e) {
            logWriter.println("Expected exception: " + e);  // SINGLE-STEP & BREAKPOINT
        }
        watchedField = 10;                                  // FIELD_MODIFICATION
        int result = watchedField;                          // FIELD_ACCESS
        return result;                                      // METHOD_EXIT[WITH_RETURN_VALUE]
    }

    public void syncAndThrow() throws TestException {
        syncAndThrowImpl();
    }

    public void syncAndThrowImpl() throws TestException {
        sync();
        throw new TestException();
    }

    private void sync() {
        synchronizer.sendMessage(JPDADebuggeeSynchronizer.SGNL_READY);
        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_CONTINUE);
    }

    @Override
    public void run() {
        // Cause class loading so that it is visible from the test.
        new TestException();

        // First run to capture catch handler location (if necessary).
        catchMethod();

        // Second run for testing.
        catchMethod();

        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_CONTINUE);
    }

    public static void main(String[] args) {
        runDebuggee(EventWithExceptionDebuggee.class);
    }
}
