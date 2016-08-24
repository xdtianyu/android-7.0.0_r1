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

import org.apache.harmony.jpda.tests.framework.TestErrorException;
import org.apache.harmony.jpda.tests.share.JPDADebuggeeSynchronizer;
import org.apache.harmony.jpda.tests.share.SyncDebuggee;

/**
 * Debuggee for SingleStepWithPendingExceptionTest unit test.
 */
public class SingleStepWithPendingExceptionDebuggee extends SyncDebuggee {

    public static void main(String[] args) {
        runDebuggee(SingleStepWithPendingExceptionDebuggee.class);
    }

    public static class DebuggeeException extends Exception {
    }

    public void catchMethod() {
        System.out.println("SingleStepWithPendingExceptionDebuggee.catchMethod()");
        try {
            throwMethod();
            logWriter.printError("Exception has not been thrown");
            throw new TestErrorException("Single-step clobbered exception");
        } catch (DebuggeeException expected) {
        }
    }

    public void throwMethod() throws DebuggeeException {
        throw new DebuggeeException();
    }

    @Override
    public void run() {
        synchronizer.sendMessage(JPDADebuggeeSynchronizer.SGNL_READY);

        logWriter.println("SingleStepWithPendingExceptionDebuggee started");

        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_CONTINUE);

        // Executes catchMethod once to let the test know the location of the catch handler.
        catchMethod();

        // Executes catchMethod another time for the single-step test this time.
        catchMethod();

        logWriter.println("SingleStepWithPendingExceptionDebuggee finished");
    }

}
