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
import org.apache.harmony.jpda.tests.framework.jdwp.Location;
import org.apache.harmony.jpda.tests.framework.jdwp.ParsedEvent;
import org.apache.harmony.jpda.tests.framework.jdwp.ReplyPacket;
import org.apache.harmony.jpda.tests.share.JPDADebuggeeSynchronizer;

/**
 * JDWP Unit test for SINGLE_STEP event with LocationOnly modifier.
 */
public class SingleStepWithLocationTest extends JDWPEventTestCase {

    private String debuggeeSignature = "Lorg/apache/harmony/jpda/tests/jdwp/Events/SingleStepDebuggee;";

    private static final String BREAKPOINT_METHOD_NAME = "breakpointTest";

    protected String getDebuggeeClassName() {
        return SingleStepDebuggee.class.getName();
    }

    /**
     * This test case exercises SINGLE_STEP event.<BR>
     *
     * Runs SingleStepDebuggee and sets breakpoint to its
     * breakpointTest method, sends a request for single step event with the
     * location of the last line so we single-step until there. Then verifies
     * that requested SINGLE_STEP event occurs.
     */
    public void testSingleStepToLocation() {
        logWriter.println("=> testSingleStepToLocation started");

        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_READY);

        // Set breakpoint.
        long refTypeID = getClassIDBySignature(debuggeeSignature);

        logWriter.println("=> Debuggee class = " + getDebuggeeClassName());
        logWriter.println("=> referenceTypeID for Debuggee class = " + refTypeID);
        logWriter.println("=> Send ReferenceType::Methods command and get methodIDs ");

        int requestID = debuggeeWrapper.vmMirror.setBreakpointAtMethodBegin(
                refTypeID, BREAKPOINT_METHOD_NAME);
        logWriter.println("=> breakpointID = " + requestID);
        logWriter.println("=> starting thread");

        // Execute the breakpoint
        synchronizer.sendMessage(JPDADebuggeeSynchronizer.SGNL_CONTINUE);

        // Wait for breakpoint event so the program is suspended.
        long breakpointThreadID = debuggeeWrapper.vmMirror
                .waitForBreakpoint(requestID);

        logWriter.println("=> breakpointThreadID = " + breakpointThreadID);

        // Remove breakpoint.
        debuggeeWrapper.vmMirror.clearBreakpoint((int)requestID);

        // Get line table and get code index of the last line.
        long methodID = getMethodID(refTypeID, BREAKPOINT_METHOD_NAME);
        ReplyPacket lineTableReply = getLineTable(refTypeID, methodID);
        checkReplyPacket(lineTableReply, "Method.LineTable");
        lineTableReply.getNextValueAsLong();  // startIndex
        lineTableReply.getNextValueAsLong();  // endIndex
        int linesCount = lineTableReply.getNextValueAsInt();
        long lastLineCodeIndex = -1;
        int lastLineNumber = -1;
        for (int i = 0; i < linesCount; ++i) {
            lastLineCodeIndex = lineTableReply.getNextValueAsLong();
            lastLineNumber = lineTableReply.getNextValueAsInt();
        }

        logWriter.println("Single-step until line " + lastLineNumber);

        // Location of last line.
        Location location = new Location(JDWPConstants.TypeTag.CLASS,
                refTypeID, methodID, lastLineCodeIndex);

        // Sending a SINGLE_STEP request
        CommandPacket setRequestCommand = new CommandPacket(
                JDWPCommands.EventRequestCommandSet.CommandSetID,
                JDWPCommands.EventRequestCommandSet.SetCommand);
        setRequestCommand.setNextValueAsByte(
                JDWPConstants.EventKind.SINGLE_STEP);
        setRequestCommand.setNextValueAsByte(JDWPConstants.SuspendPolicy.ALL);
        setRequestCommand.setNextValueAsInt(2);
        setRequestCommand.setNextValueAsByte(EventMod.ModKind.Step);
        setRequestCommand.setNextValueAsThreadID(breakpointThreadID);
        setRequestCommand.setNextValueAsInt(JDWPConstants.StepSize.LINE);
        setRequestCommand.setNextValueAsInt(JDWPConstants.StepDepth.OVER);
        setRequestCommand.setNextValueAsByte(EventMod.ModKind.LocationOnly);
        setRequestCommand.setNextValueAsLocation(location);

        ReplyPacket setRequestReply = debuggeeWrapper.vmMirror
                .performCommand(setRequestCommand);
        checkReplyPacket(setRequestReply, "Set SINGLE_STEP event");
        requestID = setRequestReply.getNextValueAsInt();

        logWriter.println("=> RequestID = " + requestID);
        assertAllDataRead(setRequestReply);

        // Resume debuggee so we can suspend on single-step.
        resumeDebuggee();

        // Wait for event.
        logWriter.println("==> Wait for SINGLE_STEP event");
        CommandPacket event = debuggeeWrapper.vmMirror.receiveEvent();
        ParsedEvent[] parsedEvents = ParsedEvent.parseEventPacket(event);

        // Check if received event is expected.
        logWriter.println("==> Received " + parsedEvents.length + " events");

        // Trace events
        for (int i = 0; i < parsedEvents.length; i++) {
            logWriter.println("");
            logWriter.println("==> Event #" + i + ";");
            logWriter.println("==> EventKind: " + parsedEvents[i].getEventKind() + "("
                    + JDWPConstants.EventKind.getName(parsedEvents[i].getEventKind()) + ")");
            logWriter.println("==> RequestID: " + parsedEvents[i].getRequestID());
        }

        // Check all
        assertEquals("Received wrong number of events,", 1, parsedEvents.length);
        assertEquals("Received wrong event request ID,", requestID, parsedEvents[0].getRequestID());
        assertEquals("Invalid event kind,", JDWPConstants.EventKind.SINGLE_STEP,
                parsedEvents[0].getEventKind(),
                JDWPConstants.EventKind.getName(JDWPConstants.EventKind.SINGLE_STEP),
                JDWPConstants.EventKind.getName(parsedEvents[0].getEventKind()));

        // Clear SINGLE_STEP event
        logWriter.println("==> Clearing SINGLE_STEP event..");
        ReplyPacket clearRequestReply =
            debuggeeWrapper.vmMirror.clearEvent(JDWPConstants.EventKind.SINGLE_STEP, (int) requestID);
        checkReplyPacket(clearRequestReply, "Clear SINGLE_STEP event");
        logWriter.println("==> SINGLE_STEP event has been cleared");

        // Resuming debuggee before leaving.
        resumeDebuggee();
        logWriter.println("==> Test PASSED!");
    }
}
