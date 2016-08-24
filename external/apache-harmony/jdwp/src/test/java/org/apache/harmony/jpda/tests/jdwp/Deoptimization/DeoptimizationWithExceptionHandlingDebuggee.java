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

package org.apache.harmony.jpda.tests.jdwp.Deoptimization;

import org.apache.harmony.jpda.tests.share.JPDADebuggeeSynchronizer;
import org.apache.harmony.jpda.tests.share.SyncDebuggee;

/**
 * Debuggee for DeoptimizationWithExceptionHandlingTest.
 */
public class DeoptimizationWithExceptionHandlingDebuggee extends SyncDebuggee {
    public static final int SUCCESS_RESULT = 1;
    public static final int FAILURE_RESULT = 0;

    public static void main(String[] args) {
        runDebuggee(DeoptimizationWithExceptionHandlingDebuggee.class);
    }

    public void run() {
        logWriter.println("--> DeoptimizationWithExceptionHandlingDebuggee started");
        synchronizer.sendMessage(JPDADebuggeeSynchronizer.SGNL_READY);

        // Wait for test setup
        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_CONTINUE);

        int result = runTest();
        logWriter.println("Result == " + result);

        // Send result to test.
        synchronizer.sendMessage(Integer.toString(result));

        logWriter.println("--> DeoptimizationWithExceptionHandlingDebuggee finished");
    }

    /**
     * This method will catch the NullPointerException after deoptimization happened.
     */
    private static int runTest() {
        int result = FAILURE_RESULT;
        try {
            runBreakpointMethodThenThrowNPE();
            result = FAILURE_RESULT;
        } catch (NullPointerException e) {
            result = SUCCESS_RESULT;
        }
        return result;
    }

    /**
     * This method will throw a NullPointerException after deoptimization happened.
     */
    private static void runBreakpointMethodThenThrowNPE() {
        runBreakpointMethod();
        throw new NullPointerException();
    }

    /**
     * This method will be the starting point of stack deoptimization.
     */
    private static void runBreakpointMethod() {
        breakpointMethod();
    }

    private static void breakpointMethod() {
        System.out.println("breakpointMethod");
    }

}
