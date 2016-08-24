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

import org.apache.harmony.jpda.tests.framework.TestErrorException;
import org.apache.harmony.jpda.tests.framework.jdwp.CommandPacket;
import org.apache.harmony.jpda.tests.framework.jdwp.JDWPConstants;
import org.apache.harmony.jpda.tests.framework.jdwp.Location;
import org.apache.harmony.jpda.tests.framework.jdwp.ParsedEvent;
import org.apache.harmony.jpda.tests.framework.jdwp.ParsedEvent.EventThread;
import org.apache.harmony.jpda.tests.framework.jdwp.ParsedEvent.Event_EXCEPTION;
import org.apache.harmony.jpda.tests.framework.jdwp.ReplyPacket;
import org.apache.harmony.jpda.tests.share.JPDADebuggeeSynchronizer;

/**
 * JDWP Unit test for BREAKPOINT event on "catch (...)" line.
 */
public class BreakpointOnCatchTest extends JDWPEventTestCase {
    @Override
    protected String getDebuggeeClassName() {
        return BreakpointOnCatchDebuggee.class.getName();
    }

    /**
     * This testcase is for BREAKPOINT event.
     * <BR>It runs BreakpointOnCatchDebuggee and set breakpoint to its breakpointOnCatch
     * method, then verifies that the requested BREAKPOINT event occurs on a catch statement
     * (with a pending exception).
     */
    public void testBreakpointOnCatch() {
        logWriter.println("testBreakpointOnCatch started");

        // Wait for debuggee to start.
        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_READY);

        long classID = getClassIDBySignature(getDebuggeeClassSignature());
        long methodID = getMethodID(classID, BreakpointOnCatchDebuggee.BREAKPOINT_METHOD_NAME);

        // First, we set an EXCEPTION caught event to know the location of the catch.
        int exceptionRequestID = requestExceptionCaughtEvent();

        // Execute the EXCEPTION.
        synchronizer.sendMessage(JPDADebuggeeSynchronizer.SGNL_CONTINUE);

        // Wait for EXCEPTION event.
        CommandPacket event =
            debuggeeWrapper.vmMirror.receiveCertainEvent(JDWPConstants.EventKind.EXCEPTION);
        ParsedEvent[] parsedEvents = ParsedEvent.parseEventPacket(event);
        assertEquals("Invalid number of events:", 1, parsedEvents.length);
        assertEquals("Invalid request ID:", exceptionRequestID, parsedEvents[0].getRequestID());

        // Extract catch location and check it is in the tested method.
        Location catchLocation = ((Event_EXCEPTION) parsedEvents[0]).getCatchLocation();
        assertEquals("Invalid class ID:", classID, catchLocation.classID);
        assertEquals("Invalid method ID:", methodID, catchLocation.methodID);

        // Clear the EXCEPTION event.
        debuggeeWrapper.vmMirror.clearEvent(JDWPConstants.EventKind.EXCEPTION, exceptionRequestID);

        // Now we can set the BREAKPOINT on the catch location.
        int requestID = requestBreakpointEvent(catchLocation);

        // The debuggee is suspended on the EXCEPTION event: resume it to hit the BREAKPOINT event.
        resumeDebuggee();

        // Wait for BREAKPOINT event.
        event =
            debuggeeWrapper.vmMirror.receiveCertainEvent(JDWPConstants.EventKind.BREAKPOINT);
        parsedEvents = ParsedEvent.parseEventPacket(event);
        assertEquals("Invalid number of events:", 1, parsedEvents.length);
        assertEquals("Invalid request ID:", requestID, parsedEvents[0].getRequestID());

        // Check the thread is in an expected state.
        long eventThreadID = ((EventThread) parsedEvents[0]).getThreadID();
        checkThreadState(eventThreadID, JDWPConstants.ThreadStatus.RUNNING,
                JDWPConstants.SuspendStatus.SUSPEND_STATUS_SUSPENDED);

        logWriter.println("Successfully suspended on a catch statement");
        logWriter.println("testBreakpointOnCatch done");
    }

    private int requestExceptionCaughtEvent() {
      String exceptionSig =
          getClassSignature(BreakpointOnCatchDebuggee.BreakpointOnCatchDebuggeeException.class);
      ReplyPacket replyPacket = debuggeeWrapper.vmMirror.setException(exceptionSig, true, false);
      return replyPacket.getNextValueAsInt();

    }

    private int requestBreakpointEvent(Location location) {
      logWriter.println("Install breakpoint at " + location);
      ReplyPacket replyPacket = debuggeeWrapper.vmMirror.setBreakpoint(location);
      return replyPacket.getNextValueAsInt();

    }
}
