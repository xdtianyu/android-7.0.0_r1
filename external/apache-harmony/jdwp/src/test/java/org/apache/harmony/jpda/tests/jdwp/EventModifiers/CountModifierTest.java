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
import org.apache.harmony.jpda.tests.framework.jdwp.Value;
import org.apache.harmony.jpda.tests.share.JPDADebuggeeSynchronizer;

/**
 * JDWP Unit test for Count event modifier.
 */
public class CountModifierTest extends JDWPEventModifierTestCase {
    private static final
            String DEBUGGEE_SIGNATURE = "Lorg/apache/harmony/jpda/tests/jdwp/EventModifiers/CountModifierDebuggee;";
    private static final
            String TEST_CLASS_SIGNATURE = "Lorg/apache/harmony/jpda/tests/jdwp/EventModifiers/CountModifierDebuggee$TestClass;";
    private static final
            String TEST_CLASS_NAME = "org.apache.harmony.jpda.tests.jdwp.EventModifiers.CountModifierDebuggee$TestClass";
    private static final
            String EXCEPTION_SIGNATURE = "Lorg/apache/harmony/jpda/tests/jdwp/EventModifiers/CountModifierDebuggee$TestException;";

    // The name of the test method where we set our event requests.
    private static final String METHOD_NAME = "eventTestMethod";

    // The name of the test method where we set our event requests.
    private static final String WATCHED_FIELD_NAME = "watchedField";

    // Fields for verifying events count.
    private static final String
            LOCATION_COUNT_FIELD_NAME = "locationEventCount";
    private static final String
            EXCEPTION_EVENT_COUNT_FIELD_NAME = "exceptionEventCount";
    private static final String THREAD_RUN_COUNT_FIELD_NAME = "threadRunCount";
    private static final String
            FIELD_READ_WRITE_COUNT_FIELD_NAME = "fieldReadWriteCount";

    @Override
    protected String getDebuggeeClassName() {
        return CountModifierDebuggee.class.getName();
    }

    /**
     * This testcase is for BREAKPOINT event with Count modifier.
     * <BR>It runs CountModifierDebuggee and sets BREAKPOINT to its
     * {@link CountModifierDebuggee.TestClass#eventTestMethod()} method.
     * <BR>Then calls this method multiple times and verifies that requested
     * BREAKPOINT event occurs once after having called the method (count - 1)
     * times. We check this by looking at the value in the field
     * {@link CountModifierDebuggee#locationEventCount}.
     */
    public void testBreakpoint() {
        logWriter.println("testBreakpoint started");

        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_READY);

        // Breakpoint at start of test method.
        byte typeTag = JDWPConstants.TypeTag.CLASS;
        Breakpoint breakpoint = new Breakpoint(TEST_CLASS_SIGNATURE,
                METHOD_NAME, 0);
        EventBuilder builder = createBreakpointEventBuilder(typeTag,
                breakpoint);
        testEventWithCountModifier(builder, LOCATION_COUNT_FIELD_NAME);

        logWriter.println("testBreakpoint done");
    }

    /**
     * This testcase is for METHOD_ENTRY event with Count modifier.
     * <BR>It runs CountModifierDebuggee and sets METHOD_ENTRY to the
     * {@link CountModifierDebuggee.TestClass} class.
     * <BR>Then calls {@link CountModifierDebuggee.TestClass#eventTestMethod()}
     * method multiple times and verifies that requested METHOD_ENTRY event
     * occurs once after having called the method (count - 1) times. We check
     * this by looking at the value in the field
     * {@link CountModifierDebuggee#locationEventCount}.
     */
    public void testMethodEntry() {
        logWriter.println("testMethodEntry started");

        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_READY);
        EventBuilder builder = createMethodEntryEventBuilder(TEST_CLASS_NAME);
        testEventWithCountModifier(builder, LOCATION_COUNT_FIELD_NAME);

        logWriter.println("testMethodEntry done");
    }

    /**
     * This testcase is for METHOD_EXIT event with Count modifier.
     * <BR>It runs CountModifierDebuggee and sets METHOD_EXIT to the
     * {@link CountModifierDebuggee.TestClass} class.
     * <BR>Then calls {@link CountModifierDebuggee.TestClass#eventTestMethod()}
     * method multiple times and verifies that requested METHOD_EXIT event
     * occurs once after having called the method (count - 1) times. We check
     * this by looking at the value in the field
     * {@link CountModifierDebuggee#locationEventCount}.
     */
    public void testMethodExit() {
        logWriter.println("testMethodExit started");

        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_READY);
        EventBuilder builder = createMethodExitEventBuilder(TEST_CLASS_NAME);
        testEventWithCountModifier(builder, LOCATION_COUNT_FIELD_NAME);

        logWriter.println("testMethodExit done");
    }


    /**
     * This testcase is for METHOD_EXIT_WITH_RETURN_VALUE event with Count
     * modifier.
     * <BR>It runs CountModifierDebuggee and sets METHOD_EXIT_WITH_RETURN_VALUE
     * to the {@link CountModifierDebuggee.TestClass} class.
     * <BR>Then calls {@link CountModifierDebuggee.TestClass#eventTestMethod()}
     * method multiple times and verifies that requested
     * METHOD_EXIT_WITH_RETURN_VALUE event occurs once after having called the
     * method (count - 1) times. We check this by looking at the value in the
     * field {@link CountModifierDebuggee#locationEventCount}.
     */
    public void testMethodExitWithReturnValue() {
        logWriter.println("testMethodExitWithReturnValue started");

        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_READY);
        EventBuilder builder = createMethodExitWithReturnValueEventBuilder(TEST_CLASS_NAME);
        testEventWithCountModifier(builder, LOCATION_COUNT_FIELD_NAME);

        logWriter.println("testMethodExitWithReturnValue done");
    }

    /**
     * This testcase is for EXCEPTION event with Count modifier.
     * <BR>It runs CountModifierDebuggee and sets EXCEPTION to the
     * {@link CountModifierDebuggee.TestException} class but only for caught
     * exceptions.
     * <BR>Then calls {@link CountModifierDebuggee.TestClass#throwException}
     * method multiple times and verifies that requested EXCEPTION event
     * occurs once after having called the method (count - 1) times. We check
     * this by looking at the value in the field
     * {@link CountModifierDebuggee#exceptionEventCount}.
     */
    public void testException() {
        logWriter.println("testException started");

        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_READY);
        EventBuilder builder = createExceptionEventBuilder(EXCEPTION_SIGNATURE,
                true, false);
        testEventWithCountModifier(builder,
                EXCEPTION_EVENT_COUNT_FIELD_NAME);

        logWriter.println("testException done");
    }

    /**
     * This testcase is for FIELD_ACCESS event with Count modifier.
     * <BR>It runs CountModifierDebuggee and requests FIELD_ACCESS event for
     * {@link CountModifierDebuggee#watchedField}.
     * <BR>Then calls {@link CountModifierDebuggee#readAndWriteField()}
     * method multiple times and verifies that requested FIELD_ACCESS event
     * occurs once after having called the method (count - 1) times. We check
     * this by looking at the value in the field
     * {@link CountModifierDebuggee#fieldReadWriteCount}.
     * <BR>Note: if the VM does not support the canWatchFieldAccess capability,
     * the test succeeds.
     */
    public void testFieldAccess() {
        logWriter.println("testFieldAccess started");

        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_READY);

        EventBuilder builder = createFieldAccessEventBuilder(
                JDWPConstants.TypeTag.CLASS, DEBUGGEE_SIGNATURE,
                WATCHED_FIELD_NAME);
        testEventWithCountModifier(builder, FIELD_READ_WRITE_COUNT_FIELD_NAME);

        logWriter.println("testFieldAccess done");
    }

    /**
     * This testcase is for FIELD_MODIFICATION event with Count modifier.
     * <BR>It runs CountModifierDebuggee and requests FIELD_MODIFICATION event
     * for {@link CountModifierDebuggee#watchedField}.
     * <BR>Then calls {@link CountModifierDebuggee#readAndWriteField()}
     * method multiple times and verifies that requested FIELD_MODIFICATION
     * event occurs once after having called the method (count - 1) times. We
     * check this by looking at the value in the field
     * {@link CountModifierDebuggee#fieldReadWriteCount}.
     * <BR>Note: if the VM does not support the canWatchFieldModification
     * capability, the test succeeds.
     */
    public void testFieldModification() {
        logWriter.println("testFieldModification started");

        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_READY);

        EventBuilder builder = createFieldModificationEventBuilder(
                JDWPConstants.TypeTag.CLASS, DEBUGGEE_SIGNATURE,
                WATCHED_FIELD_NAME);
        testEventWithCountModifier(builder, FIELD_READ_WRITE_COUNT_FIELD_NAME);

        logWriter.println("testFieldModification done");
    }

    private void testEventWithCountModifier(EventBuilder builder,
            String countFieldName) {
        // Add count modifier and build the event.
        builder.setCount(CountModifierDebuggee.EVENT_COUNT);
        Event event = builder.build();
        int requestID = requestEvent(event);

        waitForEvent(event.eventKind, requestID);

        // Check we properly ignore the (count - 1) previous events.
        int expectedCount = CountModifierDebuggee.EVENT_COUNT;
        int actualCount = getStaticIntField(DEBUGGEE_SIGNATURE, countFieldName);
        assertEquals("Invalid event count", expectedCount, actualCount);

        clearAndResume(event.eventKind, requestID);
    }

    private int getStaticIntField(String classSignature, String fieldName) {
        Value fieldValue = getFieldValue(classSignature, fieldName);
        assertEquals("Invalid field value tag", JDWPConstants.Tag.INT_TAG,
                fieldValue.getTag());
        return fieldValue.getIntValue();
    }

}
