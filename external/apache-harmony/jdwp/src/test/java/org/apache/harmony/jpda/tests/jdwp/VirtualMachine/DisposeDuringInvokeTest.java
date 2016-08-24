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
import org.apache.harmony.jpda.tests.framework.jdwp.CommandPacket;
import org.apache.harmony.jpda.tests.framework.jdwp.JDWPCommands;
import org.apache.harmony.jpda.tests.framework.jdwp.JDWPConstants;
import org.apache.harmony.jpda.tests.framework.jdwp.Value;
import org.apache.harmony.jpda.tests.jdwp.share.JDWPSyncTestCase;
import org.apache.harmony.jpda.tests.share.JPDADebuggeeSynchronizer;

import java.io.IOException;

/**
 * JDWP Unit test for VirtualMachine.Dispose command while a thread is invoking a method.
 */
public class DisposeDuringInvokeTest extends JDWPSyncTestCase {

    @Override
    protected String getDebuggeeClassName() {
        return DisposeDuringInvokeDebuggee.class.getName();
    }

    /**
     * This testcase exercises VirtualMachine.Dispose command when a thread, suspended by an
     * event, is still invoking a method.
     * <BR>At first the test starts DisposeDuringInvokeDebuggee debuggee.
     * <BR>Then the test sets a breakpoint so that the tested thread (DebuggeeThread) gets suspended
     * by an event. Once this thread is suspended, we send it an ObjectReference.InvokeMethod
     * command to initiate a method invocation executing in that thread. The method will synchronize
     * with the test, waiting for a signal to continue its execution.
     * <BR>While the tested thread waits for the signal, we send a VirtualMachine.Dispose command to
     * the debuggee and sends the expected signal so the tested thread completes the method
     * invocation.
     * <BR>Finally, we wait for the debuggee's main thread to signal us when the tested thread has
     * normally terminated after we dispose the JDWP connection.
     */
    public void testDisposeDuringInvoke() {
        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_READY);

        // Set breakpoint so the DebuggeeThread suspends itself only.
        long classID = getClassIDBySignature(getDebuggeeClassSignature());
        long invokedMethodId = getMethodID(classID,
                DisposeDuringInvokeDebuggee.INVOKED_METHOD_NAME);
        int breakpointID = debuggeeWrapper.vmMirror.setBreakpointAtMethodBegin(classID,
                DisposeDuringInvokeDebuggee.BREAKPOINT_METHOD_NAME,
                JDWPConstants.SuspendPolicy.EVENT_THREAD);
        long thisObjectId = getReceiverObjectId(classID);

        // Continue debuggee.
        synchronizer.sendMessage(JPDADebuggeeSynchronizer.SGNL_CONTINUE);

        // Wait for the DebuggeeThread to suspend on the breakpoint.
        long threadID = debuggeeWrapper.vmMirror.waitForBreakpoint(breakpointID);

        // Send ObjectReference.InvokeMethod command.
        CommandPacket command = new CommandPacket(
                JDWPCommands.ObjectReferenceCommandSet.CommandSetID,
                JDWPCommands.ObjectReferenceCommandSet.InvokeMethodCommand);
        command.setNextValueAsThreadID(thisObjectId);
        command.setNextValueAsThreadID(threadID);
        command.setNextValueAsClassID(classID);
        command.setNextValueAsMethodID(invokedMethodId);
        command.setNextValueAsInt(0);
        command.setNextValueAsInt(0);
        try {
            debuggeeWrapper.vmMirror.sendCommand(command);
        } catch (IOException e) {
            throw new TestErrorException("Failed to send ObjectReference.InvokeMethod command", e);
        }

        // Wait for the DebuggeeThread to start method invocation.
        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_READY);

        // Detach from debuggee with a VirtualMachine.Dispose command.
        debuggeeWrapper.vmMirror.dispose();

        // Signal DebuggeeThread to continue so it completes method invocation.
        synchronizer.sendMessage(JPDADebuggeeSynchronizer.SGNL_CONTINUE);

        // Wait for the DebuggeeThread to terminate. The debuggee's main thread waits for it
        // (using Thread.join) before signaling us.
        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_READY);

        // The test is a success: resume the debuggee to finish
        synchronizer.sendMessage(JPDADebuggeeSynchronizer.SGNL_CONTINUE);
    }

    /**
     * Returns the object ID of the debuggee object to issue the ObjectReference.InvokeMethod
     * command.
     *
     * @param classID
     *          the debuggee class ID.
     * @return the object ID of the debuggee
     */
    private long getReceiverObjectId(long classID) {
        long thisObjectFieldID = checkField(classID, DisposeDuringInvokeDebuggee.THIS_FIELD_NAME);
        Value thisObjectValue =
                debuggeeWrapper.vmMirror.getReferenceTypeValue(classID, thisObjectFieldID);
        assertEquals("Invalid value tag:", JDWPConstants.Tag.OBJECT_TAG, thisObjectValue.getTag());
        return thisObjectValue.getLongValue();
    }
}
