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
import org.apache.harmony.jpda.tests.framework.jdwp.exceptions.TimeoutException;
import org.apache.harmony.jpda.tests.jdwp.share.debuggee.InvokeMethodSuspendedTwiceDebuggee;
import org.apache.harmony.jpda.tests.jdwp.share.debuggee.InvokeMethodWithSuspensionDebuggee;
import org.apache.harmony.jpda.tests.share.JPDADebuggeeSynchronizer;

import java.io.IOException;

/**
 * Base class for tests checking invoke command with thread suspended more than once before the
 * invoke.
 */
public abstract class JDWPInvokeMethodSuspendedTwiceTestCase extends JDWPSyncTestCase {

    @Override
    protected final String getDebuggeeClassName() {
        return InvokeMethodSuspendedTwiceDebuggee.class.getName();
    }

    /**
     * This methods runs the {@link InvokeMethodSuspendedTwiceDebuggee} then sets up a breakpoint
     * to suspend the main thread only and signal it to continue.
     * When the debuggee hits the breakpoint and we receive the event, we clear the breakpoint and
     * suspend the thread with ThreadReference.Suspend command so its suspend count is greater
     * than 1.
     * Then, we send an invoke command for this thread and check we do not receive any reply (as
     * the thread is still suspended) by catching a {@link TimeoutException}.
     * Next, we resume the thread and check we do receive the reply with no timeout.
     *
     * @param invokedMethodName
     *          the name of the method to invoke
     */
    protected void runInvokeMethodTest(String invokedMethodName) {
        // Wait for debuggee to start.
        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_READY);

        long classID = getClassIDBySignature(getDebuggeeClassSignature());
        long invokeMethodID = getMethodID(classID, invokedMethodName);

        // Set breakpoint with EVENT_THREAD suspend policy. We will invoke the method in the thread
        // suspended on this breakpoint.
        int breakpointEventThread = debuggeeWrapper.vmMirror.setBreakpointAtMethodBegin(classID,
                InvokeMethodWithSuspensionDebuggee.BREAKPOINT_EVENT_THREAD_METHOD_NAME,
                JDWPConstants.SuspendPolicy.EVENT_THREAD);

        // Tell the debuggee to continue.
        synchronizer.sendMessage(JPDADebuggeeSynchronizer.SGNL_CONTINUE);

        // Wait for breakpoint and get id of suspended thread.
        long eventThreadOne = debuggeeWrapper.vmMirror.waitForBreakpoint(breakpointEventThread);
        int suspendCount = debuggeeWrapper.vmMirror.getThreadSuspendCount(eventThreadOne);
        assertEquals("Invalid suspend count:", 1, suspendCount);

        // Clear breakpoint
        debuggeeWrapper.vmMirror.clearBreakpoint(breakpointEventThread);

        // Suspend thread again so it's suspend count > 1
        debuggeeWrapper.vmMirror.suspendThread(eventThreadOne);
        suspendCount = debuggeeWrapper.vmMirror.getThreadSuspendCount(eventThreadOne);
        assertEquals("Invalid suspend count:", 2, suspendCount);

        // Send command but does not read the reply yet. That invoked method starts another thread
        // that is going to hit a breakpoint and suspend all threads, including the thread invoking
        // the method. The invoke can only complete when that new thread terminates, which requires
        // we send a VirtualMachine.Resume command.
        final int invoke_options = JDWPConstants.InvokeOptions.INVOKE_SINGLE_THREADED;
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

        // Check that we do not receive any reply.
        ReplyPacket invokeMethodReply = null;
        logWriter.println("Receiving reply for command " + invokeMethodCommandID + " ...");
        try {
            invokeMethodReply = debuggeeWrapper.vmMirror.receiveReply(invokeMethodCommandID);
            fail("#FAILURE: received reply too early (error " +
                    JDWPConstants.Error.getName(invokeMethodReply.getErrorCode()) + ")");
        } catch (TimeoutException e) {
            logWriter.println("OK, did not receive reply for command " + invokeMethodCommandID);
        } catch (InterruptedException e) {
            throw new TestErrorException(e);
        } catch (IOException e) {
            throw new TestErrorException(e);
        }

        // At this point, the event thread #1 should have been resumed once for the invoke. But
        // it still suspended by us.
        suspendCount = debuggeeWrapper.vmMirror.getThreadSuspendCount(eventThreadOne);
        assertEquals("Invalid suspend count:", 1, suspendCount);

        // Test that sending another invoke command in the same thread returns an error.
        CommandPacket anotherInvokeMethodCommand = buildInvokeCommand(eventThreadOne, classID,
                invokeMethodID, invoke_options);
        ReplyPacket anotherInvokeMethodReply =
                debuggeeWrapper.vmMirror.performCommand(anotherInvokeMethodCommand);
        // Note: the RI returns INVALID_THREAD but ALREADY_INVOKING is more explicit.
//        checkReplyPacket(anotherInvokeMethodReply, "Invalid error code",
//                JDWPConstants.Error.ALREADY_INVOKING);

        // Send a ThreadReference.Resume to resume event thread so it can invoke the method.
        logWriter.println("Resume event thread");
        debuggeeWrapper.vmMirror.resumeThread(eventThreadOne);

        // Now we can read the invoke reply.
        invokeMethodReply = null;
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
