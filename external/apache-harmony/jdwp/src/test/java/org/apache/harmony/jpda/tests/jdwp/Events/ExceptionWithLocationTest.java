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
import org.apache.harmony.jpda.tests.framework.jdwp.Location;
import org.apache.harmony.jpda.tests.framework.jdwp.ParsedEvent;
import org.apache.harmony.jpda.tests.framework.jdwp.TaggedObject;

/**
 * JDWP Unit test for caught EXCEPTION event with LocationOnly modifier.
 */
public class ExceptionWithLocationTest extends EventLocationEventTestCase {
    private static final String EXCEPTION_SIGNATURE = "Lorg/apache/harmony/jpda/tests/jdwp/Events/DebuggeeException;";

    // Cache exception class ID.
    private long exceptionClassId = -1;

    /**
     * This testcase is for caught EXCEPTION event with LocationOnly
     * modifier.<BR>
     * It runs ExceptionWithLocationDebuggee that throws caught
     * DebuggeeException in two different methods.
     * The test verifies that requested EXCEPTION event occurs in the
     * expected method.
     */
    public void testExceptionLocationEvent() {
        logWriter.println("testExceptionLocationEvent STARTED");

        runEventWithLocationTest(JDWPConstants.EventKind.EXCEPTION);

        logWriter.println("testExceptionLocationEvent FINISHED");
    }

    @Override
    protected String getDebuggeeClassName() {
        return ExceptionWithLocationDebuggee.class.getName();
    }

    @Override
    protected String getDebuggeeSignature() {
        return "Lorg/apache/harmony/jpda/tests/jdwp/Events/ExceptionWithLocationDebuggee;";
    }

    @Override
    protected String getExpectedLocationMethodName() {
        return "expectedThrowException";
    }

    @Override
    protected void createEventBuilder(EventBuilder builder) {
        if (exceptionClassId == -1) {
            exceptionClassId = getClassIDBySignature(EXCEPTION_SIGNATURE);
        }
        // Receive caught DebuggeeException.
        builder.setExceptionOnly(exceptionClassId, true, false);
    }

    @Override
    protected void checkEvent(ParsedEvent event) {
        ParsedEvent.Event_EXCEPTION eventException =
                (ParsedEvent.Event_EXCEPTION) event;

        TaggedObject exception = eventException.getException();
        assertEquals(JDWPConstants.Tag.OBJECT_TAG, exception.tag);

        long thrownExceptionClassId = getObjectReferenceType(exception.objectID);
        assertEquals("Received incorrect exception",
                exceptionClassId, thrownExceptionClassId);

        Location catchLocation = eventException.getCatchLocation();
        assertNotNull("Incorrect catch location", catchLocation);
    }
}
