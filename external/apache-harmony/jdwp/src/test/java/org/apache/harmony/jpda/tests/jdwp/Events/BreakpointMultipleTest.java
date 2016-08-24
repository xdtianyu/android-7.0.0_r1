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
import org.apache.harmony.jpda.tests.framework.jdwp.JDWPConstants;
import org.apache.harmony.jpda.tests.framework.jdwp.ParsedEvent;
import org.apache.harmony.jpda.tests.framework.jdwp.VmMirror;
import org.apache.harmony.jpda.tests.share.JPDADebuggeeSynchronizer;

import java.util.HashSet;
import java.util.Set;

/**
 * JDWP Unit test for multiple BREAKPOINT events.
 */
public class BreakpointMultipleTest extends JDWPEventTestCase {
    protected String getDebuggeeClassName() {
        return BreakpointDebuggee.class.getName();
    }

    /**
     * This testcase is for BREAKPOINT event. It checks we can set
     * multiple breakpoints at the same location.
     * <BR>It runs BreakpointDebuggee and sets two breakpoints in the
     * breakpointTest method, then clears both breakpoints.
     */
    public void testSetAndClearMultipleBreakpoint() {
        logWriter.println("testSetAndClearMultipleBreakpoint started");

        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_READY);

        // Set breakpoints.
        long classID = getClassIDBySignature(getDebuggeeClassSignature());
        int breakpoint1 = setBreakpoint(classID);
        int breakpoint2 = setBreakpoint(classID);

        // Clear breakpoints
        debuggeeWrapper.vmMirror.clearBreakpoint(breakpoint1);
        debuggeeWrapper.vmMirror.clearBreakpoint(breakpoint2);

        // Set breakpoint again to be sure we cleared breakpoints correctly in the runtime.
        int breakpoint3 = setBreakpoint(classID);
        debuggeeWrapper.vmMirror.clearBreakpoint(breakpoint3);

        synchronizer.sendMessage(JPDADebuggeeSynchronizer.SGNL_CONTINUE);

        logWriter.println("testSetAndClearMultipleBreakpoint done");
    }

    /**
     * This testcase is for BREAKPOINT event. It checks we can set
     * multiple breakpoints at the same location.
     * <BR>It runs BreakpointDebuggee and sets two breakpoints in the
     * breakpointTest method, then checks we receive both events at
     * the same time.
     */
    public void testSetMultipleBreakpoint() {
        logWriter.println("testSetMultipleBreakpoint started");

        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_READY);

        // Set breakpoints.
        long classID = getClassIDBySignature(getDebuggeeClassSignature());
        int breakpoint1 = setBreakpoint(classID);
        int breakpoint2 = setBreakpoint(classID);

        synchronizer.sendMessage(JPDADebuggeeSynchronizer.SGNL_CONTINUE);

        // Wait for event.
        CommandPacket event = debuggeeWrapper.vmMirror.receiveEvent();

        // Check we received all our BREAKPOINT events at the same time.
        ParsedEvent[] parsedEvents = ParsedEvent.parseEventPacket(event);
        assertEquals("Invalid number of events,", 2, parsedEvents.length);
        Set<Integer> request_ids = new HashSet<Integer>();
        checkBreakpointEvent(parsedEvents[0], request_ids);
        checkBreakpointEvent(parsedEvents[1], request_ids);
        assertTrue("Breakpoint 1 is missing", request_ids.contains(breakpoint1));
        assertTrue("Breakpoint 2 is missing", request_ids.contains(breakpoint2));

        // Clear breakpoints.
        debuggeeWrapper.vmMirror.clearBreakpoint(breakpoint1);
        debuggeeWrapper.vmMirror.clearBreakpoint(breakpoint2);

        // Resume debuggee so the test ends cleanly.
        resumeDebuggee();

        logWriter.println("testSetMultipleBreakpoint done");
    }

    /**
     * Sets a breakpoint at the start of the "breakpointTest"
     * method and returns its request ID.
     */
    private int setBreakpoint(long classID) {
        VmMirror mirror = debuggeeWrapper.vmMirror;
        return mirror.setBreakpointAtMethodBegin(classID, "breakpointTest");
    }

    /**
     * Checks the given event is a BREAKPOINT event and adds its request
     * ID to the given request set.
     */
    private void checkBreakpointEvent(ParsedEvent parsedEvent, Set<Integer> request_ids) {
        byte eventKind = parsedEvent.getEventKind();
        assertEquals("Invalid event kind,",
                     JDWPConstants.EventKind.BREAKPOINT,
                     eventKind,
                     JDWPConstants.EventKind.getName(JDWPConstants.EventKind.BREAKPOINT),
                     JDWPConstants.EventKind.getName(eventKind));
        request_ids.add(parsedEvent.getRequestID());
    }
}
