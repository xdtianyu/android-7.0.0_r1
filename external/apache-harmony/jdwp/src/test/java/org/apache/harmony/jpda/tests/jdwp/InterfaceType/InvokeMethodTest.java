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

package org.apache.harmony.jpda.tests.jdwp.InterfaceType;

import org.apache.harmony.jpda.tests.framework.LogWriter;
import org.apache.harmony.jpda.tests.framework.jdwp.*;
import org.apache.harmony.jpda.tests.jdwp.share.JDWPSyncTestCase;
import org.apache.harmony.jpda.tests.jdwp.share.debuggee.*;
import org.apache.harmony.jpda.tests.share.JPDADebuggeeSynchronizer;
import org.apache.harmony.jpda.tests.share.JPDATestOptions;

/**
 * JDWP unit test to exercise InterfaceType.InvokeMethod command.
 */
public class InvokeMethodTest extends JDWPSyncTestCase {

    @Override
    protected String getDebuggeeClassName() {
        return InvokeMethodDebuggee.class.getName();
    }

    /**
     * This testcase exercises InterfaceType.InvokeMethod command.
     * <BR>The test first starts the debuggee, request METHOD_ENTRY event so the
     * application suspends on first invoke.
     * <BR>Then sends InterfaceType.InvokeMethod command for method with null
     * argument. Checks that returned value is expected int value and returned
     * exception object is as expected.
     * <BR>Finally resume the application.
     */
    private void testInvokeMethodStatic(boolean shouldThrow) {
        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_READY);

        // Get debuggee class ID.
        String debuggeeClassSig = getDebuggeeClassSignature();
        long debuggeeTypeID = debuggeeWrapper.vmMirror.getClassID(debuggeeClassSig);
        assertTrue("Failed to find debuggee class", debuggeeTypeID != 0);

        // Set METHOD_ENTRY event request so application is suspended.
        CommandPacket packet = new CommandPacket(
                JDWPCommands.EventRequestCommandSet.CommandSetID,
                JDWPCommands.EventRequestCommandSet.SetCommand);
        packet.setNextValueAsByte(JDWPConstants.EventKind.METHOD_ENTRY);
        packet.setNextValueAsByte(JDWPConstants.SuspendPolicy.ALL);
        packet.setNextValueAsInt(1);  // number of modifiers.
        packet.setNextValueAsByte(EventMod.ModKind.ClassOnly);  // class-only modifier.
        packet.setNextValueAsReferenceTypeID(debuggeeTypeID);
        ReplyPacket reply = debuggeeWrapper.vmMirror.performCommand(packet);
        checkReplyPacket(reply, "EventRequest::Set command");

        int requestID = reply.getNextValueAsInt();
        logWriter.println(" EventRequest.Set: requestID=" + requestID);
        assertAllDataRead(reply);
        synchronizer.sendMessage(JPDADebuggeeSynchronizer.SGNL_CONTINUE);

        long targetThreadID = 0;
        // Wait for METHOD_ENTRY event and collect event thread.
        CommandPacket event = debuggeeWrapper.vmMirror.receiveEvent();
        byte suspendPolicy = event.getNextValueAsByte();
        int events = event.getNextValueAsInt();
        logWriter.println(" EVENT_THREAD event: suspendPolicy=" + suspendPolicy + " events=" + events);
        for (int i = 0; i < events; i++) {
            byte eventKind = event.getNextValueAsByte();
            int newRequestID = event.getNextValueAsInt();
            long threadID = event.getNextValueAsThreadID();
            //Location location =
                event.getNextValueAsLocation();
            logWriter.println("  EVENT_THREAD event " + i + ": eventKind="
                    + eventKind + " requestID=" + newRequestID + " threadID="
                    + threadID);
            if (newRequestID == requestID) {
                targetThreadID = threadID;
            }
        }
        assertAllDataRead(event);
        assertTrue("Invalid targetThreadID, must be != 0", targetThreadID != 0);

        //  Now we're suspended, clear event request.
        debuggeeWrapper.vmMirror.clearEvent(JDWPConstants.EventKind.METHOD_ENTRY, requestID);

        // Get test method ID.
        String debuggeeInterfaceSig = getClassSignature(InvokeMethodTestInterface.class);
        long debuggeeInterfaceTypeID =
                debuggeeWrapper.vmMirror.getInterfaceID(debuggeeInterfaceSig);
        assertTrue("Failed to find debuggee interface", debuggeeInterfaceTypeID != 0);
        long targetMethodID = debuggeeWrapper.vmMirror.getMethodID(debuggeeInterfaceTypeID,
                "testInvokeMethodStatic1");
        assertTrue("Failed to find method", targetMethodID != 0);

        Value throwValue = new Value(shouldThrow);
        // Invoke test method with null argument.
        packet = new CommandPacket(
                JDWPCommands.InterfaceTypeCommandSet.CommandSetID,
                JDWPCommands.InterfaceTypeCommandSet.InvokeMethodCommand);
        packet.setNextValueAsInterfaceID(debuggeeInterfaceTypeID);
        packet.setNextValueAsThreadID(targetThreadID);
        packet.setNextValueAsMethodID(targetMethodID);
        packet.setNextValueAsInt(1);  // number of arguments
        packet.setNextValueAsValue(throwValue);
        packet.setNextValueAsInt(0);  // invoke options
        logWriter.println(" Send InterfaceType.InvokeMethod " +
                ((shouldThrow) ? "with" : "without") + " exception.");
        reply = debuggeeWrapper.vmMirror.performCommand(packet);
        checkReplyPacket(reply, "InterfaceType::InvokeMethod command");

        Value returnValue = reply.getNextValueAsValue();
        assertNotNull("Returned value is null", returnValue);
        if (shouldThrow) {
            assertEquals("Invalid returned value,", 0, returnValue.getIntValue());
            logWriter.println(" InterfaceType.InvokeMethod: returnValue.getIntValue()="
                    + returnValue.getIntValue());

            // Check that some exception was thrown.
            TaggedObject exception = reply.getNextValueAsTaggedObject();
            assertNotNull("Returned exception is null", exception);
            assertTrue("Invalid exception object ID:<" + exception.objectID + ">",
                    exception.objectID != 0);
            assertEquals("Invalid exception tag,", JDWPConstants.Tag.OBJECT_TAG, exception.tag
                    , JDWPConstants.Tag.getName(JDWPConstants.Tag.OBJECT_TAG)
                    , JDWPConstants.Tag.getName(exception.tag));
            logWriter.println(" InterfaceType.InvokeMethod: exception.tag="
                    + exception.tag + " exception.objectID=" + exception.objectID);
        } else {
            assertEquals("Invalid returned value,",
                InvokeMethodTestInterface.RETURN_VALUE, returnValue.getIntValue());
            logWriter.println(" InterfaceType.InvokeMethod: returnValue.getIntValue()="
                    + returnValue.getIntValue());

            TaggedObject exception = reply.getNextValueAsTaggedObject();
            assertNotNull("Returned exception is null", exception);
            assertEquals("Invalid exception object ID:<" + exception.objectID + ">",
                    exception.objectID, 0);
            assertEquals("Invalid exception tag,", JDWPConstants.Tag.OBJECT_TAG, exception.tag
                    , JDWPConstants.Tag.getName(JDWPConstants.Tag.OBJECT_TAG)
                    , JDWPConstants.Tag.getName(exception.tag));
            logWriter.println(" InterfaceType.InvokeMethod: exception.tag="
                    + exception.tag + " exception.objectID=" + exception.objectID);
        }

        assertAllDataRead(reply);

        //  Let's resume application suspended on the METHOD_ENTRY event.
        debuggeeWrapper.vmMirror.resume();

        synchronizer.sendMessage(JPDADebuggeeSynchronizer.SGNL_CONTINUE);
    }

    public void testInvokeMethodStaticWithoutThrowing() {
        testInvokeMethodStatic(false);
    }
    public void testInvokeMethodStaticThrowing() {
        testInvokeMethodStatic(true);
    }
}
