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

import org.apache.harmony.jpda.tests.framework.jdwp.EventPacket;
import org.apache.harmony.jpda.tests.framework.jdwp.JDWPConstants;
import org.apache.harmony.jpda.tests.framework.jdwp.Location;
import org.apache.harmony.jpda.tests.framework.jdwp.ParsedEvent;
import org.apache.harmony.jpda.tests.framework.jdwp.ReplyPacket;
import org.apache.harmony.jpda.tests.share.JPDADebuggeeSynchronizer;

/**
 * JDWP Unit test for SINGLE_STEP event with pending exception.
 */
public class SingleStepWithPendingExceptionTest extends JDWPEventTestCase {

    @Override
    protected String getDebuggeeClassName() {
        return SingleStepWithPendingExceptionDebuggee.class.getName();
    }

    /**
     * Tests that we properly single-step OUT of a method throwing an exception
     * to a catch handler.
     *
     * We execute the test method once with an EXCEPTION event request to capture the
     * location of the catch handler.
     * Then we set a BREAKPOINT event in the method throwing the exception and execute
     * the test method another time. We wait for hitting the breakpoint then we issue
     * a SINGLE_STEP OUT request and check that we stop in the catch handler.
     */
    public void testSingleStepWithPendingException() {
        logWriter.println("=> testSingleStepWithPendingException started");

        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_READY);

        // Find debuggee class id.
        long refTypeID = getClassIDBySignature(getDebuggeeClassSignature());
        logWriter.println("=> Debuggee class = " + getDebuggeeClassName());
        logWriter.println("=> referenceTypeID for Debuggee class = " + refTypeID);

        long catchMethodId = getMethodID(refTypeID, "catchMethod");

        // Request exception event.
        int exceptionRequestId = setCatchException();

        // Resume debuggee so we hit the breakpoint.
        synchronizer.sendMessage(JPDADebuggeeSynchronizer.SGNL_CONTINUE);

        // Wait for exception and remember catch location.
        EventPacket eventPacket =
                debuggeeWrapper.vmMirror.receiveCertainEvent(JDWPConstants.EventKind.EXCEPTION);
        ParsedEvent[] parsedEvents = ParsedEvent.parseEventPacket(eventPacket);
        assertEquals("Expected only one event", 1, parsedEvents.length);
        assertEventKindEquals("Expected EXCEPTION event", parsedEvents[0].getEventKind(),
                JDWPConstants.EventKind.EXCEPTION);
        assertEquals("Unexpected event", exceptionRequestId, parsedEvents[0].getRequestID());
        Location catchHandlerLocation =
                ((ParsedEvent.Event_EXCEPTION)parsedEvents[0]).getCatchLocation();
        assertEquals("Invalid location's class", refTypeID, catchHandlerLocation.classID);
        assertEquals("Invalid location's method", catchMethodId, catchHandlerLocation.methodID);

        // Remove exception request.
        clearEvent(JDWPConstants.EventKind.EXCEPTION, exceptionRequestId, true);

        // Install breakpoint at begin of throwMethod.
        int breakpointRequestId = debuggeeWrapper.vmMirror.setBreakpointAtMethodBegin(refTypeID,
                "throwMethod");

        // Exception event suspended all threads: resume them now.
        debuggeeWrapper.resume();

        // Wait for breakpoint.
        long threadId = debuggeeWrapper.vmMirror.waitForBreakpoint(breakpointRequestId);

        // Single-step OUT of the throwMethod.
        int singleStepRequestId = setSingleStepOut(threadId);

        // Breakpoint event suspended all threads: resume them now.
        debuggeeWrapper.resume();

        // Wait for single-step.
        eventPacket =
                debuggeeWrapper.vmMirror.receiveCertainEvent(JDWPConstants.EventKind.SINGLE_STEP);
        parsedEvents = ParsedEvent.parseEventPacket(eventPacket);
        assertEquals("Expected only one event", 1, parsedEvents.length);
        assertEventKindEquals("Expected SINGLE_STEP event", parsedEvents[0].getEventKind(),
                JDWPConstants.EventKind.SINGLE_STEP);

        // Check that we reached the catch handler location.
        Location singleStepLocation =
                ((ParsedEvent.Event_SINGLE_STEP)parsedEvents[0]).getLocation();
        if (!catchHandlerLocation.equals(singleStepLocation)) {
            fail("Invalid single-step location: expected " + catchHandlerLocation +
                    " but was " + singleStepLocation);
        }

        // Remove single-step request.
        clearEvent(JDWPConstants.EventKind.SINGLE_STEP, singleStepRequestId, true);

        logWriter.println("==> Resuming debuggee");
        resumeDebuggee();
        logWriter.println("==> testSingleStepWithPendingException PASSED!");
    }

    private int setCatchException() {
        ReplyPacket replyPacket = debuggeeWrapper.vmMirror.setException(
                getClassSignature(SingleStepWithPendingExceptionDebuggee.DebuggeeException.class),
                true, true);
        checkReplyPacket(replyPacket, "Failed to set EXCEPTION event request");
        return replyPacket.getNextValueAsInt();
    }

    private int setSingleStepOut(long threadId) {
        ReplyPacket replyPacket = debuggeeWrapper.vmMirror.setStep(new String[0], threadId,
                JDWPConstants.StepSize.LINE, JDWPConstants.StepDepth.OUT);
        checkReplyPacket(replyPacket, "Failed to set SINGLE_STEP OUT event request");
        return replyPacket.getNextValueAsInt();
    }
}
