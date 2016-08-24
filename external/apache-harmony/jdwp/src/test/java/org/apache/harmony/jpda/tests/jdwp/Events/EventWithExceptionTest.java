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
import org.apache.harmony.jpda.tests.framework.jdwp.JDWPConstants.StepDepth;
import org.apache.harmony.jpda.tests.framework.jdwp.Location;
import org.apache.harmony.jpda.tests.framework.jdwp.ParsedEvent;
import org.apache.harmony.jpda.tests.framework.jdwp.ReplyPacket;
import org.apache.harmony.jpda.tests.share.JPDADebuggeeSynchronizer;

/**
 * JDWP unit test checking that we can report different events, requested during
 * an EXCEPTION event where the exception is caught (by a catch handler).
 */
public class EventWithExceptionTest extends JDWPEventTestCase {
    private static final String TEST_METHOD_NAME = "catchMethod";

    private static final String WATCHED_FIELD_NAME = "watchedField";

    private static final Class<? extends Throwable> EXCEPTION_CLASS =
            EventWithExceptionDebuggee.TestException.class;

    @Override
    protected String getDebuggeeClassName() {
        return EventWithExceptionDebuggee.class.getName();
    }

    /**
     * Tests we properly SINGLE-STEP OUT to the catch handler.
     */
    public void testSingleStepOut() {
        runSingleStepTest(JDWPConstants.StepDepth.OUT);
    }

    /**
     * Tests we properly SINGLE-STEP OVER to the catch handler.
     */
    public void testSingleStepOver() {
        runSingleStepTest(JDWPConstants.StepDepth.OVER);
    }

    /**
     * Tests we properly SINGLE-STEP INTO to the catch handler.
     */
    public void testSingleStepInto() {
        runSingleStepTest(JDWPConstants.StepDepth.INTO);
    }

    /**
     * Tests we properly BREAKPOINT in the catch handler if we request it before the EXCEPTION
     * event happens.
     */
    public void testBreakpoint_BeforeException() {
        logWriter.println(getName() + " STARTS");

        // Wait for debuggee.
        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_READY);

        // Set EXCEPTION event
        int exceptionRequestId = setException();

        synchronizer.sendMessage(JPDADebuggeeSynchronizer.SGNL_CONTINUE);

        // Wait for EXCEPTION event
        ParsedEvent exceptionEvent = waitForEvent(JDWPConstants.EventKind.EXCEPTION,
                exceptionRequestId);

        // Get catch handler location
        Location catchHandlerLocation =
                ((ParsedEvent.Event_EXCEPTION) exceptionEvent).getCatchLocation();

        // Remove EXCEPTION event.
        clearEvent(JDWPConstants.EventKind.EXCEPTION, exceptionRequestId, true);

        // Resume debuggee suspended on EXCEPTION event.
        logWriter.println("Resume debuggee after EXCEPTION event");
        debuggeeWrapper.resume();

        // Wait for debuggee.
        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_READY);

        // Install BREAKPOINT in catch handler.
        int breakpointRequestId = setBreakpoint(catchHandlerLocation);

        synchronizer.sendMessage(JPDADebuggeeSynchronizer.SGNL_CONTINUE);

        // Wait for BREAKPOINT.
        ParsedEvent breakpointEvent = waitForEvent(JDWPConstants.EventKind.BREAKPOINT,
                breakpointRequestId);

        // Check location of BREAKPOINT event.
        Location eventLocation = ((ParsedEvent.EventThreadLocation) breakpointEvent).getLocation();
        assertEquals("Not the expected location", catchHandlerLocation, eventLocation);

        // Remove BREAKPOINT request.
        clearEvent(JDWPConstants.EventKind.BREAKPOINT, breakpointRequestId, true);

        // Resume debuggee suspended on BREAKPOINT event.
        logWriter.println("Resume debuggee after BREAKPOINT event");
        debuggeeWrapper.resume();

        // Continue debuggee
        synchronizer.sendMessage(JPDADebuggeeSynchronizer.SGNL_CONTINUE);

        logWriter.println(getName() + " ENDS");
    }

    /**
     * Tests we properly BREAKPOINT in the catch handler if we request it upon the EXCEPTION
     * event happens.
     */
    public void testBreakpoint_UponException() {
        logWriter.println(getName() + " STARTS");

        // Wait for debuggee.
        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_READY);

        // Set EXCEPTION event
        int exceptionRequestId = setException();

        synchronizer.sendMessage(JPDADebuggeeSynchronizer.SGNL_CONTINUE);

        // Wait for EXCEPTION event
        ParsedEvent exceptionEvent = waitForEvent(JDWPConstants.EventKind.EXCEPTION,
                exceptionRequestId);

        // Get catch handler location
        Location catchHandlerLocation =
                ((ParsedEvent.Event_EXCEPTION) exceptionEvent).getCatchLocation();

        // Resume debuggee suspended on EXCEPTION event.
        logWriter.println("Resume debuggee after EXCEPTION event");
        debuggeeWrapper.resume();

        // Wait for debuggee.
        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_READY);

        synchronizer.sendMessage(JPDADebuggeeSynchronizer.SGNL_CONTINUE);

        waitForEvent(JDWPConstants.EventKind.EXCEPTION, exceptionRequestId);

        // Install BREAKPOINT in catch handler.
        int breakpointRequestId = setBreakpoint(catchHandlerLocation);

        debuggeeWrapper.resume();

        // Wait for BREAKPOINT.
        ParsedEvent breakpointEvent = waitForEvent(JDWPConstants.EventKind.BREAKPOINT,
                breakpointRequestId);

        // Check location of BREAKPOINT event.
        Location eventLocation = ((ParsedEvent.EventThreadLocation) breakpointEvent).getLocation();
        assertEquals("Not the expected location", catchHandlerLocation, eventLocation);

        // Remove BREAKPOINT request.
        clearEvent(JDWPConstants.EventKind.BREAKPOINT, breakpointRequestId, true);

        // Resume debuggee suspended on BREAKPOINT event.
        logWriter.println("Resume debuggee after BREAKPOINT event");
        debuggeeWrapper.resume();

        // Continue debuggee
        synchronizer.sendMessage(JPDADebuggeeSynchronizer.SGNL_CONTINUE);

        logWriter.println(getName() + " ENDS");
    }

    /**
     * Tests we properly FIELD_ACCESS in the method catching the exception.
     */
    public void testFieldAccess() {
        runFieldWatchpointTest(JDWPConstants.EventKind.FIELD_ACCESS);
    }

    /**
     * Tests we properly FIELD_MODIFICATION in the method catching the exception.
     */
    public void testFieldModification() {
        runFieldWatchpointTest(JDWPConstants.EventKind.FIELD_MODIFICATION);
    }

    /**
     * Tests we properly detect METHOD_EXIT event in the method catching the exception.
     */
    public void testMethodExit() {
        runMethodExitTest(JDWPConstants.EventKind.METHOD_EXIT);
    }

    /**
     * Tests we properly detect METHOD_EXIT event in the method catching the exception.
     */
    public void testMethodExitWithReturnValue() {
        runMethodExitTest(JDWPConstants.EventKind.METHOD_EXIT_WITH_RETURN_VALUE);
    }

    /**
     * Tests we properly single-step to the catch handler.
     *
     * @param singleStepDepth
     *          the kind of single-step
     */
    private void runSingleStepTest(byte singleStepDepth) {
        logWriter.println(getName() + " STARTS");

        // Wait for debuggee.
        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_READY);

        // Set EXCEPTION event
        int exceptionRequestId = setException();

        synchronizer.sendMessage(JPDADebuggeeSynchronizer.SGNL_CONTINUE);

        // Wait for EXCEPTION event
        ParsedEvent exceptionEvent = waitForEvent(JDWPConstants.EventKind.EXCEPTION,
                exceptionRequestId);

        // Get catch handler location
        Location catchHandlerLocation =
                ((ParsedEvent.Event_EXCEPTION) exceptionEvent).getCatchLocation();

        // Resume debuggee suspended on EXCEPTION event.
        logWriter.println("Resume debuggee after EXCEPTION event #1");
        debuggeeWrapper.resume();

        // Wait for debuggee.
        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_READY);

        synchronizer.sendMessage(JPDADebuggeeSynchronizer.SGNL_CONTINUE);

        // Wait for EXCEPTION event
        ParsedEvent parsedEvent = waitForEvent(JDWPConstants.EventKind.EXCEPTION, exceptionRequestId);
        long eventThreadId = ((ParsedEvent.EventThread)parsedEvent).getThreadID();

        // Install SINGLE_STEP in catch handler.
        int singleStepRequestId = setSingleStep(eventThreadId, singleStepDepth);

        // Resume debuggee suspended on EXCEPTION event.
        logWriter.println("Resume debuggee after EXCEPTION event #2");
        debuggeeWrapper.resume();

        // Wait for SINGLE_STEP.
        ParsedEvent singleStepEvent = waitForEvent(JDWPConstants.EventKind.SINGLE_STEP,
                singleStepRequestId);

        // Check location of SINGLE_STEP event.
        Location eventLocation = ((ParsedEvent.EventThreadLocation) singleStepEvent).getLocation();
        assertEquals("Not the expected location, ", catchHandlerLocation, eventLocation);

        // Remove SINGLE_STEP request.
        clearEvent(JDWPConstants.EventKind.SINGLE_STEP, singleStepRequestId, true);

        // Resume debuggee suspended on SINGLE_STEP event.
        logWriter.println("Resume debuggee after SINGLE_STEP event");
        debuggeeWrapper.resume();

        // Continue debuggee
        synchronizer.sendMessage(JPDADebuggeeSynchronizer.SGNL_CONTINUE);

        logWriter.println(getName() + " ENDS");
    }

    /**
     * Tests we properly detect field watchpoint events in the method catching the exception.
     *
     * @param fieldEventKind
     *          the desired field event kind
     */
    private void runFieldWatchpointTest(byte fieldEventKind) {
        logWriter.println(getName() + " STARTS");

        // Skip first method call since we do not need to know the location of
        // the catch handler.
        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_READY);
        synchronizer.sendMessage(JPDADebuggeeSynchronizer.SGNL_CONTINUE);

        // Wait for second method call.
        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_READY);

        // Set EXCEPTION event
        int exceptionRequestId = setException();

        synchronizer.sendMessage(JPDADebuggeeSynchronizer.SGNL_CONTINUE);

        // Wait for EXCEPTION event
        waitForEvent(JDWPConstants.EventKind.EXCEPTION, exceptionRequestId);

        // Install FIELD watchpoint event
        int fieldWatchpointRequestId = setFieldWatchpoint(fieldEventKind);

        // Resume debuggee suspended on EXCEPTION event.
        logWriter.println("Resume debuggee after EXCEPTION event");
        debuggeeWrapper.resume();

        // Wait for FIELD event.
        ParsedEvent fieldWatchpointEvent = waitForEvent(fieldEventKind,
                fieldWatchpointRequestId);

        // Check location of FIELD event.
        Location eventLocation =
                ((ParsedEvent.EventThreadLocation) fieldWatchpointEvent).getLocation();
        assertNotNull(eventLocation);
        long classId = getClassIDBySignature(getDebuggeeClassSignature());
        long methodId = getMethodID(classId, TEST_METHOD_NAME);
        assertEquals("Invalid class ID", classId, eventLocation.classID);
        assertEquals("Invalid method ID", methodId, eventLocation.methodID);

        // Remove FIELD watchpoint request.
        clearEvent(fieldEventKind, fieldWatchpointRequestId, true);

        // Resume debuggee suspended on FIELD event.
        logWriter.println("Resume debuggee after " +
                JDWPConstants.EventKind.getName(fieldEventKind) + " event");
        debuggeeWrapper.resume();

        // Continue debuggee
        synchronizer.sendMessage(JPDADebuggeeSynchronizer.SGNL_CONTINUE);

        logWriter.println(getName() + " ENDS");
    }

    /**
     * Tests we properly detect method exit event in the method catching the exception.
     *
     * @param methodExitEventKind
     *          the desired method exit event kind
     */
    private void runMethodExitTest(byte methodExitEventKind) {

        logWriter.println(getName() + " STARTS");

        // Skip first method call since we do not need to know the location of
        // the catch handler.
        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_READY);
        synchronizer.sendMessage(JPDADebuggeeSynchronizer.SGNL_CONTINUE);

        // Wait for second method call.
        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_READY);

        // Set EXCEPTION event
        int exceptionRequestId = setException();

        synchronizer.sendMessage(JPDADebuggeeSynchronizer.SGNL_CONTINUE);

        // Wait for EXCEPTION event
        waitForEvent(JDWPConstants.EventKind.EXCEPTION, exceptionRequestId);

        // Install METHOD_EXIT event
        int methodExitRequestId = setMethodExit(methodExitEventKind);

        logWriter.println("Resume debuggee after EXCEPTION event");
        debuggeeWrapper.resume();

        // Wait for METHOD_EXIT event.
        ParsedEvent methodExitEvent = waitForEvent(methodExitEventKind, methodExitRequestId);

        // Check location of METHOD_EXIT event.
        Location eventLocation =
                ((ParsedEvent.EventThreadLocation) methodExitEvent).getLocation();
        assertNotNull(eventLocation);
        long classId = getClassIDBySignature(getDebuggeeClassSignature());
        long methodId = getMethodID(classId, TEST_METHOD_NAME);
        checkLocation(classId, methodId, eventLocation);

        // Remove METHOD_EXIT request.
        clearEvent(methodExitEventKind, methodExitRequestId, true);

        // Resume debuggee suspended on METHOD_EXIT event.
        logWriter.println("Resume debuggee after " +
                JDWPConstants.EventKind.getName(methodExitEventKind) + " event");
        debuggeeWrapper.resume();

        // Continue debuggee
        synchronizer.sendMessage(JPDADebuggeeSynchronizer.SGNL_CONTINUE);

        logWriter.println(getName() + " ENDS");
    }

    private void checkLocation(long expectedClassId, long expectedMethodId, Location actualLocation) {
        if (expectedClassId != actualLocation.classID || expectedMethodId != actualLocation.methodID) {
            String expectedClassName = getClassSignature(expectedClassId);
            String expectedMethodName = getMethodName(expectedClassId, expectedMethodId);
            String actualClassName = getClassSignature(actualLocation.classID);
            String actualMethodName = getMethodName(actualLocation.classID, actualLocation.methodID);
            fail(String.format("Invalid method, expected  \"%s.%s\" (classId=%d, methodId=%d)" +
                    " but got \"%s.%s\" (classId=%d, methodId=%d)",
                    expectedClassName, expectedMethodName, expectedClassId, expectedMethodId,
                    actualClassName, actualMethodName, actualLocation.classID, actualLocation.methodID));
        }
    }

    private int setException() {
        logWriter.println("Set EXCEPTION on class " + EXCEPTION_CLASS.getName());
        ReplyPacket exceptionReplyPacket =
                debuggeeWrapper.vmMirror.setException(getClassSignature(EXCEPTION_CLASS), true,
                        false);
        checkReplyPacket(exceptionReplyPacket, "Failed to install EXCEPTION");
        return readRequestId(exceptionReplyPacket);
    }

    private int setSingleStep(long eventThreadId, byte singleStepDepth) {
        logWriter.println("Set SINGLE_STEP " + JDWPConstants.StepDepth.getName(singleStepDepth) +
                " in thread " + eventThreadId);
        ReplyPacket replyPacket = debuggeeWrapper.vmMirror.setStep(eventThreadId,
                JDWPConstants.StepSize.LINE, singleStepDepth);
        checkReplyPacket(replyPacket, "Failed to set SINGLE_STEP " +
                StepDepth.getName(singleStepDepth));
        return readRequestId(replyPacket);
    }

    private int setBreakpoint(Location catchHandlerLocation) {
        logWriter.println("Set BREAKPOINT at " + catchHandlerLocation);
        ReplyPacket breakpointReplyPacket =
                debuggeeWrapper.vmMirror.setBreakpoint(catchHandlerLocation);
        checkReplyPacket(breakpointReplyPacket, "Failed to install BREAKPOINT");
        return readRequestId(breakpointReplyPacket);
    }

    private int setFieldWatchpoint(byte fieldEventKind) {
        String eventKindName = JDWPConstants.EventKind.getName(fieldEventKind);
        logWriter.println("Set " + eventKindName + " on field " + WATCHED_FIELD_NAME);
        final String classSignature = getDebuggeeClassSignature();
        final byte classTypeTag = JDWPConstants.TypeTag.CLASS;
        ReplyPacket replyPacket = null;
        if (fieldEventKind == JDWPConstants.EventKind.FIELD_ACCESS) {
            replyPacket = debuggeeWrapper.vmMirror.setFieldAccess(classSignature, classTypeTag,
                    WATCHED_FIELD_NAME);
        } else if (fieldEventKind == JDWPConstants.EventKind.FIELD_MODIFICATION) {
            replyPacket = debuggeeWrapper.vmMirror.setFieldModification(classSignature,
                    classTypeTag, WATCHED_FIELD_NAME);
        } else {
            fail("Unsupported eventkind " + fieldEventKind);
        }
        checkReplyPacket(replyPacket, "Failed to set " + eventKindName);
        return readRequestId(replyPacket);
    }

    private int setMethodExit(byte methodExitEventKind) {
        String eventKindName = JDWPConstants.EventKind.getName(methodExitEventKind);
        logWriter.println("Set " + eventKindName + " on class " + getDebuggeeClassName());
        ReplyPacket replyPacket = null;
        if (methodExitEventKind == JDWPConstants.EventKind.METHOD_EXIT) {
            replyPacket = debuggeeWrapper.vmMirror.setMethodExit(getDebuggeeClassName());
        } else if (methodExitEventKind == JDWPConstants.EventKind.METHOD_EXIT_WITH_RETURN_VALUE) {
            replyPacket =
                    debuggeeWrapper.vmMirror.setMethodExitWithReturnValue(getDebuggeeClassName());
        } else {
            fail("Not a method exit event: " + methodExitEventKind);
        }
        checkReplyPacket(replyPacket, "Failed to set " + eventKindName);
        return readRequestId(replyPacket);
    }

    private int readRequestId(ReplyPacket replyPacket) {
        int requestId = replyPacket.getNextValueAsInt();
        assertAllDataRead(replyPacket);
        return requestId;
    }

    private ParsedEvent waitForEvent(byte eventKind, int requestId) {
        logWriter.println("Waiting for " + JDWPConstants.EventKind.getName(eventKind) +
                " (request " + requestId + ")");

        // Wait for event.
        EventPacket eventPacket = debuggeeWrapper.vmMirror.receiveCertainEvent(eventKind);
        ParsedEvent[] parsedEvents = ParsedEvent.parseEventPacket(eventPacket);

        // Check that this event is the expected one.
        assertEquals(1, parsedEvents.length);
        ParsedEvent parsedEvent = parsedEvents[0];
        assertEventKindEquals("Invalid event kind", eventKind,  parsedEvent.getEventKind());
        assertEquals("Invalid request id", requestId, parsedEvent.getRequestID());
        return parsedEvent;
    }
}
