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

package org.apache.harmony.jpda.tests.jdwp.share;

import org.apache.harmony.jpda.tests.framework.TestErrorException;
import org.apache.harmony.jpda.tests.framework.jdwp.CommandPacket;
import org.apache.harmony.jpda.tests.framework.jdwp.JDWPConstants;
import org.apache.harmony.jpda.tests.framework.jdwp.ReplyPacket;
import org.apache.harmony.jpda.tests.jdwp.share.debuggee.InvokeMethodWithSuspensionDebuggee;
import org.apache.harmony.jpda.tests.share.JPDADebuggeeSynchronizer;

import java.io.IOException;

/**
 * Base class for tests checking invoke command with thread suspension.
 */
public abstract class JDWPInvokeMethodWithSuspensionTestCase extends JDWPSyncTestCase {

    @Override
    protected final String getDebuggeeClassName() {
        return InvokeMethodWithSuspensionDebuggee.class.getName();
    }

    /**
     * This methods runs the {@link InvokeMethodWithSuspensionDebuggee} then sets up the
     * following breakpoints:
     * - breakpoint #1 to suspend the current thread only when the debuggee starts
     * - breakpoint #2 to suspend all threads from a new thread started by a method called through
     * JDWP.
     * When we receive the event for breakpoint #1, we issue a request to invoke a method (or a
     * constructor) in the event thread (suspended on the breakpoint). The event thread starts
     * another child thread and loops as long as the child is running. However, we do not read the
     * reply of the invoke yet.
     * Next, we wait for the new thread to hit breakpoint #2. We resume all threads in the debuggee
     * and read the reply of the invoke.
     * Finally, we resume the thread that completed the invoke.
     *
     * @param invokedMethodName
     *          the name of the method to invoke
     */
    protected void runInvokeMethodTest(String invokedMethodName) {
        // Wait for debuggee to start.
        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_READY);

        long classID = getClassIDBySignature(getDebuggeeClassSignature());
        long invokeMethodID = getMethodID(classID, invokedMethodName);

        // Set breakpoint with EVENT_THREAD suspend policy so only the event thread is suspended.
        // We will invoke the method in this thread.
        int breakpointEventThread = debuggeeWrapper.vmMirror.setBreakpointAtMethodBegin(classID,
                InvokeMethodWithSuspensionDebuggee.BREAKPOINT_EVENT_THREAD_METHOD_NAME,
                JDWPConstants.SuspendPolicy.EVENT_THREAD);

        // Set breakpoint with ALL suspend policy to suspend all threads. The thread started
        // during the invoke will suspend all threads, including the thread executing the invoke.
        int breakpointAllThreads = debuggeeWrapper.vmMirror.setBreakpointAtMethodBegin(classID,
                InvokeMethodWithSuspensionDebuggee.BREAKPOINT_ALL_THREADS_METHOD_NAME,
                JDWPConstants.SuspendPolicy.ALL);

        // Tell the debuggee to continue.
        synchronizer.sendMessage(JPDADebuggeeSynchronizer.SGNL_CONTINUE);

        // Wait for breakpoint and get id of suspended thread.
        long eventThreadOne = debuggeeWrapper.vmMirror.waitForBreakpoint(breakpointEventThread);
        int suspendCount = debuggeeWrapper.vmMirror.getThreadSuspendCount(eventThreadOne);
        assertEquals("Invalid suspend count:", 1, suspendCount);

        // Send command but does not read reply now. That invoked method starts another thread
        // that is going to hit a breakpoint and suspend all threads, including the thread invoking
        // the method. The invoke can only complete when that new thread terminates, which requires
        // we send a VirtualMachine.Resume command.
        final int invoke_options = 0;  // resume/suspend all threads before/after the invoke.
        CommandPacket invokeMethodCommand = buildInvokeCommand(eventThreadOne, classID,
                invokeMethodID, invoke_options);

        String commandName = getInvokeCommandName();
        logWriter.println("Send " + commandName);
        int invokeMethodCommandID = -1;
        try {
            invokeMethodCommandID = debuggeeWrapper.vmMirror.sendCommand(invokeMethodCommand);
        } catch (IOException e) {
            logWriter.printError("Failed to send " + commandName, e);
            fail();
        }

        // Wait for 2nd breakpoint to hit.
        long eventThreadTwo = debuggeeWrapper.vmMirror.waitForBreakpoint(breakpointAllThreads);
        suspendCount = debuggeeWrapper.vmMirror.getThreadSuspendCount(eventThreadTwo);
        assertEquals("Invalid suspend count:", 1, suspendCount);

        // At this point, the event thread #1 must have been suspended by event thread #2. Since
        // the invoke has resumed it too, its suspend count must remain 1.
        suspendCount = debuggeeWrapper.vmMirror.getThreadSuspendCount(eventThreadOne);
        assertEquals("Invalid suspend count:", 1, suspendCount);

        // Test that sending another invoke command in the same thread returns an error.
        CommandPacket anotherInvokeMethodCommand = buildInvokeCommand(eventThreadOne, classID,
                invokeMethodID, invoke_options);
        ReplyPacket anotherInvokeMethodReply =
                debuggeeWrapper.vmMirror.performCommand(anotherInvokeMethodCommand);
        // The RI returns INVALID_THREAD error while ART returns ALREADY_INVOKING error which is
        // more accurate. We only test we get an error so we can run the test with both runtimes.
        assertTrue("Expected an error",
                anotherInvokeMethodReply.getErrorCode() != JDWPConstants.Error.NONE);

        // Send a VirtualMachine.Resume to resume all threads. This will unblock the event thread
        // with the invoke in-progress.
        logWriter.println("Resume all threads");
        resumeDebuggee();

        // Now we can read the invoke reply.
        ReplyPacket invokeMethodReply = null;
        try {
            logWriter.println("Receiving reply for command " + invokeMethodCommandID + " ...");
            invokeMethodReply = debuggeeWrapper.vmMirror.receiveReply(invokeMethodCommandID);
        } catch (Exception e) {
            throw new TestErrorException("Did not receive invoke reply", e);
        }
        checkReplyPacket(invokeMethodReply, commandName + " command");
        logWriter.println("Received reply for command " + invokeMethodCommandID + " OK");

        checkInvokeReply(invokeMethodReply);

        // The invoke is complete but the thread is still suspended: let's resume it now.
        suspendCount = debuggeeWrapper.vmMirror.getThreadSuspendCount(eventThreadOne);
        assertEquals("Invalid suspend count:", 1, suspendCount);

        logWriter.println("Resume event thread #1");
        debuggeeWrapper.vmMirror.resumeThread(eventThreadOne);
    }

    /**
     * Builds the packed for the tested JDWP command.
     *
     * @param threadId
     *          the id of the thread that will invoke the method
     * @param classID
     *          the class ID of the invoked method
     * @param methodId
     *          the ID of the invoke method
     * @param invokeOptions
     *          options for the invoke
     * @return a command
     */
    protected abstract CommandPacket buildInvokeCommand(long threadId, long classID,
                                                        long methodId, int invokeOptions);

    /**
     * Returns the name of the command returned by {@link #buildInvokeCommand} for printing.
     *
     * @return the name of the invoke command sent to the debuggee
     */
    protected abstract String getInvokeCommandName();

    /**
     * Checks the reply for the tested JDWP command.
     *
     * @param reply the reply of the invoke
     */
    protected abstract void checkInvokeReply(ReplyPacket reply);

}
