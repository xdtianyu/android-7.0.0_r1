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

package org.apache.harmony.jpda.tests.jdwp.EventModifiers;

import org.apache.harmony.jpda.tests.framework.Breakpoint;
import org.apache.harmony.jpda.tests.framework.jdwp.Event;
import org.apache.harmony.jpda.tests.framework.jdwp.EventBuilder;
import org.apache.harmony.jpda.tests.framework.jdwp.JDWPConstants;
import org.apache.harmony.jpda.tests.framework.jdwp.Location;
import org.apache.harmony.jpda.tests.framework.jdwp.ParsedEvent.EventThreadLocation;
import org.apache.harmony.jpda.tests.framework.jdwp.ReplyPacket;
import org.apache.harmony.jpda.tests.framework.jdwp.Value;
import org.apache.harmony.jpda.tests.share.JPDADebuggeeSynchronizer;

/**
 * JDWP Unit test for InstanceOnly event modifier.
 */
public class InstanceOnlyModifierTest extends JDWPEventModifierTestCase {

    private static final
            String DEBUGGEE_SIGNATURE = "Lorg/apache/harmony/jpda/tests/jdwp/EventModifiers/InstanceOnlyModifierDebuggee;";
    private static final
            String TEST_CLASS_SIGNATURE = "Lorg/apache/harmony/jpda/tests/jdwp/EventModifiers/InstanceOnlyModifierDebuggee$TestClass;";
    private static final
            String TEST_CLASS_NAME = "org.apache.harmony.jpda.tests.jdwp.EventModifiers.InstanceOnlyModifierDebuggee$TestClass";

    // The name of the test method where we set our event requests.
    private static final String METHOD_NAME = "eventTestMethod";

    // The name of the test method where we set our event requests.
    private static final String WATCHED_FIELD_NAME = "watchedField";

    private static final String INSTANCE_FIELD_NAME = "INSTANCE_ONLY";

    @Override
    protected String getDebuggeeClassName() {
        return InstanceOnlyModifierDebuggee.class.getName();
    }

    /**
     * This testcase is for BREAKPOINT event with InstanceOnly modifier.
     * <BR>It runs InstanceOnlyModifierDebuggee and sets BREAKPOINT to its
     * {@link InstanceOnlyModifierDebuggee.TestClass#eventTestMethod} method.
     * <BR>Then calls this method multiple times and verifies that requested
     * BREAKPOINT event occurs only when 'this' object is the object in field
     * {@link InstanceOnlyModifierDebuggee#INSTANCE_ONLY}.
     * <BR>Note: if the VM does not support the canUseInstanceFilters
     * capability, the test succeeds.
     */
    public void testBreakpoint() {
        logWriter.println("testBreakpoint started");

        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_READY);

        byte typeTag = JDWPConstants.TypeTag.CLASS;
        Breakpoint breakpoint = new Breakpoint(TEST_CLASS_SIGNATURE,
                METHOD_NAME, 0);
        EventBuilder builder = createBreakpointEventBuilder(typeTag,
                breakpoint);
        testEventWithInstanceOnlyModifier(builder);
        logWriter.println("testBreakpoint done");
    }

    /**
     * This testcase is for METHOD_ENTRY event with InstanceOnly modifier.
     * <BR>It runs InstanceOnlyModifierDebuggee and sets METHOD_ENTRY to the
     * {@link InstanceOnlyModifierDebuggee.TestClass} class.
     * <BR>Then calls
     * {@link InstanceOnlyModifierDebuggee.TestClass#eventTestMethod} method
     * multiple times and verifies that requested METHOD_ENTRY event occurs
     * only when 'this' object is the object in field
     * {@link InstanceOnlyModifierDebuggee#INSTANCE_ONLY}.
     * <BR>Note: if the VM does not support the canUseInstanceFilters
     * capability, the test succeeds.
     */
    public void testMethodEntry() {
        logWriter.println("testMethodEntry started");

        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_READY);

        EventBuilder builder = createMethodEntryEventBuilder(TEST_CLASS_NAME);
        testEventWithInstanceOnlyModifier(builder);

        logWriter.println("testMethodEntry done");
    }

    /**
     * This testcase is for METHOD_EXIT event with InstanceOnly modifier.
     * <BR>It runs InstanceOnlyModifierDebuggee and sets METHOD_EXIT to the
     * {@link InstanceOnlyModifierDebuggee.TestClass} class.
     * <BR>Then calls
     * {@link InstanceOnlyModifierDebuggee.TestClass#eventTestMethod} method
     * multiple times and verifies that requested METHOD_EXIT event occurs
     * only when 'this' object is the object in field
     * {@link InstanceOnlyModifierDebuggee#INSTANCE_ONLY}.
     * <BR>Note: if the VM does not support the canUseInstanceFilters
     * capability, the test succeeds.
     */
    public void testMethodExit() {
        logWriter.println("testMethodExit started");

        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_READY);

        EventBuilder builder = createMethodExitEventBuilder(TEST_CLASS_NAME);
        testEventWithInstanceOnlyModifier(builder);

        logWriter.println("testMethodExit done");
    }

    /**
     * This testcase is for METHOD_EXIT_WITH_RETURN_VALUE event with
     * InstanceOnly modifier.
     * <BR>It runs InstanceOnlyModifierDebuggee and sets
     * METHOD_EXIT_WITH_RETURN_VALUE to the
     * {@link InstanceOnlyModifierDebuggee.TestClass} class.
     * <BR>Then calls
     * {@link InstanceOnlyModifierDebuggee.TestClass#eventTestMethod} method
     * multiple times and verifies that requested METHOD_EXIT_WITH_RETURN_VALUE
     * event occurs only when 'this' object is the object in field
     * {@link InstanceOnlyModifierDebuggee#INSTANCE_ONLY}.
     * <BR>Note: if the VM does not support the canUseInstanceFilters
     * capability, the test succeeds.
     */
    public void testMethodExitWithReturnValue() {
        logWriter.println("testMethodExitWithReturnValue started");

        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_READY);

        EventBuilder builder = createMethodExitWithReturnValueEventBuilder(TEST_CLASS_NAME);
        testEventWithInstanceOnlyModifier(builder);

        logWriter.println("testMethodExitWithReturnValue done");
    }

    /**
     * This testcase is for EXCEPTION event with ThreadOnly modifier.
     * <BR>It runs InstanceOnlyModifierDebuggee and sets EXCEPTION to the
     * {@link InstanceOnlyModifierDebuggee.TestException} class but only for
     * caught exceptions.
     * <BR>Then calls
     * {@link InstanceOnlyModifierDebuggee.TestClass#throwException} method
     * multiple times and verifies that requested EXCEPTION event occurs only
     * when 'this' object is the object in field
     * {@link InstanceOnlyModifierDebuggee#INSTANCE_ONLY}.
     */
    public void testException() {
        logWriter.println("testException started");

        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_READY);

        String exceptionClassSignature =
                "Lorg/apache/harmony/jpda/tests/jdwp/EventModifiers/InstanceOnlyModifierDebuggee$TestException;";
        EventBuilder builder = createExceptionEventBuilder(exceptionClassSignature, true, false);
        testEventWithInstanceOnlyModifier(builder);

        logWriter.println("testException done");
    }

    /**
     * This testcase is for FIELD_ACCESS event with InstanceOnly modifier.
     * <BR>It runs InstanceOnlyModifierDebuggee and requests FIELD_ACCESS event
     * for {@link InstanceOnlyModifierDebuggee.TestClass#watchedField}.
     * <BR>Then calls
     * {@link InstanceOnlyModifierDebuggee.TestClass#readAndWriteField} method
     * multiple times and verifies that requested FIELD_ACCESS event occurs
     * only when 'this' object is the object in field
     * {@link InstanceOnlyModifierDebuggee#INSTANCE_ONLY}.
     * <BR>Note: if the VM does not support the canUseInstanceFilters and
     * canWatchFieldAccess capabilities, the test succeeds.
     */
    public void testFieldAccess() {
        logWriter.println("testFieldAccess started");

        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_READY);

        EventBuilder builder = createFieldAccessEventBuilder(
                JDWPConstants.TypeTag.CLASS, TEST_CLASS_SIGNATURE,
                WATCHED_FIELD_NAME);
        testEventWithInstanceOnlyModifier(builder);

        logWriter.println("testFieldAccess done");
    }

    /**
     * This testcase is for FIELD_MODIFICATION event with InstanceOnly modifier.
     * <BR>It runs InstanceOnlyModifierDebuggee and requests FIELD_MODIFICATION
     * event for {@link InstanceOnlyModifierDebuggee.TestClass#watchedField}.
     * <BR>Then calls
     * {@link InstanceOnlyModifierDebuggee.TestClass#readAndWriteField} method
     * multiple times and verifies that requested FIELD_MODIFICATION event
     * occurs only when 'this' object is the object in field
     * {@link InstanceOnlyModifierDebuggee#INSTANCE_ONLY}.
     * <BR>Note: if the VM does not support the canUseInstanceFilters and
     * canWatchFieldModification capabilities, the test succeeds.
     */
    public void testFieldModification() {
        logWriter.println("testFieldModification started");

        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_READY);

        EventBuilder builder = createFieldModificationEventBuilder(
                JDWPConstants.TypeTag.CLASS, TEST_CLASS_SIGNATURE,
                WATCHED_FIELD_NAME);
        testEventWithInstanceOnlyModifier(builder);

        logWriter.println("testFieldModification done");
    }

    private long getInstanceObjectId() {
        Value fieldValue = getFieldValue(DEBUGGEE_SIGNATURE,
                INSTANCE_FIELD_NAME);
        assertEquals("Invalid field value tag", JDWPConstants.Tag.OBJECT_TAG,
                fieldValue.getTag());
        return fieldValue.getLongValue();
    }

    private void testEventWithInstanceOnlyModifier(EventBuilder builder) {
        long objectID = getInstanceObjectId();
        builder.setInstanceOnly(objectID);
        Event event = builder.build();
        int requestID = requestEvent(event);

        EventThreadLocation eventThread =
                (EventThreadLocation) waitForEvent(event.eventKind, requestID);

        checkThisObject(eventThread, objectID);

        clearAndResume(event.eventKind, requestID);
    }

    private void checkThisObject(EventThreadLocation eventThread, long objectID) {
        long threadID = eventThread.getThreadID();
        assertTrue(threadID != 0);

        Location location = eventThread.getLocation();

        logWriter.println("Search the frame ID of the event location in thread " +  threadID);
        long frameID = -1;
        int framesCount = debuggeeWrapper.vmMirror.getFrameCount(threadID);
        ReplyPacket reply = debuggeeWrapper.vmMirror.getThreadFrames(threadID,
                0, framesCount);
        checkReplyPacket(reply, "Failed to get frames for thread " + threadID);
        int frames = reply.getNextValueAsInt();
        for (int i = 0; i < frames; ++i) {
            long currentFrameID = reply.getNextValueAsLong();
            Location currentFrameLocation = reply.getNextValueAsLocation();
            if (currentFrameLocation.equals(location)) {
                frameID = currentFrameID;
                break;
            }
        }
        assertTrue("Failed to find frame for event location", frameID != -1);

        logWriter.println("Check this object of frame " + frameID);
        long thisObjectID = debuggeeWrapper.vmMirror.getThisObject(threadID,
                frameID);
        assertEquals("Event is not related to the object we're looking for",
                objectID, thisObjectID);
    }
}
