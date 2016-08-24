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

import org.apache.harmony.jpda.tests.framework.jdwp.EventPacket;
import org.apache.harmony.jpda.tests.framework.jdwp.JDWPConstants;
import org.apache.harmony.jpda.tests.framework.jdwp.ParsedEvent;
import org.apache.harmony.jpda.tests.framework.jdwp.ReplyPacket;
import org.apache.harmony.jpda.tests.jdwp.share.JDWPSyncTestCase;
import org.apache.harmony.jpda.tests.share.JPDADebuggeeSynchronizer;

public class DeoptimizationWithExceptionHandlingTest extends JDWPSyncTestCase {

    @Override
    protected String getDebuggeeClassName() {
        return DeoptimizationWithExceptionHandlingDebuggee.class.getName();
    }

    /**
     * This tests checks we properly handle exception event when we fully
     * deoptimize the stack.
     * We first set a BREAKPOINT event to suspend the debuggee. Once we
     * hit the breakpoint, we set a METHOD_ENTRY event on the debuggee class
     * to cause a full deoptimization of the stack and resume it.
     * Finally, we wait for the debuggee to send us the result of the test
     * (an integer as a string) and check this is the expected result.
     */
    public void testDeoptimizationWithExceptionHandling_001() {
        logWriter.println("testDeoptimizationWithExceptionHandling_001 starts");
        runTestDeoptimizationWithExceptionHandling(false);
        logWriter.println("testDeoptimizationWithExceptionHandling_001 ends");
    }

    /**
     * This tests checks we properly handle exception event when we fully
     * deoptimize the stack and are able to receive EXCEPTION event for the
     * thrown exception.
     * We first set a BREAKPOINT event to suspend the debuggee. Once we
     * hit the breakpoint, we set a METHOD_ENTRY event on the debuggee class
     * to cause a full deoptimization of the stack and an EXCEPTION event
     * to check we do suspend the debuggee for the thrown exception, and
     * we resume the debuggee.
     * Then we wait for the EXCEPTION event to be posted and resume the
     * debuggee again.
     * Finally, we wait for the debuggee to send us the result of the test
     * (an integer as a string) and check this is the expected result.
     */
    public void testDeoptimizationWithExceptionHandling_002() {
        logWriter.println("testDeoptimizationWithExceptionHandling_002 starts");
        runTestDeoptimizationWithExceptionHandling(true);
        logWriter.println("testDeoptimizationWithExceptionHandling_002 ends");
    }

    private void runTestDeoptimizationWithExceptionHandling(boolean withExceptionEvent) {
        // Suspend debuggee on a breakpoint.
        stopOnBreakpoint();

        // Request MethodEntry event to cause full deoptimization of the debuggee.
        installMethodEntry();

        int exceptionRequestID = -1;
        if (withExceptionEvent) {
            // Request Exception event to test we suspend for this event during deoptimization.
            exceptionRequestID = requestExceptionEvent();
        }

        // Resume the debuggee from the breakpoint.
        debuggeeWrapper.vmMirror.resume();

        if (exceptionRequestID != -1) {
            // Wait for the Exception event.
            waitForExceptionEvent(exceptionRequestID);

            // Resume the debuggee from the exception.
            debuggeeWrapper.vmMirror.resume();
        }

        // Wait for result from debuggee
        String resultAsString = synchronizer.receiveMessage();
        int result = Integer.parseInt(resultAsString);

        assertEquals("Incorrect result",
                     DeoptimizationWithExceptionHandlingDebuggee.SUCCESS_RESULT, result);
    }

    private void stopOnBreakpoint() {
        // Wait for debuggee to start.
        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_READY);

        long debuggeeClassID = getClassIDBySignature(getDebuggeeClassSignature());
        int requestID = debuggeeWrapper.vmMirror.setBreakpointAtMethodBegin(debuggeeClassID,
                                                                            "breakpointMethod");

        // Continue debuggee.
        synchronizer.sendMessage(JPDADebuggeeSynchronizer.SGNL_CONTINUE);

        // Wait for breakpoint.
        debuggeeWrapper.vmMirror.waitForBreakpoint(requestID);

        // Remove breakpoint.
        debuggeeWrapper.vmMirror.clearBreakpoint(requestID);
    }

    private void installMethodEntry() {
        ReplyPacket replyPacket = debuggeeWrapper.vmMirror.setMethodEntry(getDebuggeeClassName());
        replyPacket.getNextValueAsInt();  // unused 'requestID'
        assertAllDataRead(replyPacket);
    }

    private int requestExceptionEvent() {
        final String exceptionClassSignature = "Ljava/lang/NullPointerException;";
        ReplyPacket replyPacket =
                debuggeeWrapper.vmMirror.setException(exceptionClassSignature, true, false);
        int requestID = replyPacket.getNextValueAsInt();
        assertAllDataRead(replyPacket);
        return requestID;
    }

    private void waitForExceptionEvent(int requestID) {
        final byte eventKind = JDWPConstants.EventKind.EXCEPTION;
        EventPacket event = debuggeeWrapper.vmMirror.receiveCertainEvent(eventKind);

        ParsedEvent[] parsedEvents = ParsedEvent.parseEventPacket(event);
        assertNotNull("Expected an exception event", parsedEvents);
        assertEquals("Expected only one event", 1, parsedEvents.length);
        assertEquals("Not the excepted event", requestID, parsedEvents[0].getRequestID());

        // Clear the event
        debuggeeWrapper.vmMirror.clearEvent(eventKind, requestID);
    }
}
