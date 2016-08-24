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

import org.apache.harmony.jpda.tests.framework.TestErrorException;
import org.apache.harmony.jpda.tests.share.JPDADebuggeeSynchronizer;
import org.apache.harmony.jpda.tests.share.SyncDebuggee;

/**
 * Debuggee for {@link DisposeDuringInvokeTest}.
 */
public class DisposeDuringInvokeDebuggee extends SyncDebuggee {
    public static final String INVOKED_METHOD_NAME = "invokeMethodWithSynchronization";
    public static final String BREAKPOINT_METHOD_NAME = "breakpointMethod";
    public static final String THIS_FIELD_NAME = "thisObject";

    // This field holds the receiver to invoke the invokeMethodWithSynchronization method.
    public static DisposeDuringInvokeDebuggee thisObject;

    private class DebuggeeThread extends Thread {
        public DebuggeeThread() {
            super("DebuggeeThread");
        }

        public void run() {
            breakpointMethod();
        }
    }

    /**
     * The method used to suspend the DebuggeeThread on a breakpoint.
     */
    private void breakpointMethod() {
    }

    /**
     * The method called by the DebuggeeThread through JDWP.
     */
    private void invokeMethodWithSynchronization() {
        logWriter.println("#START invokeMethodWithSynchronization");

        // Tell the test we are invoking the requested method.
        synchronizer.sendMessage(JPDADebuggeeSynchronizer.SGNL_READY);

        // Wait for the test to send a VirtualMachine.Dispose command and resume us.
        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_CONTINUE);

        logWriter.println("#END invokeMethodWithSynchronization");
    }

    @Override
    public void run() {
        thisObject = this;

        DebuggeeThread thrd = new DebuggeeThread();
        synchronizer.sendMessage(JPDADebuggeeSynchronizer.SGNL_READY);

        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_CONTINUE);

        logWriter.println("Start DebuggeeThread");
        thrd.start();

        logWriter.println("Main thread waits for DebuggeeThread ...");
        try {
            thrd.join();
        } catch (InterruptedException e) {
           throw new TestErrorException(e);
        }
        logWriter.println("DebuggeeThread is finished");

        // Tell the test we successfully waited for the end of DebuggeeThread.
        synchronizer.sendMessage(JPDADebuggeeSynchronizer.SGNL_READY);

        // Wait for the test to resume us.
        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_CONTINUE);
    }

    public static void main(String [] args) {
        runDebuggee(DisposeDuringInvokeDebuggee.class);
    }
}
