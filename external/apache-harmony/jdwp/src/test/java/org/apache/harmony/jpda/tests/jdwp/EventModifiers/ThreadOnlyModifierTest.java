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
import org.apache.harmony.jpda.tests.framework.jdwp.ParsedEvent.EventThread;
import org.apache.harmony.jpda.tests.framework.jdwp.Value;
import org.apache.harmony.jpda.tests.share.JPDADebuggeeSynchronizer;

/**
 * JDWP Unit test for ThreadOnly event modifier.
 */
public class ThreadOnlyModifierTest extends JDWPEventModifierTestCase {

    private static final
            String DEBUGGEE_SIGNATURE = "Lorg/apache/harmony/jpda/tests/jdwp/EventModifiers/ThreadOnlyModifierDebuggee;";
    private static final
            String TEST_CLASS_SIGNATURE = "Lorg/apache/harmony/jpda/tests/jdwp/EventModifiers/ThreadOnlyModifierDebuggee$TestClass;";
    private static final
            String TEST_CLASS_NAME = "org.apache.harmony.jpda.tests.jdwp.EventModifiers.ThreadOnlyModifierDebuggee$TestClass";

    // The name of the test method where we set our event requests.
    private static final String METHOD_NAME = "eventTestMethod";

    // The name of the test method where we set our event requests.
    private static final String WATCHED_FIELD_NAME = "watchedField";

    private static final String THREAD_FIELD_NAME = "THREAD_ONLY";

    @Override
    protected String getDebuggeeClassName() {
        return ThreadOnlyModifierDebuggee.class.getName();
    }

    /**
     * This testcase is for BREAKPOINT event with ThreadOnly modifier.
     * <BR>It runs ThreadOnlyModifierDebuggee and sets BREAKPOINT to its
     * {@link ThreadOnlyModifierDebuggee.TestClass#eventTestMethod} method.
     * <BR>Then calls this method multiple times and verifies that requested
     * BREAKPOINT event occurs only in the
     * {@link ThreadOnlyModifierDebuggee#THREAD_ONLY} thread.
     */
    public void testBreakpoint() {
        logWriter.println("testBreakpoint started");

        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_READY);

        byte typeTag = JDWPConstants.TypeTag.CLASS;
        Breakpoint breakpoint = new Breakpoint(TEST_CLASS_SIGNATURE,
                METHOD_NAME, 0);
        EventBuilder builder = createBreakpointEventBuilder(typeTag,
                breakpoint);
        testEventWithThreadOnlyModifier(builder);

        logWriter.println("testBreakpoint done");
    }

    /**
     * This testcase is for METHOD_ENTRY event with ThreadOnly modifier.
     * <BR>It runs ThreadOnlyModifierDebuggee and sets METHOD_ENTRY to the
     * {@link ThreadOnlyModifierDebuggee.TestClass} class.
     * <BR>Then calls
     * {@link ThreadOnlyModifierDebuggee.TestClass#eventTestMethod} method
     * multiple times and verifies that requested METHOD_ENTRY event occurs
     * only in the {@link ThreadOnlyModifierDebuggee#THREAD_ONLY} thread.
     */
    public void testMethodEntry() {
        logWriter.println("testMethodEntry started");

        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_READY);
        EventBuilder builder = createMethodEntryEventBuilder(TEST_CLASS_NAME);
        testEventWithThreadOnlyModifier(builder);

        logWriter.println("testMethodEntry done");
    }

    /**
     * This testcase is for METHOD_EXIT event with ThreadOnly modifier.
     * <BR>It runs ThreadOnlyModifierDebuggee and sets METHOD_EXIT to the
     * {@link ThreadOnlyModifierDebuggee.TestClass} class.
     * <BR>Then calls
     * {@link ThreadOnlyModifierDebuggee.TestClass#eventTestMethod} method
     * multiple times and verifies that requested METHOD_EXIT event occurs only
     * in the {@link ThreadOnlyModifierDebuggee#THREAD_ONLY} thread.
     */
    public void testMethodExit() {
        logWriter.println("testMethodExit started");

        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_READY);
        EventBuilder builder = createMethodExitEventBuilder(TEST_CLASS_NAME);
        testEventWithThreadOnlyModifier(builder);

        logWriter.println("testMethodExit done");
    }

    /**
     * This testcase is for METHOD_EXIT_WITH_RETURN_VALUE event with ThreadOnly
     * modifier.
     * <BR>It runs ThreadOnlyModifierDebuggee and sets
     * METHOD_EXIT_WITH_RETURN_VALUE to the
     * {@link ThreadOnlyModifierDebuggee.TestClass} class.
     * <BR>Then calls
     * {@link ThreadOnlyModifierDebuggee.TestClass#eventTestMethod} method
     * multiple times and verifies that requested METHOD_EXIT_WITH_RETURN_VALUE
     * event occurs only in the {@link ThreadOnlyModifierDebuggee#THREAD_ONLY}
     * thread.
     */
    public void testMethodExitWithReturnValue() {
        logWriter.println("testMethodExitWithReturnValue started");

        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_READY);
        EventBuilder builder = createMethodExitWithReturnValueEventBuilder(TEST_CLASS_NAME);
        testEventWithThreadOnlyModifier(builder);

        logWriter.println("testMethodExitWithReturnValue done");
    }

    /**
     * This testcase is for EXCEPTION event with ThreadOnly modifier.
     * <BR>It runs ThreadOnlyModifierDebuggee and sets EXCEPTION to the
     * {@link ThreadOnlyModifierDebuggee.TestException} class but only for
     * caught exceptions.
     * <BR>Then calls
     * {@link ThreadOnlyModifierDebuggee.TestThread#throwException} method
     * multiple times and verifies that requested EXCEPTION event occurs only
     * in the {@link ThreadOnlyModifierDebuggee#THREAD_ONLY} thread.
     */
    public void testException() {
        logWriter.println("testException started");

        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_READY);
        EventBuilder builder = createExceptionEventBuilder(
                "Lorg/apache/harmony/jpda/tests/jdwp/EventModifiers/ThreadOnlyModifierDebuggee$TestException;",
                true, false);
        testEventWithThreadOnlyModifier(builder);

        logWriter.println("testException done");
    }

    /**
     * This testcase is for THREAD_START event with ThreadOnly modifier.
     * <BR>It runs ThreadOnlyModifierDebuggee and requests THREAD_START event.
     * <BR>Then calls {@link ThreadOnlyModifierDebuggee#runThread} method
     * multiple times and verifies that requested THREAD_START event occurs
     * only in the {@link ThreadOnlyModifierDebuggee#THREAD_ONLY} thread.
     */
    public void testThreadStart() {
        logWriter.println("testThreadStart started");

        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_READY);

        EventBuilder builder = createThreadStartBuilder();
        testEventWithThreadOnlyModifier(builder);

        logWriter.println("testThreadStart done");
    }

    /**
     * This testcase is for THREAD_END event with ThreadOnly modifier.
     * <BR>It runs ThreadOnlyModifierDebuggee and requests THREAD_END event.
     * <BR>Then calls {@link ThreadOnlyModifierDebuggee#runThread} method
     * multiple times and verifies that requested THREAD_END event occurs only
     * in the {@link ThreadOnlyModifierDebuggee#THREAD_ONLY} thread.
     */
    public void testThreadEnd() {
        logWriter.println("testThreadEnd started");

        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_READY);

        EventBuilder builder = createThreadEndBuilder();
        testEventWithThreadOnlyModifier(builder);

        logWriter.println("testThreadEnd done");
    }

    /**
     * This testcase is for FIELD_ACCESS event with ThreadOnly modifier.
     * <BR>It runs ThreadOnlyModifierDebuggee and requests FIELD_ACCESS event
     * for {@link ThreadOnlyModifierDebuggee#watchedField}.
     * <BR>Then calls
     * {@link ThreadOnlyModifierDebuggee.TestThread#readAndWriteField} method
     * multiple times and verifies that requested FIELD_ACCESS event occurs
     * only in the {@link ThreadOnlyModifierDebuggee#THREAD_ONLY} thread.
     * <BR>Note: if the VM does not support the canWatchFieldAccess capability,
     * the test succeeds.
     */
    public void testFieldAccess() {
        logWriter.println("testFieldAccess started");

        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_READY);

        EventBuilder builder = createFieldAccessEventBuilder(
                JDWPConstants.TypeTag.CLASS, DEBUGGEE_SIGNATURE,
                WATCHED_FIELD_NAME);
        testEventWithThreadOnlyModifier(builder);

        logWriter.println("testFieldAccess done");
    }

    /**
     * This testcase is for FIELD_MODIFICATION event with ThreadOnly modifier.
     * <BR>It runs ThreadOnlyModifierDebuggee and requests FIELD_MODIFICATION
     * event for {@link ThreadOnlyModifierDebuggee#watchedField}.
     * <BR>Then calls
     * {@link ThreadOnlyModifierDebuggee.TestThread#readAndWriteField} method
     * multiple times and verifies that requested FIELD_MODIFICATION event
     * occurs only in the {@link ThreadOnlyModifierDebuggee#THREAD_ONLY} thread.
     * <BR>Note: if the VM does not support the canWatchFieldModification
     * capability, the test succeeds.
     */
    public void testFieldModification() {
        logWriter.println("testFieldModification started");

        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_READY);

        EventBuilder builder = createFieldModificationEventBuilder(
                JDWPConstants.TypeTag.CLASS, DEBUGGEE_SIGNATURE,
                WATCHED_FIELD_NAME);
        testEventWithThreadOnlyModifier(builder);

        logWriter.println("testFieldModification done");
    }

    private long getFilteredThreadId() {
        Value fieldValue = getFieldValue(DEBUGGEE_SIGNATURE, THREAD_FIELD_NAME);
        assertEquals("Invalid field value tag", JDWPConstants.Tag.THREAD_TAG,
                fieldValue.getTag());
        return fieldValue.getLongValue();
    }

    private void testEventWithThreadOnlyModifier(EventBuilder builder) {
        long threadID = getFilteredThreadId();
        Event event = builder.setThreadOnly(threadID).build();
        int requestID = requestEvent(event);

        EventThread eventThread = waitForEvent(event.eventKind, requestID);
        assertEquals(threadID, eventThread.getThreadID());

        clearAndResume(event.eventKind, requestID);
    }
}
