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

package org.apache.harmony.jpda.tests.jdwp.ClassType;

import org.apache.harmony.jpda.tests.framework.jdwp.CommandPacket;
import org.apache.harmony.jpda.tests.framework.jdwp.JDWPCommands;
import org.apache.harmony.jpda.tests.framework.jdwp.JDWPConstants;
import org.apache.harmony.jpda.tests.framework.jdwp.ReplyPacket;
import org.apache.harmony.jpda.tests.framework.jdwp.TaggedObject;
import org.apache.harmony.jpda.tests.framework.jdwp.Value;
import org.apache.harmony.jpda.tests.jdwp.share.JDWPSyncTestCase;
import org.apache.harmony.jpda.tests.share.JPDADebuggeeSynchronizer;


/**
 * JDWP unit test for ClassType.NewInstance command.
 */
public class NewInstance002Test extends JDWPSyncTestCase {

    protected String getDebuggeeClassName() {
        return "org.apache.harmony.jpda.tests.jdwp.ClassType.NewInstance002Debuggee";
    }

    /**
     * This testcase exercises ClassType.NewInstance command.
     * <BR>The test first starts the debuggee, request METHOD_ENTRY event so the
     * application suspends on first invoke.
     * <BR>Then sends ClassType.NewInstance command to create a new instance and
     * invoke a constructor with a null argument. Checks that returned value
     * is expected int value and returned exception object is null.
     * <BR>Finally resume the application.
     */
    public void testNewInstance_null_argument() {
        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_READY);

        // Get debuggee class ID.
        String debuggeeClassSig = "Lorg/apache/harmony/jpda/tests/jdwp/ClassType/NewInstance002Debuggee;";
        long debuggeeTypeID = debuggeeWrapper.vmMirror.getClassID(debuggeeClassSig);
        assertTrue("Failed to find debuggee class", debuggeeTypeID != 0);

        // Set METHOD_ENTRY event request so application is suspended.
        CommandPacket packet = new CommandPacket(
                JDWPCommands.EventRequestCommandSet.CommandSetID,
                JDWPCommands.EventRequestCommandSet.SetCommand);
        packet.setNextValueAsByte(JDWPConstants.EventKind.METHOD_ENTRY);
        packet.setNextValueAsByte(JDWPConstants.SuspendPolicy.ALL);
        packet.setNextValueAsInt(1);
        packet.setNextValueAsByte((byte) 4);  // class-only modifier.
        packet.setNextValueAsReferenceTypeID(debuggeeTypeID);
        ReplyPacket reply = debuggeeWrapper.vmMirror.performCommand(packet);
        checkReplyPacket(reply, "EventRequest::Set command");

        int requestID = reply.getNextValueAsInt();
        logWriter.println(" EventRequest.Set: requestID=" + requestID);
        assertAllDataRead(reply);
        synchronizer.sendMessage(JPDADebuggeeSynchronizer.SGNL_CONTINUE);

        long targetThreadID = 0;
        // Wait for METHOD_ENTRY event and collect event thread.
        CommandPacket event = debuggeeWrapper.vmMirror.receiveEvent();
        byte suspendPolicy = event.getNextValueAsByte();
        int events = event.getNextValueAsInt();
        logWriter.println(" EVENT_THREAD event: suspendPolicy=" + suspendPolicy + " events=" + events);
        for (int i = 0; i < events; i++) {
            byte eventKind = event.getNextValueAsByte();
            int newRequestID = event.getNextValueAsInt();
            long threadID = event.getNextValueAsThreadID();
            //Location location =
                event.getNextValueAsLocation();
            logWriter.println("  EVENT_THREAD event " + i + ": eventKind="
                    + eventKind + " requestID=" + newRequestID + " threadID="
                    + threadID);
            if (newRequestID == requestID) {
                targetThreadID = threadID;
            }
        }
        assertAllDataRead(event);
        assertTrue("Invalid targetThreadID, must be != 0", targetThreadID != 0);

        //  Now we're suspended, clear event request.
        packet = new CommandPacket(
                JDWPCommands.EventRequestCommandSet.CommandSetID,
                JDWPCommands.EventRequestCommandSet.ClearCommand);
        packet.setNextValueAsByte(JDWPConstants.EventKind.METHOD_ENTRY);
        packet.setNextValueAsInt(requestID);
        reply = debuggeeWrapper.vmMirror.performCommand(packet);
        checkReplyPacket(reply, "EventRequest::Clear command");
        assertAllDataRead(reply);

        // Get test class ID.
        String testClassSig = "Lorg/apache/harmony/jpda/tests/jdwp/ClassType/NewInstance002Debuggee$TestClass;";
        long testTypeID = debuggeeWrapper.vmMirror.getClassID(testClassSig);
        assertTrue("Failed to find test class", testTypeID != 0);

        // Get constructor ID.
        long targetMethodID = debuggeeWrapper.vmMirror.getMethodID(testTypeID, "<init>");
        assertTrue("Failed to find constructor", targetMethodID != 0);

        // Make NewInstance without Exception
        Value nullObjectValue = new Value(JDWPConstants.Tag.OBJECT_TAG, 0);
        packet = new CommandPacket(
                JDWPCommands.ClassTypeCommandSet.CommandSetID,
                JDWPCommands.ClassTypeCommandSet.NewInstanceCommand);
        packet.setNextValueAsClassID(testTypeID);
        packet.setNextValueAsThreadID(targetThreadID);
        packet.setNextValueAsMethodID(targetMethodID);
        packet.setNextValueAsInt(1);
        packet.setNextValueAsValue(nullObjectValue);
        packet.setNextValueAsInt(0);
        logWriter.println(" Send ClassType.NewInstance (without Exception)");
        reply = debuggeeWrapper.vmMirror.performCommand(packet);
        checkReplyPacket(reply, "ClassType::NewInstance command");

        TaggedObject newObject = reply.getNextValueAsTaggedObject();
        logWriter.println(" ClassType.NewInstance: newObject.tag="
                + newObject.tag + "; newObject.objectID=" + newObject.objectID);
        TaggedObject exception = reply.getNextValueAsTaggedObject();
        logWriter.println(" ClassType.NewInstance: exception.tag="
                + exception.tag + "; exception.objectID=" + exception.objectID);

        assertNotNull("newObject is null", newObject);
        assertTrue("newObject.objectID is 0", newObject.objectID != 0);
        assertEquals("ClassType::NewInstance returned invalid newObject.tag,", JDWPConstants.Tag.OBJECT_TAG, newObject.tag
                , JDWPConstants.Tag.getName(JDWPConstants.Tag.OBJECT_TAG)
                , JDWPConstants.Tag.getName(newObject.tag));

        assertNotNull("exception is null", newObject);
        assertEquals("ClassType::NewInstance returned invalid exception.objectID,", 0, exception.objectID);
        assertTrue(exception.tag == JDWPConstants.Tag.OBJECT_TAG);
        assertAllDataRead(reply);

        //  Let's resume application
        debuggeeWrapper.vmMirror.resume();

        synchronizer.sendMessage(JPDADebuggeeSynchronizer.SGNL_CONTINUE);
    }
}
