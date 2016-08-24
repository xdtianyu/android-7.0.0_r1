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

/**
 * @author Anatoly F. Bondarenko
 */

/**
 * Created on 06.10.2006
 */
package org.apache.harmony.jpda.tests.jdwp.Events;

import org.apache.harmony.jpda.tests.framework.jdwp.CommandPacket;
import org.apache.harmony.jpda.tests.framework.jdwp.JDWPConstants;
import org.apache.harmony.jpda.tests.framework.jdwp.Location;
import org.apache.harmony.jpda.tests.framework.jdwp.ParsedEvent;
import org.apache.harmony.jpda.tests.framework.jdwp.ReplyPacket;
import org.apache.harmony.jpda.tests.share.JPDADebuggeeSynchronizer;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;


/**
 * JDWP Unit test for METHOD_ENTRY, METHOD_EXIT events for empty method.
 */
public class CombinedEvents002Test extends CombinedEventsTestCase {
    static final String TESTED_CLASS_NAME =
        CombinedEvents002Debuggee.TESTED_CLASS_NAME;
    static final String TESTED_CLASS_SIGNATURE =
        CombinedEvents002Debuggee.TESTED_CLASS_SIGNATURE;
    static final String TESTED_METHOD_NAME = CombinedEvents002Debuggee.TESTED_METHOD_NAME;

    private long testedClassID = -1;
    private long testedMethodID = -1;
    private long testedMethodStartCodeIndex = -1;
    private long testedMethodEndCodeIndex = -1;
    private Map<Byte, Integer> requestsMap = new HashMap<>();

    @Override
    protected String getDebuggeeClassName() {
        return CombinedEvents002Debuggee.class.getName();
    }

    /**
     * This testcase is for METHOD_ENTRY, METHOD_EXIT events for empty method.
     * <BR>It runs CombinedEvents002Debuggee that executed its own empty method
     * and verify that requested METHOD_ENTRY, METHOD_EXIT events occur
     * for empty method.
     */
    public void testCombinedEvents002_01() {
        byte[] expectedEventKinds = {
                JDWPConstants.EventKind.METHOD_ENTRY,
                JDWPConstants.EventKind.METHOD_EXIT
        };
        runTest(expectedEventKinds);
    }

    /**
     * This testcase is for METHOD_ENTRY, METHOD_EXIT_WITH_RETURN_VALUE events for empty method.
     * <BR>It runs CombinedEvents002Debuggee that executed its own empty method
     * and verify that requested METHOD_ENTRY, METHOD_EXIT_WITH_RETURN_VALUE events occur
     * for empty method.
     */
    public void testCombinedEvents002_02() {
        byte[] expectedEventKinds = {
                JDWPConstants.EventKind.METHOD_ENTRY,
                JDWPConstants.EventKind.METHOD_EXIT_WITH_RETURN_VALUE
        };
        runTest(expectedEventKinds);
    }

    private void runTest(byte[] expectedEventKinds) {
        logWriter.println("==> " + getName() + ": Start...");

        prepareDebuggee(expectedEventKinds);
        List<ParsedEvent> receivedEvents = receiveEvents();
        checkEvents(receivedEvents, expectedEventKinds);
        clearEvents();

        logWriter.println("==> Resume debuggee VM...");
        debuggeeWrapper.vmMirror.resume();
        logWriter.println("==> " + getName() + ": PASSED! ");
    }

    /**
     * Computes JDWP ids and requests events.
     */
    private void prepareDebuggee(byte[] expectedEventKinds) {
        logWriter.println("==> Wait for SGNL_READY signal from debuggee...");
        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_READY);
        logWriter.println("==> OK - SGNL_READY signal received!");

        testedClassID =
            debuggeeWrapper.vmMirror.getClassID(TESTED_CLASS_SIGNATURE);
        if ( testedClassID == -1 ) {
            String failureMessage = "## FAILURE: Can NOT get ClassID for '"
                + TESTED_CLASS_SIGNATURE + "'";
            printErrorAndFail(failureMessage);
        }
        logWriter.println("==> Tested Class Name = '" + TESTED_CLASS_NAME + "'");
        logWriter.println("==> testedClassID = " + testedClassID);

        logWriter.println("==> ");
        logWriter.println("==> Info for tested method '" + TESTED_METHOD_NAME + "':");
        testedMethodID = debuggeeWrapper.vmMirror.getMethodID(testedClassID, TESTED_METHOD_NAME);
        if (testedMethodID == -1 ) {
            String failureMessage = "## FAILURE: Can NOT get MethodID for class '"
                + TESTED_CLASS_NAME + "'; Method name = " + TESTED_METHOD_NAME;
            printErrorAndFail(failureMessage);
        }
        logWriter.println("==> testedMethodID = " + testedMethodID);
        printMethodLineTable(testedClassID, null, TESTED_METHOD_NAME);
        testedMethodStartCodeIndex = getMethodStartCodeIndex(testedClassID, TESTED_METHOD_NAME);
        if ( testedMethodStartCodeIndex == -1 ) {
            String failureMessage = "## FAILURE: Can NOT get MethodStartCodeIndex for method '"
                + TESTED_METHOD_NAME + "' ";
            printErrorAndFail(failureMessage);
        }
        testedMethodEndCodeIndex = getMethodEndCodeIndex(testedClassID, TESTED_METHOD_NAME);
        if ( testedMethodEndCodeIndex == -1 ) {
            String failureMessage = "## FAILURE: Can NOT get MethodEndCodeIndex for method '"
                + TESTED_METHOD_NAME + "' ";
            printErrorAndFail(failureMessage);
        }

        // Request events.
        for (byte eventKind : expectedEventKinds) {
            String eventKindName = JDWPConstants.EventKind.getName(eventKind);
            logWriter.println("==> ");
            logWriter.println("==> Set request for " + eventKindName +
                    " event for '" + TESTED_CLASS_NAME + "'... ");
            ReplyPacket reply = null;
            switch (eventKind) {
            case JDWPConstants.EventKind.METHOD_ENTRY:
                reply = debuggeeWrapper.vmMirror.setMethodEntry(TESTED_CLASS_NAME);
                break;
            case JDWPConstants.EventKind.METHOD_EXIT:
                reply = debuggeeWrapper.vmMirror.setMethodExit(TESTED_CLASS_NAME);
                break;
            case JDWPConstants.EventKind.METHOD_EXIT_WITH_RETURN_VALUE:
                reply = debuggeeWrapper.vmMirror.setMethodExitWithReturnValue(TESTED_CLASS_NAME);
                break;
            }
            checkReplyPacket(reply, "Set " + eventKindName + " event.");  //DBG needless ?
            int requestId = reply.getNextValueAsInt();
            requestsMap.put(Byte.valueOf(eventKind), Integer.valueOf(requestId));
            logWriter.println("==> OK - request " + requestId + " for " + eventKind +
                    " event is set!");
        }

        logWriter.println("==> Send SGNL_CONTINUE signal to debuggee...");
        synchronizer.sendMessage(JPDADebuggeeSynchronizer.SGNL_CONTINUE);
    }

    /**
     * Receives events from the debuggee
     */
    private List<ParsedEvent> receiveEvents() {
        List<ParsedEvent> receivedEvents = new ArrayList<ParsedEvent>();
        logWriter.println("==> ");
        logWriter.println("==> Receiving events... ");
        CommandPacket event = debuggeeWrapper.vmMirror.receiveEvent();
        ParsedEvent[] parsedEvents = ParsedEvent.parseEventPacket(event);

        int receivedEventsNumber = parsedEvents.length;
        logWriter.println("==> Number of received events in event packet = " + receivedEventsNumber);
        for (int i = 0; i < receivedEventsNumber; ++i) {
            receivedEvents.add(parsedEvents[i]);

            byte eventKind = parsedEvents[i].getEventKind();
            eventKind = parsedEvents[i].getEventKind();
            logWriter.println("==> Received event[" + i + "] kind = "
                +  eventKind
                + "(" + JDWPConstants.EventKind.getName(eventKind) + ")");
        }
        if (receivedEventsNumber > 2) {
            String failureMessage = "## FAILURE: Unexpected number of received events in packet = "
                + receivedEventsNumber + "\n## Expected number of received events in packet = 1 or 2";
            printErrorAndFail(failureMessage);
        }
        if (receivedEventsNumber == 1) {
            logWriter.println("==> ");
            logWriter.println("==> Resume debuggee VM...");
            debuggeeWrapper.vmMirror.resume();
            logWriter.println("==> Receiving events... ");
            event = debuggeeWrapper.vmMirror.receiveEvent();
            parsedEvents = ParsedEvent.parseEventPacket(event);

            receivedEventsNumber = parsedEvents.length;
            logWriter.println("==> Number of received events in event packet = " + receivedEventsNumber);
            for (int i = 0; i < receivedEventsNumber; ++i) {
                receivedEvents.add(parsedEvents[i]);

                byte eventKind = parsedEvents[i].getEventKind();
                logWriter.println("==> Received event[" + i + "] kind = "
                    +  eventKind
                    + "(" + JDWPConstants.EventKind.getName(eventKind) + ")");
            }
            if (receivedEventsNumber != 1) {
                String failureMessage = "## FAILURE: Unexpected number of received events in packet = "
                    + receivedEventsNumber + "\n## Expected number of received events in packet = 1";
                printErrorAndFail(failureMessage);
            }
        }
        return receivedEvents;
    }

    /**
     * Checks we received expected events from the debuggee.
     */
    private void checkEvents(List<ParsedEvent> receivedEvents,
            byte[] expectedEventKinds) {
        boolean testCaseIsOk = true;
        byte[] receivedEventKinds = new byte[receivedEvents.size()];
        for (int i = 0, e = receivedEvents.size(); i < e; ++i) {
            logWriter.println("==> ");
            logWriter.println("==> Check received event #" + i + "...");
            ParsedEvent parsedEvent = receivedEvents.get(i);
            byte eventKind = parsedEvent.getEventKind();
            receivedEventKinds[i] = eventKind;
            switch (eventKind) {
            case JDWPConstants.EventKind.METHOD_ENTRY:
                testCaseIsOk &= checkMethodEntryEvent(parsedEvent);
                break;
            case JDWPConstants.EventKind.METHOD_EXIT:
                testCaseIsOk &= checkMethodExitEvent(parsedEvent);
                break;
            case JDWPConstants.EventKind.METHOD_EXIT_WITH_RETURN_VALUE:
                testCaseIsOk &= checkMethodExitWithReturnValueEvent(parsedEvent);
                break;
            }
        }
        if (!testCaseIsOk) {
            String failureMessage = "## FAILURE: Unexpected events attributes are found out!";
            printErrorAndFail(failureMessage);
        }

        // Check that we received all expected events.
        Arrays.sort(expectedEventKinds);
        Arrays.sort(receivedEventKinds);
        if (!Arrays.equals(expectedEventKinds, receivedEventKinds)) {
            String failureMessage = "## FAILURE: Did not receive all expected events!";
            printErrorAndFail(failureMessage);
        }
    }

    private boolean checkMethodEntryEvent(ParsedEvent parsedEvent) {
        ParsedEvent.Event_METHOD_ENTRY methodEntryEvent =
                (ParsedEvent.Event_METHOD_ENTRY) parsedEvent;
        Location expectedLocation = new Location(JDWPConstants.TypeTag.CLASS, testedClassID,
                testedMethodID, testedMethodStartCodeIndex);
        return checkEventLocation(methodEntryEvent, expectedLocation);
    }

    private boolean checkMethodExitEvent(ParsedEvent parsedEvent) {
        ParsedEvent.Event_METHOD_EXIT methodExitEvent =
                (ParsedEvent.Event_METHOD_EXIT) parsedEvent;
        Location expectedLocation = new Location(JDWPConstants.TypeTag.CLASS, testedClassID,
                testedMethodID, testedMethodEndCodeIndex);
        return checkEventLocation(methodExitEvent, expectedLocation);
    }

    private boolean checkMethodExitWithReturnValueEvent(ParsedEvent parsedEvent) {
        ParsedEvent.Event_METHOD_EXIT_WITH_RETURN_VALUE methodExitWithReturnValueEvent =
                (ParsedEvent.Event_METHOD_EXIT_WITH_RETURN_VALUE) parsedEvent;
        Location expectedLocation = new Location(JDWPConstants.TypeTag.CLASS, testedClassID,
                testedMethodID, testedMethodEndCodeIndex);
        boolean result = checkEventLocation(methodExitWithReturnValueEvent, expectedLocation);
        // Expect null return value because method is 'void'.
        if (methodExitWithReturnValueEvent.getReturnValue() != null) {
            logWriter.println("## FAILURE: Unexpected return value in event!");
            logWriter.println("##          Expected null");
            result = false;
        } else {
            logWriter.println("==> OK - it is expected return value tag");
        }
        return result;
    }

    /**
     * Clear event requests.
     */
    private void clearEvents() {
        for (Byte eventKind : requestsMap.keySet()) {
            Integer requestId = requestsMap.get(eventKind);
            logWriter.println("==> ");
            logWriter.println("==> Clear request " + requestId.intValue() + " for " +
                    JDWPConstants.EventKind.getName(eventKind.byteValue()));
            debuggeeWrapper.vmMirror.clearEvent(eventKind.byteValue(), requestId.intValue());
        }
        requestsMap.clear();
    }
}
