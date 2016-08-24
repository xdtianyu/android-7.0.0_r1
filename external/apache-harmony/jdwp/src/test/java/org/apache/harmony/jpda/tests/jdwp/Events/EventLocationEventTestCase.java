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

import org.apache.harmony.jpda.tests.framework.jdwp.Event;
import org.apache.harmony.jpda.tests.framework.jdwp.EventBuilder;
import org.apache.harmony.jpda.tests.framework.jdwp.EventPacket;
import org.apache.harmony.jpda.tests.framework.jdwp.JDWPConstants;
import org.apache.harmony.jpda.tests.framework.jdwp.Location;
import org.apache.harmony.jpda.tests.framework.jdwp.ParsedEvent;
import org.apache.harmony.jpda.tests.framework.jdwp.ReplyPacket;
import org.apache.harmony.jpda.tests.share.JPDADebuggeeSynchronizer;

import java.util.HashSet;
import java.util.Set;

/**
 * Base class to test event with LocationOnly modifier.
 */
abstract class EventLocationEventTestCase extends JDWPEventTestCase {

    private Set<Integer> requestIds = new HashSet<Integer>();

    protected abstract String getDebuggeeSignature();
    protected abstract String getExpectedLocationMethodName();
    protected abstract void createEventBuilder(EventBuilder builder);
    protected abstract void checkEvent(ParsedEvent event);

    protected void runEventWithLocationTest(byte eventKind) {
        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_READY);

        // Request event for all possible locations in the expected
        // method.
        requestEventForAllLocations(eventKind);

        synchronizer.sendMessage(JPDADebuggeeSynchronizer.SGNL_CONTINUE);

        // Wait for the event.
        EventPacket event = debuggeeWrapper.vmMirror.receiveEvent();
        ParsedEvent[] parsedEvents = ParsedEvent.parseEventPacket(event);

        // We expect only one event.
        assertEquals("Invalid number of events,", 1, parsedEvents.length);

        ParsedEvent parsedEvent = parsedEvents[0];

        // Check this is the event we expect.
        assertEquals("Invalid event kind,",
                eventKind,
                parsedEvent.getEventKind(),
                JDWPConstants.EventKind.getName(eventKind),
                JDWPConstants.EventKind.getName(parsedEvent.getEventKind()));

        // Check this is one event we requested.
        int eventRequestId = parsedEvent.getRequestID();
        assertTrue("Unexpected event request " + eventRequestId,
                requestIds.contains(Integer.valueOf(eventRequestId)));

        // Check the event is the expected one.
        checkEvent(parsedEvent);

        // Clear all event requests.
        clearAllEvents(eventKind);

        // Resume debuggee before leaving.
        resumeDebuggee();
    }

    /**
     * Since we don't know the location where the event can be reported,
     * we send a request for all possible locations inside the method.
     */
    private void requestEventForAllLocations(byte eventKind) {
        // Ensure we start with no request.
        requestIds.clear();

        // Find the method where we expect the event to occur.
        long typeId = getClassIDBySignature(getDebuggeeSignature());
        long methodId = getMethodID(typeId, getExpectedLocationMethodName());

        // Get its line table
        ReplyPacket replyPacket = getLineTable(typeId, methodId);
        long startIndex = replyPacket.getNextValueAsLong();
        long endIndex = replyPacket.getNextValueAsLong();
        logWriter.println("Method code index starts at " + startIndex +
                " and ends at " + endIndex);

        // Request event at all possible locations. We'd like to do
        // this for each code instruction but we do not know them and
        // do not know their size. Therefore we include any code
        // index between start and end.
        logWriter.println("Creating request for each possible index");
        for (long idx = startIndex; idx <= endIndex; ++idx) {
            Location location = new Location(JDWPConstants.TypeTag.CLASS,
                    typeId, methodId, idx);
            EventBuilder builder = new EventBuilder(eventKind,
                            JDWPConstants.SuspendPolicy.ALL);
            createEventBuilder(builder);
            setEvent(builder, location);

        }
        logWriter.println("Created " + requestIds.size() + " requests");
    }

    private void setEvent(EventBuilder builder, Location location) {
        builder.setLocationOnly(location);
        Event event = builder.build();
        ReplyPacket reply = debuggeeWrapper.vmMirror.setEvent(event);
        int requestId = reply.getNextValueAsInt();
        logWriter.println("=> New request " + requestId);
        requestIds.add(Integer.valueOf(requestId));
    }

    private void clearAllEvents(byte eventKind) {
        logWriter.println("Clear all field requests");
        for (Integer requestId : requestIds) {
            clearEvent(eventKind, requestId.intValue());
        }
        requestIds.clear();
    }

    private void clearEvent(byte fieldEventKind, int requestId) {
        logWriter.println("=> Clear request " + requestId);
        debuggeeWrapper.vmMirror.clearEvent(fieldEventKind, requestId);
    }
}
