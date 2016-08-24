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

import org.apache.harmony.jpda.tests.framework.jdwp.EventBuilder;
import org.apache.harmony.jpda.tests.framework.jdwp.JDWPConstants;
import org.apache.harmony.jpda.tests.framework.jdwp.ParsedEvent;
import org.apache.harmony.jpda.tests.framework.jdwp.TaggedObject;
import org.apache.harmony.jpda.tests.framework.jdwp.VmMirror;

/**
 *
 * JDWP Unit test for FIELD_ACCESS and FIELD_MODIFICATION events with
 * LocationOnly modifier.
 */
public class FieldWithLocationTest extends EventLocationEventTestCase {

    private static final String DEBUGGEE_SIGNATURE =
            "Lorg/apache/harmony/jpda/tests/jdwp/Events/FieldWithLocationDebuggee;";
    private static final String FIELD_NAME = "testIntField";

    // Cache debuggee class ID.
    private long debuggeeClassId = -1;

    // Cache field ID.
    private long fieldId = -1;

    /**
     * This testcase is for FIELD_ACCESS event.
     * <BR>It runs FieldDebuggee that accesses to the value of its internal field
     * and verify that requested FIELD_ACCESS event occurs in the
     * expected method.
     */
    public void testFieldAccessLocationEvent() {
        logWriter.println("testFieldAccessLocationEvent started");

        runFieldLocationTest(false);

        logWriter.println("testFieldAccessLocationEvent done");
    }

    /**
     * This testcase is for FIELD_MODIFICATION event.
     * <BR>It runs FieldDebuggee that modifies the value of its internal field
     * and verify that requested FIELD_MODIFICATION event occurs in the
     * expected method.
     */
    public void testFieldModificationLocationEvent() {
        logWriter.println("testFieldModificationLocationEvent started");

        runFieldLocationTest(true);

        logWriter.println("testFieldModificationLocationEvent done");
    }

    @Override
    protected final String getDebuggeeClassName() {
        return FieldWithLocationDebuggee.class.getName();
    }

    @Override
    protected final String getDebuggeeSignature() {
        return DEBUGGEE_SIGNATURE;
    }

    @Override
    protected final String getExpectedLocationMethodName() {
        return "expectedMethodForFieldEvent";
    }

    @Override
    protected final void createEventBuilder(EventBuilder builder) {
        if (debuggeeClassId == -1) {
            debuggeeClassId = getClassIDBySignature(DEBUGGEE_SIGNATURE);
        }
        if (fieldId == -1) {
            fieldId = debuggeeWrapper.vmMirror.getFieldID(debuggeeClassId, FIELD_NAME);
        }
        builder.setFieldOnly(debuggeeClassId, fieldId);
    }

    @Override
    protected void checkEvent(ParsedEvent event) {
        TaggedObject accessedField = null;
        byte fieldEventKind = event.getEventKind();
        if (fieldEventKind  == JDWPConstants.EventKind.FIELD_ACCESS) {
            accessedField = ((ParsedEvent.Event_FIELD_ACCESS)event).getObject();
        } else if (fieldEventKind == JDWPConstants.EventKind.FIELD_MODIFICATION) {
            accessedField = ((ParsedEvent.Event_FIELD_MODIFICATION)event).getObject();
        }

        // Check the field receiver is an instance of our debuggee class.
        long typeID = getObjectReferenceType(accessedField.objectID);
        String returnedExceptionSignature = getClassSignature(typeID);
        assertString("Invalid class signature,",
                DEBUGGEE_SIGNATURE, returnedExceptionSignature);
    }

    private static String getFieldCapabilityName(boolean modification) {
        return modification ? "canWatchFieldModification" :
            "canWatchFieldAccess";
    }

    private static byte getFieldEventKind(boolean modification) {
        return modification ? JDWPConstants.EventKind.FIELD_MODIFICATION :
            JDWPConstants.EventKind.FIELD_ACCESS;
    }

    private void runFieldLocationTest(boolean modification) {
        final byte eventKind = getFieldEventKind(modification);
        final String capabilityname = getFieldCapabilityName(modification);

        logWriter.println("Check capability " + capabilityname);
        runEventWithLocationTest(eventKind);
    }
}
