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

import org.apache.harmony.jpda.tests.framework.jdwp.JDWPConstants;
import org.apache.harmony.jpda.tests.framework.jdwp.Location;
import org.apache.harmony.jpda.tests.framework.jdwp.ParsedEvent;
import org.apache.harmony.jpda.tests.framework.jdwp.ReplyPacket;
import org.apache.harmony.jpda.tests.framework.jdwp.TaggedObject;
import org.apache.harmony.jpda.tests.share.JPDADebuggeeSynchronizer;

/**
 * JDWP Unit test for uncaught EXCEPTION event.
 */
public class ExceptionUncaughtTest extends ExceptionBaseTest {
    private static final String EXCEPTION_SIGNATURE = getClassSignature(DebuggeeException.class);

    private static final String THROW_EXCEPTION_METHOD = "throwDebuggeeException";

    protected String getDebuggeeClassName() {
        return ExceptionUncaughtDebuggee.class.getName();
    }

    /**
     * This testcase is for uncaught EXCEPTION event and reported exception object.
     * <BR>It runs ExceptionUncaughtDebuggee that throws an uncaught DebuggeeException.
     * It verifies the following:
     * <ul>
     * <li>the requested EXCEPTION event occurs</li>
     * <li>the reported exception object is not null</li>
     * <li>the reported exception object is instance of expected class with expected tag</li>
     * </ul>
     */
    public void testExceptionUncaughtEvent_ExceptionObject() {
        printTestLog("STARTED...");

        ParsedEvent.Event_EXCEPTION exceptionEvent = requestAndReceiveExceptionEvent();
        TaggedObject returnedException = exceptionEvent.getException();

        // assert that exception ObjectID is not null
        printTestLog("returnedException.objectID = " + returnedException.objectID);
        assertTrue("Returned exception object is null.", returnedException.objectID != 0);

        // assert that exception tag is OBJECT
        printTestLog("returnedException.tag = " + returnedException.objectID);
        assertEquals("Returned exception tag is not OBJECT.",
                JDWPConstants.Tag.OBJECT_TAG, returnedException.tag);

        // assert that exception class is the expected one
        long typeID = getObjectReferenceType(returnedException.objectID);
        String returnedExceptionSignature = getClassSignature(typeID);
        printTestLog("returnedExceptionSignature = |" + returnedExceptionSignature+"|");
        assertString("Invalid signature of returned exception,",
                EXCEPTION_SIGNATURE, returnedExceptionSignature);

        // resume debuggee
        printTestLog("resume debuggee...");
        debuggeeWrapper.vmMirror.resume();
    }

    /**
     * This testcase is for uncaught EXCEPTION event and reported throw location.
     * <BR>It runs ExceptionUncaughtDebuggee that throws an uncaught DebuggeeException.
     * It verifies the following:
     * <ul>
     * <li>the requested EXCEPTION event occurs</li>
     * <li>the reported thread is not null</li>
     * <li>the reported throw location is not null</li>
     * <li>the reported throw location is equal to location of the top stack frame</li>
     * </ul>
     */
    public void testExceptionUncaughtEvent_ThrowLocation() {
        printTestLog("STARTED...");

        ParsedEvent.Event_EXCEPTION exceptionEvent = requestAndReceiveExceptionEvent();
        long returnedThread = exceptionEvent.getThreadID();
        Location throwLocation = exceptionEvent.getLocation();

        // assert that exception thread is not null
        printTestLog("returnedThread = " + returnedThread);
        assertTrue("Returned exception ThreadID is null,", returnedThread != 0);

        // assert that exception location is not null
        printTestLog("returnedExceptionLoc = " + throwLocation);
        assertNotNull("Returned exception location is null,", throwLocation);

        // assert that top stack frame location is not null
        Location topFrameLoc = getTopFrameLocation(returnedThread);
        printTestLog("topFrameLoc = " + topFrameLoc);
        assertNotNull("Returned top stack frame location is null,", topFrameLoc);

        // assert that locations of exception and top frame are equal
        assertTrue("Different exception and top frame location tag,",
                throwLocation.equals(topFrameLoc));

        // check throw location's method
        long debuggeeClassID = getClassIDBySignature(getDebuggeeClassSignature());
        long debuggeeThrowMethodID = getMethodID(debuggeeClassID, THROW_EXCEPTION_METHOD);
        if (debuggeeClassID != throwLocation.classID ||
            debuggeeThrowMethodID != throwLocation.methodID) {
            StringBuilder builder = new StringBuilder("Invalid method for throw location:");
            builder.append(" expected ");
            builder.append(getDebuggeeClassSignature());
            builder.append('.');
            builder.append(THROW_EXCEPTION_METHOD);
            builder.append(" but got ");
            builder.append(dumpLocation(throwLocation));
            fail(builder.toString());
        }

        // resume debuggee
        printTestLog("resume debuggee...");
        debuggeeWrapper.vmMirror.resume();
    }

    /**
     * This testcase is for uncaught EXCEPTION event and reported catch location.
     * <BR>It runs ExceptionUncaughtDebuggee that throws an uncaught DebuggeeException.
     * It verifies the following:
     * <ul>
     * <li>the requested EXCEPTION event occurs</li>
     * <li>the reported thread is not null</li>
     * <li>the reported catch location is null</li>
     * </ul>
     */
    public void testExceptionUncaughtEvent_CatchLocation() {
        printTestLog("STARTED...");

        ParsedEvent.Event_EXCEPTION exceptionEvent = requestAndReceiveExceptionEvent();
        long returnedThread = exceptionEvent.getThreadID();

        // assert that exception thread is not null
        printTestLog("returnedThread = " + returnedThread);
        assertTrue("Returned exception ThreadID is null,", returnedThread != 0);

        // assert that exception catch location is not null
        Location catchLocation = exceptionEvent.getCatchLocation();
        printTestLog("returnedExceptionLoc = " + catchLocation);
        assertNotNull("Returned exception catch location is null,", catchLocation);

        // check catch location's method
        if (!isNullLocation(catchLocation)) {
            fail("Invalid catch location: expected null but got " + dumpLocation(catchLocation));
        }

        // resume debuggee
        printTestLog("resume debuggee...");
        debuggeeWrapper.vmMirror.resume();
    }

    /**
     * Indicates whether the location is "null" or "0". Note: we don't test the type tag as it
     * can only be CLASS.
     *
     * @param location the location to test
     * @return true if the location is "null", false otherwise
     */
    private static boolean isNullLocation(Location location) {
        return location.classID == 0 && location.methodID == 0
                && location.index == 0;
    }

    /**
     * Requests and receives EXCEPTION event then checks the received event kind,
     * the received event request ID and the thread state.
     * @return the exception event
     */
    private ParsedEvent.Event_EXCEPTION requestAndReceiveExceptionEvent() {
        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_READY);

        boolean isCatch = true;
        boolean isUncatch = true;
        printTestLog("=> setException(...)...");
        ReplyPacket replyPacket = debuggeeWrapper.vmMirror.setException(EXCEPTION_SIGNATURE,
                isCatch, isUncatch);
        int requestID = replyPacket.getNextValueAsInt();
        assertAllDataRead(replyPacket);

        printTestLog("setException(...) DONE");

        printTestLog("send to Debuggee SGNL_CONTINUE...");
        synchronizer.sendMessage(JPDADebuggeeSynchronizer.SGNL_CONTINUE);

        return receiveAndCheckExceptionEvent(requestID);
    }

}
