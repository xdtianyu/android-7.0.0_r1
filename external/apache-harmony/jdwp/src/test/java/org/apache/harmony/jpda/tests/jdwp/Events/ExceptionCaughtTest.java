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
 * @author Anton V. Karnachuk
 */

/**
 * Created on 11.04.2005
 */
package org.apache.harmony.jpda.tests.jdwp.Events;

import org.apache.harmony.jpda.tests.framework.Breakpoint;
import org.apache.harmony.jpda.tests.framework.jdwp.JDWPConstants;
import org.apache.harmony.jpda.tests.framework.jdwp.Location;
import org.apache.harmony.jpda.tests.framework.jdwp.ParsedEvent;
import org.apache.harmony.jpda.tests.framework.jdwp.ReplyPacket;
import org.apache.harmony.jpda.tests.framework.jdwp.TaggedObject;
import org.apache.harmony.jpda.tests.share.JPDADebuggeeSynchronizer;



/**
 * JDWP Unit test for caught EXCEPTION event.
 */
public class ExceptionCaughtTest extends ExceptionBaseTest {

    protected String getDebuggeeClassName() {
        return ExceptionCaughtDebuggee.class.getName();
    }

    /**
     * This testcase is for caught EXCEPTION event thrown from Java code. It tests
     * the reported exception object.
     * <BR>It runs ExceptionDebuggee that throws and catches a DebuggeeException with
     * native and interpreter frames in between. It verifies the following:
     * <ul>
     * <li>the requested EXCEPTION event occurs</li>
     * <li>the reported exception object is not null</li>
     * <li>the reported exception object is instance of expected class with expected tag</li>
     * </ul>
     */
    public void testExceptionEvent_ExceptionObject_FromJava() {
        runExceptionObjectTest(false);
    }

    /**
     * This testcase is for caught EXCEPTION event thrown from native code. It tests
     * the reported exception object.
     * <BR>It runs ExceptionDebuggee that throws an exception from a native method
     * and catches it. It verifies the following:
     * <ul>
     * <li>the requested EXCEPTION event occurs</li>
     * <li>the reported exception object is not null</li>
     * <li>the reported exception object is instance of expected class with expected tag</li>
     * </ul>
     */
    public void testExceptionEvent_ExceptionObject_FromNative() {
        runExceptionObjectTest(true);
    }

    /**
     * This testcase is for caught EXCEPTION event thrown from Java code. It tests
     * the reported throw location.
     * <BR>It runs ExceptionDebuggee that throws and catches a DebuggeeException with
     * native and interpreter frames in between. It verifies the following:
     * <ul>
     * <li>the requested EXCEPTION event occurs</li>
     * <li>the reported thread is not null</li>
     * <li>the reported throw location is not null</li>
     * <li>the reported throw location is equal to location of the top stack frame</li>
     * </ul>
     */
    public void testExceptionEvent_ThrowLocation_FromJava() {
        runThrowLocationTest(false);
    }

    /**
     * This testcase is for caught EXCEPTION event thrown from native code. It tests
     * the reported throw location.
     * <BR>It runs ExceptionDebuggee that throws an exception from a native method
     * and catches it. It verifies the following:
     * <ul>
     * <li>the requested EXCEPTION event occurs</li>
     * <li>the reported thread is not null</li>
     * <li>the reported throw location is not null</li>
     * <li>the reported throw location is equal to location of the top stack frame</li>
     * </ul>
     */
    public void testExceptionEvent_ThrowLocation_FromNative() {
        runThrowLocationTest(true);
    }

    /**
     * This testcase is for caught EXCEPTION event thrown from Java code. It tests
     * the reported catch location.
     * <BR>It runs ExceptionDebuggee that throws and catches a DebuggeeException with
     * native and interpreter frames in between. It verifies the following:
     * <ul>
     * <li>the requested EXCEPTION event occurs</li>
     * <li>the reported thread is not null</li>
     * <li>the reported catch location is not null</li>
     * <li>the reported catch location is different than the top stack frame</li>
     * </ul>
     */
    public void testExceptionEvent_CatchLocation_FromJava() {
        runCatchLocationTest(false);
    }

    /**
     * This testcase is for caught EXCEPTION event thrown from native code. It tests
     * the reported catch location.
     * <BR>It runs ExceptionDebuggee that throws an exception from a native method
     * and catches it. It verifies the following:
     * <ul>
     * <li>the requested EXCEPTION event occurs</li>
     * <li>the reported thread is not null</li>
     * <li>the reported catch location is not null</li>
     * <li>the reported catch location is different than the top stack frame</li>
     * </ul>
     */
    public void testExceptionEvent_CatchLocation_FromNative() {
        runCatchLocationTest(true);
    }

    /**
     * Requests and receives EXCEPTION event then checks reported exception object.
     */
    private void runExceptionObjectTest(boolean fromNative) {
        printTestLog("STARTED...");

        ParsedEvent.Event_EXCEPTION exceptionEvent = requestAndReceiveExceptionEvent(fromNative);
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
                getExpectedExceptionSignature(fromNative), returnedExceptionSignature);

        // resume debuggee
        printTestLog("resume debuggee...");
        debuggeeWrapper.vmMirror.resume();
    }

    /**
     * Requests and receives EXCEPTION event then checks reported throw location.
     */
    private void runThrowLocationTest(boolean fromNative) {
        printTestLog("STARTED...");

        ParsedEvent.Event_EXCEPTION exceptionEvent = requestAndReceiveExceptionEvent(fromNative);
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
        assertEquals("Different exception and top frame location tag,", topFrameLoc, throwLocation);

        // check throw location's method
        String expectedThrowLocationClassSignature =
                getThrowLocationMethodClassSignature(fromNative);
        String expectedThrowLocationMethodName = getThrowLocationMethodName(fromNative);
        long debuggeeClassID = getClassIDBySignature(expectedThrowLocationClassSignature);
        long debuggeeThrowMethodID = getMethodID(debuggeeClassID, expectedThrowLocationMethodName);
        if (debuggeeClassID != throwLocation.classID ||
            debuggeeThrowMethodID != throwLocation.methodID) {
            StringBuilder builder = new StringBuilder("Invalid method for throw location:");
            builder.append(" expected ");
            builder.append(expectedThrowLocationClassSignature);
            builder.append('.');
            builder.append(expectedThrowLocationMethodName);
            builder.append(" but got ");
            builder.append(dumpLocation(throwLocation));
            fail(builder.toString());
        }

        // resume debuggee
        printTestLog("resume debuggee...");
        debuggeeWrapper.vmMirror.resume();
    }

    /**
     * Requests and receives EXCEPTION event then checks reported catch location.
     */
    private void runCatchLocationTest(boolean fromNative) {
        printTestLog("STARTED...");

        ParsedEvent.Event_EXCEPTION exceptionEvent = requestAndReceiveExceptionEvent(fromNative);
        long returnedThread = exceptionEvent.getThreadID();

        // assert that exception thread is not null
        printTestLog("returnedThread = " + returnedThread);
        assertTrue("Returned exception ThreadID is null,", returnedThread != 0);

        // assert that exception catch location is not null
        Location catchLocation = exceptionEvent.getCatchLocation();
        printTestLog("returnedExceptionLoc = " + catchLocation);
        assertNotNull("Returned exception catch location is null,", catchLocation);

        // assert that top stack frame location is not null
        Location topFrameLoc = getTopFrameLocation(returnedThread);
        printTestLog("topFrameLoc = " + topFrameLoc);
        assertNotNull("Returned top stack frame location is null,", topFrameLoc);
        assertFalse("Same throw and catch locations", catchLocation.equals(topFrameLoc));

        // check catch location's method
        String expectedCatchLocationClassSignature = getDebuggeeClassSignature();
        String expectedCatchLocationMethodName = getCatchLocationMethodName(fromNative);
        long debuggeeClassID = getClassIDBySignature(expectedCatchLocationClassSignature);
        long debuggeeThrowMethodID = getMethodID(debuggeeClassID, expectedCatchLocationMethodName);
        if (debuggeeClassID != catchLocation.classID ||
            debuggeeThrowMethodID != catchLocation.methodID) {
            StringBuilder builder = new StringBuilder("Invalid method for catch location:");
            builder.append(" expected ");
            builder.append(expectedCatchLocationClassSignature);
            builder.append('.');
            builder.append(expectedCatchLocationMethodName);
            builder.append(" but got ");
            builder.append(dumpLocation(catchLocation));
            fail(builder.toString());
        }

        // resume debuggee
        printTestLog("resume debuggee...");
        debuggeeWrapper.vmMirror.resume();
    }

    /**
     * Requests and receives EXCEPTION event then checks the received event kind,
     * the received event request ID and the thread state.
     * @return the exception event
     */
    private ParsedEvent.Event_EXCEPTION requestAndReceiveExceptionEvent(boolean fromNative) {
        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_READY);

        boolean isCatch = true;
        boolean isUncatch = true;
        printTestLog("=> setException(...)...");
        String exceptionSignature = getExpectedExceptionSignature(fromNative);
        ReplyPacket replyPacket = debuggeeWrapper.vmMirror.setException(exceptionSignature,
                isCatch, isUncatch);
        int requestID = replyPacket.getNextValueAsInt();
        assertAllDataRead(replyPacket);

        printTestLog("setException(...) DONE");

        if (!fromNative) {
            // We want native and interpreter frames between the throw and the catch
            // to check that we properly compute the catch location. We insert a native
            // frame by using reflection in the code. To insert an interpreter frame,
            // we install a breakpoint.
            // Note: we use a Count modifier to avoid suspending on the breakpoint. This
            // prevents from handling a suspend/resume sequence.
            Breakpoint breakpoint = new Breakpoint(getDebuggeeClassSignature(),
                    "throwDebuggeeExceptionWithTransition", 0);
            debuggeeWrapper.vmMirror.setCountableBreakpoint(JDWPConstants.TypeTag.CLASS,
                    breakpoint, JDWPConstants.SuspendPolicy.ALL, 10);
        }

        printTestLog("send to Debuggee SGNL_CONTINUE...");

        String signalToSend = getSignalMessage(fromNative);
        synchronizer.sendMessage(signalToSend);

        return receiveAndCheckExceptionEvent(requestID);
    }

    static String getExpectedExceptionSignature(boolean fromNative) {
        Class<?> c = fromNative ? NullPointerException.class : DebuggeeException.class;
        return getClassSignature(c);
    }

    static String getThrowLocationMethodName(boolean fromNative) {
        return fromNative ? "arraycopy" : "throwDebuggeeException";
    }

    String getThrowLocationMethodClassSignature(boolean fromNative) {
        return fromNative ? getClassSignature(System.class) : getDebuggeeClassSignature();
    }

    static String getCatchLocationMethodName(boolean fromNative) {
        return fromNative ? "testThrowAndCatchExceptionFromNative"
                : "testThrowAndCatchDebuggeeExceptionFromJava";
    }

    static String getSignalMessage(boolean fromNative) {
        return fromNative ? ExceptionCaughtDebuggee.TEST_EXCEPTION_FROM_NATIVE_METHOD
                : JPDADebuggeeSynchronizer.SGNL_CONTINUE;
    }
}
