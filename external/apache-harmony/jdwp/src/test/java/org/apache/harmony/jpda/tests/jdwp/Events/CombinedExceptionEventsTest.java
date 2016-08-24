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
import org.apache.harmony.jpda.tests.framework.jdwp.ParsedEvent;
import org.apache.harmony.jpda.tests.framework.jdwp.ReplyPacket;
import org.apache.harmony.jpda.tests.jdwp.Events.CombinedExceptionEventsDebuggee.SubDebuggeeException;
import org.apache.harmony.jpda.tests.share.JPDADebuggeeSynchronizer;

import java.util.Arrays;

/**
 * JDWP Unit test for combined EXCEPTION events.
 */
public class CombinedExceptionEventsTest extends CombinedEventsTestCase {
    @Override
    protected String getDebuggeeClassName() {
        return CombinedExceptionEventsDebuggee.class.getName();
    }

    /**
     * Tests combined EXCEPTION events for caught exception. It runs the
     * CombinedExceptionEventsDebuggee and verifies we only received
     * EXCEPTION events for caught exception.
     */
    public void testCombinedExceptionEvents_CaughtExceptionOnly() {
        runCombinedExceptionEventsTest(false);
    }

    /**
     * Tests combined EXCEPTION events for uncaught exception. It runs the
     * CombinedExceptionEventsDebuggee and verifies we only received
     * EXCEPTION events for uncaught exception.
     */
    public void testCombinedExceptionEvents_UncaughtExceptionOnly() {
        runCombinedExceptionEventsTest(true);
    }

    /**
     * Tests combined EXCEPTION events. It runs the CombinedExceptionEventsDebuggee
     * and requests the following EXCEPTION events:
     * <ol>
     * <li>only caught DebuggeeException (and subclasses)</li>
     * <li>only caught SubDebuggeeException (and subclasses)</li>
     * <li>only uncaught DebuggeeException (and subclasses)</li>
     * <li>only uncaught SubDebuggeeException (and subclasses)</li>
     * <li>caught and uncaught DebuggeeException (and subclasses)</li>
     * <li>caught and uncaught SubDebuggeeException (and subclasses)</li>
     * </ol>
     *
     * Finally it verifies we received only the expected events.
     *
     * @param testUncaughtException
     *          true to test uncaught exception, false to test caught exception.
     */
    private void runCombinedExceptionEventsTest(boolean testUncaughtException) {
        // Wait for debuggee.
        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_READY);

        String superExceptionClassSignature = getClassSignature(
                DebuggeeException.class);
        String subExceptionClassSignature = getClassSignature(
                SubDebuggeeException.class);

        long superExceptionClassID = debuggeeWrapper.vmMirror.getClassID(
                superExceptionClassSignature);
        long subExceptionClassID = debuggeeWrapper.vmMirror.getClassID(
                subExceptionClassSignature);

        // Request "caught only" exceptions with super and sub classes.
        int request1 = requestException(superExceptionClassID, true, false);
        int request2 = requestException(subExceptionClassID, true, false);

        // Request "uncaught only" exceptions with super and sub classes.
        int request3 = requestException(superExceptionClassID, false, true);
        int request4 = requestException(subExceptionClassID, false, true);

        // Request "caught & uncaught" exceptions with super and sub classes.
        int request5 = requestException(superExceptionClassID, true, true);
        int request6 = requestException(subExceptionClassID, true, true);

        int[] expectedRequests = null;
        String signalMessage = null;
        if (testUncaughtException) {
            // We expect requests 3, 4, 5 and 6.
            expectedRequests = new int[] { request3, request4, request5, request6 };
            signalMessage = CombinedExceptionEventsDebuggee.TEST_UNCAUGHT_EXCEPTION_SIGNAL;
        } else {
            // We expect requests 1, 2, 5 and 6.
            expectedRequests = new int[] { request1, request2, request5, request6 };
            signalMessage = CombinedExceptionEventsDebuggee.TEST_CAUGHT_EXCEPTION_SIGNAL;
        }

        // Signal debuggee.
        synchronizer.sendMessage(signalMessage);

        printTestLog("=> receiveEvent()...");
        EventPacket event = debuggeeWrapper.vmMirror.receiveEvent();
        ParsedEvent[] parsedEvents = ParsedEvent.parseEventPacket(event);
        printTestLog("Received " + parsedEvents.length + " + event(s).");

        int[] receivedRequests = new int[parsedEvents.length];
        long eventThreadID = -1;
        long exceptionObjectID = -1;
        for (int i = 0; i < parsedEvents.length; ++i) {
            ParsedEvent parsedEvent = parsedEvents[i];
            byte eventKind = parsedEvent.getEventKind();
            int requestID = parsedEvent.getRequestID();
            printTestLog("Event #" + i + ": kind="
                    + JDWPConstants.EventKind.getName(eventKind)
                    + ", requestID=" + requestID);
            assertEquals("Invalid event kind,",
                    JDWPConstants.EventKind.EXCEPTION, eventKind,
                    JDWPConstants.EventKind.getName(
                            JDWPConstants.EventKind.EXCEPTION),
                    JDWPConstants.EventKind.getName(eventKind));

            ParsedEvent.Event_EXCEPTION exceptionEvent = (ParsedEvent.Event_EXCEPTION) parsedEvent;

            long currentEventThreadID = exceptionEvent.getThreadID();
            long currentExceptionObjectID = exceptionEvent
                    .getException().objectID;
            // Checks all events are for the same thread.
            if (eventThreadID != -1) {
                assertEquals("Invalid event thread ID", eventThreadID,
                        currentEventThreadID);
            } else {
                eventThreadID = currentEventThreadID;
            }
            // Checks all events are for the same exception.
            if (exceptionObjectID != -1) {
                assertEquals("Invalid exception object ID", exceptionObjectID,
                        currentExceptionObjectID);
            } else {
                exceptionObjectID = currentExceptionObjectID;
            }
            // Check thread is properly suspended.
            checkThreadState(eventThreadID, JDWPConstants.ThreadStatus.RUNNING,
                    JDWPConstants.SuspendStatus.SUSPEND_STATUS_SUSPENDED);

            receivedRequests[i] = requestID;
        }

        // Check we receive all expected events.
        Arrays.sort(expectedRequests);
        Arrays.sort(receivedRequests);
        if (!Arrays.equals(expectedRequests, receivedRequests)) {
            String expectedArrayString = Arrays.toString(expectedRequests);
            String receivedArrayString = Arrays.toString(receivedRequests);
            fail("Unexpected event: expected " + expectedArrayString
                    + " but got " + receivedArrayString);
        }
    }

    private int requestException(long exceptionClassID, boolean caught,
            boolean uncaught) {
        printTestLog("Request EXCEPTION event: class=" + exceptionClassID + ", caught="
                + caught + ", uncaught=" + uncaught);
        ReplyPacket replyPacket = debuggeeWrapper.vmMirror.setException(
                exceptionClassID, caught, uncaught);
        int requestID = replyPacket.getNextValueAsInt();
        assertAllDataRead(replyPacket);
        printTestLog("Created request " + requestID);
        return requestID;
    }
}
