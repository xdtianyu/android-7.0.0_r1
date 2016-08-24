package org.apache.harmony.jpda.tests.jdwp.EventModifiers;

import org.apache.harmony.jpda.tests.framework.Breakpoint;
import org.apache.harmony.jpda.tests.framework.jdwp.Event;
import org.apache.harmony.jpda.tests.framework.jdwp.EventBuilder;
import org.apache.harmony.jpda.tests.framework.jdwp.EventPacket;
import org.apache.harmony.jpda.tests.framework.jdwp.JDWPConstants;
import org.apache.harmony.jpda.tests.framework.jdwp.Location;
import org.apache.harmony.jpda.tests.framework.jdwp.ParsedEvent;
import org.apache.harmony.jpda.tests.framework.jdwp.ParsedEvent.EventThread;
import org.apache.harmony.jpda.tests.framework.jdwp.ReplyPacket;
import org.apache.harmony.jpda.tests.framework.jdwp.Value;
import org.apache.harmony.jpda.tests.jdwp.share.JDWPSyncTestCase;
import org.apache.harmony.jpda.tests.share.JPDADebuggeeSynchronizer;

/**
 * This base class provides utilities for all event modifier tests.
 */
abstract class JDWPEventModifierTestCase extends JDWPSyncTestCase {
    /**
     * The suspend policy used for all events in the tests.
     */
    protected static final byte
            TEST_SUSPEND_POLICY = JDWPConstants.SuspendPolicy.ALL;

    /**
     * Returns value of the requested field.
     *
     * @param classSignature the signature of the field's declaring class
     * @param fieldName the field name.
     * @return the value of the field
     */
    protected Value getFieldValue(String classSignature, String fieldName) {
        long classID = debuggeeWrapper.vmMirror.getClassID(classSignature);
        assertTrue("Failed to find debuggee class " + classSignature,
                classID != 0);
        long fieldID = debuggeeWrapper.vmMirror.getFieldID(classID, fieldName);
        assertTrue("Failed to find field " + classSignature + "." + fieldName,
                fieldID != 0);

        long[] fieldIDs = new long[] { fieldID };
        Value[] fieldValues = debuggeeWrapper.vmMirror.getReferenceTypeValues(
                classID, fieldIDs);
        assertNotNull("Failed to get field values for class " + classSignature,
                fieldValues);
        assertEquals("Invalid number of field values", 1, fieldValues.length);
        return fieldValues[0];
    }

    /**
     * Creates an {@link EventBuilder} for BREAKPOINT event and sets a
     * LocationOnly modifier.
     *
     * @param typeTag the type tag of the location's class
     * @param breakpoint the breakpoint info
     * @return a new {@link EventBuilder}
     */
    protected EventBuilder createBreakpointEventBuilder(byte typeTag,
            Breakpoint breakpoint) {
        long typeID = debuggeeWrapper.vmMirror.getTypeID(breakpoint.className, typeTag);
        long methodID = getMethodID(typeID, breakpoint.methodName);
        byte eventKind = JDWPConstants.EventKind.BREAKPOINT;
        EventBuilder builder = new EventBuilder(eventKind, TEST_SUSPEND_POLICY);
        builder.setLocationOnly(new Location(typeTag, typeID, methodID,
                breakpoint.index));
        return builder;
    }

    /**
     * Creates an {@link EventBuilder} for EXCEPTION event and sets an
     * ExceptionOnly modifier.
     *
     * @param exceptionClassSignature the signature of the exception class
     * @param caught whether the exception must be caught
     * @param uncaught whether the exception must be uncaught
     * @return a new {@link EventBuilder}
     */
    protected EventBuilder createExceptionEventBuilder(
            String exceptionClassSignature, boolean caught, boolean uncaught) {
        byte eventKind = JDWPConstants.EventKind.EXCEPTION;
        EventBuilder builder = new EventBuilder(eventKind, TEST_SUSPEND_POLICY);
        long exceptionClassID = debuggeeWrapper.vmMirror.getClassID(
                exceptionClassSignature);
        assertTrue("Failed to find type ID " + exceptionClassSignature,
                exceptionClassID != 1);
        builder.setExceptionOnly(exceptionClassID, caught, uncaught);
        return builder;
    }

    /**
     * Creates an {@link EventBuilder} for METHOD_ENTRY event and sets a
     * ClassMatch modifier.
     *
     * @param className a regular expression of class names matching the method
     * entry events.
     * @return a new {@link EventBuilder}
     */
    protected EventBuilder createMethodEntryEventBuilder(String className) {
        EventBuilder builder = new EventBuilder(
                JDWPConstants.EventKind.METHOD_ENTRY, TEST_SUSPEND_POLICY);
        return builder.setClassMatch(className);
    }

    /**
     * Creates an {@link EventBuilder} for METHOD_EXIT event and sets a
     * ClassMatch modifier.
     *
     * @param className a regular expression of class names matching the method
     * exit events.
     * @return a new {@link EventBuilder}
     */
    protected EventBuilder createMethodExitEventBuilder(String className) {
        EventBuilder builder = new EventBuilder(
                JDWPConstants.EventKind.METHOD_EXIT, TEST_SUSPEND_POLICY);
        return builder.setClassMatch(className);
    }

    /**
     * Creates an {@link EventBuilder} for METHOD_EXIT_WITH_RETURN_VALUE event
     * and sets a ClassMatch modifier.
     *
     * @param className a regular expression of class names matching the method
     * exit events.
     * @return a new {@link EventBuilder}
     */
    protected EventBuilder createMethodExitWithReturnValueEventBuilder(String className) {
        EventBuilder builder = new EventBuilder(
                JDWPConstants.EventKind.METHOD_EXIT_WITH_RETURN_VALUE,
                TEST_SUSPEND_POLICY);
        return builder.setClassMatch(className);
    }

    /**
     * Creates an {@link EventBuilder} for THREAD_START event.
     *
     * @return a new {@link EventBuilder}
     */
    protected EventBuilder createThreadStartBuilder() {
        return new EventBuilder(JDWPConstants.EventKind.THREAD_START,
                TEST_SUSPEND_POLICY);
    }

    /**
     * Creates an {@link EventBuilder} for THREAD_END event.
     *
     * @return a new {@link EventBuilder}
     */
    protected EventBuilder createThreadEndBuilder() {
        return new EventBuilder(JDWPConstants.EventKind.THREAD_END,
                TEST_SUSPEND_POLICY);
    }

    /**
     * Creates an {@link EventBuilder} for FIELD_ACCESS event and sets a
     * FieldOnly modifier.
     *
     * @param typeTag the type tag of the field's declaring class
     * @param classSignature the field's declaring class signature
     * @param fieldName the field name
     * @return a new {@link EventBuilder}
     */
    protected EventBuilder createFieldAccessEventBuilder(byte typeTag,
            String classSignature, String fieldName) {
        return createFieldEventBuilder(typeTag, classSignature, fieldName, false);
    }

    /**
     * Creates an {@link EventBuilder} for FIELD_MODIFICATION event and sets a
     * FieldOnly modifier.
     *
     * @param typeTag the type tag of the field's declaring class
     * @param classSignature the field's declaring class signature
     * @param fieldName the field name
     * @return a new {@link EventBuilder}
     */
    protected EventBuilder createFieldModificationEventBuilder(byte typeTag,
            String classSignature, String fieldName) {
        return createFieldEventBuilder(typeTag, classSignature, fieldName, true);
    }

    private EventBuilder createFieldEventBuilder(byte typeTag,
                                                String typeSignature,
                                                String fieldName,
                                                boolean modification) {
        byte eventKind;
        if (modification) {
            eventKind = JDWPConstants.EventKind.FIELD_MODIFICATION;
        } else {
            eventKind = JDWPConstants.EventKind.FIELD_ACCESS;
        }
        EventBuilder builder = new EventBuilder(eventKind, TEST_SUSPEND_POLICY);
        long typeID = debuggeeWrapper.vmMirror.getTypeID(typeSignature, typeTag);
        assertTrue("Failed to find type ID " + typeSignature, typeID != 1);
        long fieldID = debuggeeWrapper.vmMirror.getFieldID(typeID, fieldName);
        assertTrue("Failed to find field ID " + typeSignature + "." + fieldName,
                fieldID != 1);
        builder.setFieldOnly(typeID, fieldID);
        return builder;
    }

    /**
     * Sends a request for the given event.
     *
     * @param event the event to request
     * @return the request ID
     */
    protected int requestEvent(Event event) {
        String eventName = JDWPConstants.EventKind.getName(event.eventKind);
        logWriter.println("Requesting " + eventName);
        ReplyPacket reply = debuggeeWrapper.vmMirror.setEvent(event);
        checkReplyPacket(reply, "Failed to request " + eventName);
        int requestID = reply.getNextValueAsInt();
        assertAllDataRead(reply);
        return requestID;
    }

    /**
     * Waits for the first corresponding event.
     *
     * @param eventKind the event kind
     * @param requestID the event request ID
     * @return the event
     */
    protected EventThread waitForEvent(byte eventKind, int requestID) {
        logWriter.println("Signaling debuggee to continue");
        synchronizer.sendMessage(JPDADebuggeeSynchronizer.SGNL_CONTINUE);

        String eventName = JDWPConstants.EventKind.getName(eventKind);
        logWriter.println(
                "Waiting for " + eventName + " with requestID " + requestID + " ...");
        EventPacket eventPacket = debuggeeWrapper.vmMirror.receiveCertainEvent(
                eventKind);
        ParsedEvent[] parsedEvents = ParsedEvent.parseEventPacket(eventPacket);
        assertNotNull(parsedEvents);
        assertTrue(parsedEvents.length > 0);
        ParsedEvent event = parsedEvents[0];
        assertEquals(eventKind, event.getEventKind());
        assertEquals(requestID, event.getRequestID());
        logWriter.println("Received " + eventName + " event");
        return (EventThread) event;
    }

    /**
     * Clears the corresponding event and resumes the VM.
     *
     * @param eventKind the event kind
     * @param requestID the event request ID
     */
    protected void clearAndResume(byte eventKind, int requestID) {
        clearEvent(eventKind, requestID, true);
        resumeDebuggee();
    }

    /**
     * Warns about an unsupported capability by printing a message in the
     * console.
     *
     * @param capabilityName the capability name
     */
    protected void logCapabilityWarning(String capabilityName) {
        // Build the message to prompt.
        StringBuilder messageBuilder =
                new StringBuilder("# WARNING: this VM doesn't possess capability: ");
        messageBuilder.append(capabilityName);
        messageBuilder.append(' ');
        messageBuilder.append('#');
        String message = messageBuilder.toString();

        // Build a sharp string long enough.
        int sharpLineLength = message.length();
        StringBuilder sharpLineBuilder = new StringBuilder(sharpLineLength);
        for (int i = 0; i < sharpLineLength; ++i) {
            sharpLineBuilder.append('#');
        }
        String sharpLine = sharpLineBuilder.toString();

        // Print warning message.
        logWriter.println(sharpLine);
        logWriter.println(message);
        logWriter.println(sharpLine);
    }
}
