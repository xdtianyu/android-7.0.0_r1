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
public class InvokeMethodSuspendedTwiceDebuggee extends SyncDebuggee {
    // Information for the test.
    public static final String STATIC_METHOD_NAME = "invokedStaticMethod";
    public static final String INSTANCE_METHOD_NAME = "invokedInstanceMethod";
    public static final String BREAKPOINT_EVENT_THREAD_METHOD_NAME = "breakpointEventThread";

    // Invoked to suspend main thread (event thread #1) on a breakpoint.
    private void breakpointEventThread() {
    }

    // Static method to test ClassType.InvokeMethod.
    public static void invokedStaticMethod() {
    }

    // Constructor to test ClassType.NewInstance.
    public InvokeMethodSuspendedTwiceDebuggee() {
    }

    // Instance method to test ObjectReference.InvokeMethod.
    public void invokedInstanceMethod() {
    }

    @Override
    public void run() {
        logWriter.println("InvokeMethodSuspendedTwiceDebuggee starts");

        synchronizer.sendMessage(JPDADebuggeeSynchronizer.SGNL_READY);

        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_CONTINUE);

        // We want to suspend the main thread on a breakpoint.
        logWriter.println("Breakpoint for event thread #1");
        breakpointEventThread();

        logWriter.println("InvokeMethodSuspendedTwiceDebuggee ends");
    }

    public static void main(String[] args) {
        runDebuggee(InvokeMethodSuspendedTwiceDebuggee.class);
    }
}
