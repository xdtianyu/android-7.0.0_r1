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
import org.apache.harmony.jpda.tests.framework.jdwp.EventPacket;
import org.apache.harmony.jpda.tests.framework.jdwp.JDWPCommands;
import org.apache.harmony.jpda.tests.framework.jdwp.JDWPConstants;
import org.apache.harmony.jpda.tests.framework.jdwp.Location;
import org.apache.harmony.jpda.tests.framework.jdwp.ParsedEvent;
import org.apache.harmony.jpda.tests.framework.jdwp.ReplyPacket;

/**
 * Base class for EXCEPTION tests.
 */
public abstract class ExceptionBaseTest extends JDWPEventTestCase {

    /**
     * Waits for EXCEPTION event and checks it is the one we expect and the event
     * thread's state.
     */
    protected ParsedEvent.Event_EXCEPTION receiveAndCheckExceptionEvent(int requestID) {
        printTestLog("=> receiveEvent()...");
        EventPacket event = debuggeeWrapper.vmMirror.receiveEvent();
        printTestLog("Event is received! Check it ...");
        ParsedEvent[] parsedEvents = ParsedEvent.parseEventPacket(event);

        // assert that event is the expected one
        printTestLog("parsedEvents.length = " + parsedEvents.length);
        printTestLog("parsedEvents[0].getEventKind() = " + parsedEvents[0].getEventKind());
        assertEquals("Invalid number of events,", 1, parsedEvents.length);
        assertEquals("Invalid event kind,",
                JDWPConstants.EventKind.EXCEPTION,
                parsedEvents[0].getEventKind(),
                JDWPConstants.EventKind.getName(JDWPConstants.EventKind.EXCEPTION),
                JDWPConstants.EventKind.getName(parsedEvents[0].getEventKind()));
        assertEquals("Invalid event request ID", requestID, parsedEvents[0].getRequestID());

        ParsedEvent.Event_EXCEPTION exceptionEvent = (ParsedEvent.Event_EXCEPTION) parsedEvents[0];

        long eventThreadID = exceptionEvent.getThreadID();
        checkThreadState(eventThreadID, JDWPConstants.ThreadStatus.RUNNING,
                JDWPConstants.SuspendStatus.SUSPEND_STATUS_SUSPENDED);

        // Remove event request.
        debuggeeWrapper.vmMirror.clearEvent(JDWPConstants.EventKind.EXCEPTION, requestID);

        return exceptionEvent;
    }

    /**
     * Returns the location of the top stack frame of a thread by sending a
     * ThreadReference.Frames command for only one frame.
     * @param threadID the thread ID
     * @return the location of the top stack frame of a thread
     */
    protected Location getTopFrameLocation(long threadID) {
        // getting frames of the thread
        CommandPacket packet = new CommandPacket(
                JDWPCommands.ThreadReferenceCommandSet.CommandSetID,
                JDWPCommands.ThreadReferenceCommandSet.FramesCommand);
        packet.setNextValueAsThreadID(threadID);
        packet.setNextValueAsInt(0);
        packet.setNextValueAsInt(1);
        ReplyPacket reply = debuggeeWrapper.vmMirror.performCommand(packet);
        debuggeeWrapper.vmMirror.checkReply(reply);

        // assert that only one top frame is returned
        int framesCount = reply.getNextValueAsInt();
        assertEquals("Invalid number of top stack frames,", 1, framesCount);

        reply.getNextValueAsFrameID(); // frameID
        return reply.getNextValueAsLocation();
    }

    public String dumpLocation(Location location) {
        StringBuilder builder = new StringBuilder("{");
        String classSig = "<null>";
        String methodName = "<null>";
        if (location.classID != 0 && location.methodID != 0) {
            classSig = getClassSignature(location.classID);
            methodName = getMethodName(location.classID, location.methodID);
        }
        builder.append(JDWPConstants.TypeTag.getName(location.tag));
        builder.append(',');
        builder.append("0x" + Long.toHexString(location.classID));
        builder.append(" (" + classSig + "),");
        builder.append("0x" + Long.toHexString(location.methodID));
        builder.append(" (" + methodName + "),");
        builder.append("0x" + Long.toHexString(location.index));
        builder.append('}');
        return builder.toString();
    }
}
