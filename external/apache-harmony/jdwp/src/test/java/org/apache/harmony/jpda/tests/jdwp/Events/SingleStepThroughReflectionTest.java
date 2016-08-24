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

import org.apache.harmony.jpda.tests.framework.jdwp.CommandPacket;
import org.apache.harmony.jpda.tests.framework.jdwp.EventMod;
import org.apache.harmony.jpda.tests.framework.jdwp.JDWPCommands;
import org.apache.harmony.jpda.tests.framework.jdwp.JDWPConstants;
import org.apache.harmony.jpda.tests.framework.jdwp.JDWPConstants.StepDepth;
import org.apache.harmony.jpda.tests.framework.jdwp.JDWPConstants.StepSize;
import org.apache.harmony.jpda.tests.framework.jdwp.Location;
import org.apache.harmony.jpda.tests.framework.jdwp.ParsedEvent;
import org.apache.harmony.jpda.tests.framework.jdwp.ParsedEvent.EventThreadLocation;
import org.apache.harmony.jpda.tests.framework.jdwp.ReplyPacket;
import org.apache.harmony.jpda.tests.share.JPDADebuggeeSynchronizer;

/**
 * JDWP Unit test for SINGLE_STEP event.
 */
public class SingleStepThroughReflectionTest extends JDWPEventTestCase {

    private static final String DEBUGGEE_SIGNATURE = "Lorg/apache/harmony/jpda/tests/jdwp/Events/SingleStepThroughReflectionDebuggee;";

    private static final String DEBUGGEE_CLASS_NAME = "org.apache.harmony.jpda.tests.jdwp.Events.SingleStepThroughReflectionDebuggee";

    // The method where we set a breakpoint to suspend execution.
    private static final String BREAKPOINT_METHOD = "breakpointTest";

    // The method where we expect to suspend with the single-step.
    private static final String EVENT_METHOD = "methodCalledThroughReflection";

    protected String getDebuggeeClassName() {
        return DEBUGGEE_CLASS_NAME;
    }

    /**
     * This test case exercises SINGLE_STEP INTO event through reflection.<BR>
     *
     * Runs SingleStepNativeDebuggee and sets breakpoint to its
     * breakpointTest method, sends a request for single step event, then
     * verifies that requested SINGLE_STEP event occurred in the
     * methodCalledThroughReflection method.
     *
     */
    public void testSingleStepIntoThroughReflection() {
        logWriter.println("=> testSingleStepIntoThroughReflection started");

        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_READY);

        // Find debuggee class id.
        long refTypeID = getClassIDBySignature(DEBUGGEE_SIGNATURE);
        logWriter.println("=> Debuggee class = " + getDebuggeeClassName());
        logWriter.println("=> referenceTypeID for Debuggee class = " + refTypeID);
        logWriter.println("=> Send ReferenceType::Methods command and get methodIDs ");

        // Set breakpoint to suspend execution.
        int breakpointRequestID = debuggeeWrapper.vmMirror.setBreakpointAtMethodBegin(
                refTypeID, BREAKPOINT_METHOD);
        logWriter.println("=> breakpointID = " + breakpointRequestID);
        logWriter.println("=> starting thread");

        // Resume debuggee so we hit the breakpoint.
        synchronizer.sendMessage(JPDADebuggeeSynchronizer.SGNL_CONTINUE);

        // Wait for the breakpoint event.
        long breakpointThreadID = debuggeeWrapper.vmMirror
                .waitForBreakpoint(breakpointRequestID);
        logWriter.println("=> breakpointThreadID = " + breakpointThreadID);

        // Sending a SINGLE_STEP request with a ClassOnly modifier so we only
        // suspend in the debuggee class.
        CommandPacket setRequestCommand = new CommandPacket(
                JDWPCommands.EventRequestCommandSet.CommandSetID,
                JDWPCommands.EventRequestCommandSet.SetCommand);
        setRequestCommand
                .setNextValueAsByte(JDWPConstants.EventKind.SINGLE_STEP);
        setRequestCommand.setNextValueAsByte(JDWPConstants.SuspendPolicy.ALL);
        setRequestCommand.setNextValueAsInt(2);
        setRequestCommand.setNextValueAsByte(EventMod.ModKind.Step);
        setRequestCommand.setNextValueAsThreadID(breakpointThreadID);
        setRequestCommand.setNextValueAsInt(StepSize.LINE);
        setRequestCommand.setNextValueAsInt(StepDepth.INTO);
        setRequestCommand.setNextValueAsByte(EventMod.ModKind.ClassOnly);
        setRequestCommand.setNextValueAsReferenceTypeID(refTypeID);

        ReplyPacket setRequestReply = debuggeeWrapper.vmMirror
                .performCommand(setRequestCommand);

        checkReplyPacket(setRequestReply, "Set SINGLE_STEP event");
        int stepRequestID = setRequestReply.getNextValueAsInt();

        logWriter.println("=> RequestID = " + stepRequestID);
        assertAllDataRead(setRequestReply);

        // Resume debuggee.
        resumeDebuggee();

        ParsedEvent parsedEvent = waitForSingleStepEvent(stepRequestID);

        // Clear SINGLE_STEP event.
        clearSingleStep(stepRequestID);

        // Check we stopped in the expected method.
        checkSingleStepEvent((EventThreadLocation) parsedEvent, refTypeID);

        logWriter.println("==> Resuming debuggee");
        resumeDebuggee();
        logWriter.println("==> Test PASSED!");
    }

    private void checkSingleStepEvent(EventThreadLocation eventThreadLocation,
            long refTypeID) {
        // Find expected method id.
        long expectedMethodID = getMethodID(refTypeID, EVENT_METHOD);

        // Dump where we stopped.
        Location location = eventThreadLocation.getLocation();
        String className = getClassSignature(location.classID);
        String methodName = getMethodName(location.classID,
                location.methodID);
        logWriter.println("Stopped in " + className + "." + methodName
                + " at code index " + location.index);

        // Check we stopped in the right method.
        assertEquals("Stopped in wrong class", refTypeID, location.classID);
        assertEquals("Stopped in wrong method", expectedMethodID, location.methodID);
    }

    private ParsedEvent waitForSingleStepEvent(int requestID) {
        logWriter.println("==> Wait for SINGLE_STEP event");
        CommandPacket event = debuggeeWrapper.vmMirror.receiveEvent();
        ParsedEvent[] parsedEvents = ParsedEvent.parseEventPacket(event);

        logWriter.println("==> Received " + parsedEvents.length + " events");

        // Dump events.
        for (int i = 0; i < parsedEvents.length; i++) {
            logWriter.println("");
            logWriter.println("==> Event #" + i + ";");
            logWriter.println("==> EventKind: " + parsedEvents[i].getEventKind() + "("
                    + JDWPConstants.EventKind.getName(parsedEvents[i].getEventKind()) + ")");
            logWriter.println("==> RequestID: " + parsedEvents[i].getRequestID());
        }

        // We expect only one event.
        assertEquals("Received wrong number of events,", 1, parsedEvents.length);

        // Check this is the event we were waiting for.
        ParsedEvent parsedEvent = parsedEvents[0];
        assertEquals("Received wrong event request ID,", requestID, parsedEvent.getRequestID());
        assertEquals("Invalid event kind,", JDWPConstants.EventKind.SINGLE_STEP,
                parsedEvent.getEventKind(),
                JDWPConstants.EventKind.getName(JDWPConstants.EventKind.SINGLE_STEP),
                JDWPConstants.EventKind.getName(parsedEvent.getEventKind()));

        return parsedEvent;
    }

    private void clearSingleStep(int stepRequestID) {
        logWriter.println("==> Clearing SINGLE_STEP event..");
        ReplyPacket clearRequestReply =
            debuggeeWrapper.vmMirror.clearEvent(JDWPConstants.EventKind.SINGLE_STEP,
                                                stepRequestID);
        checkReplyPacket(clearRequestReply, "Clear SINGLE_STEP event");
        logWriter.println("==> SINGLE_STEP event has been cleared");
    }
}
