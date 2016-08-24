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

package org.apache.harmony.jpda.tests.jdwp.VirtualMachine;

import org.apache.harmony.jpda.tests.framework.jdwp.CommandPacket;
import org.apache.harmony.jpda.tests.framework.jdwp.JDWPCommands;
import org.apache.harmony.jpda.tests.framework.jdwp.JDWPConstants;
import org.apache.harmony.jpda.tests.framework.jdwp.Location;
import org.apache.harmony.jpda.tests.framework.jdwp.ReplyPacket;
import org.apache.harmony.jpda.tests.jdwp.share.JDWPSyncTestCase;
import org.apache.harmony.jpda.tests.share.JPDADebuggeeSynchronizer;

/**
 * JDWP Unit test for VirtualMachine.Resume command.
 */
public class Resume002Test extends JDWPSyncTestCase {
    private static final String BREAKPOINT_METHOD_NAME = "breakpointMethod";

    @Override
    protected String getDebuggeeClassName() {
        return Resume002Debuggee.class.getName();
    }

    /**
     * This testcase exercises VirtualMachine.Resume command when some threads are suspended and
     * others are running.
     * <BR>At first the test starts Resume002Debuggee, installs a breakpoint and waits for tested
     * threads to suspend on this breakpoint.
     * <BR> Then the test checks all suspended threads have a suspend count of 1 and their
     * suspend status is SUSPEND_STATUS_SUSPENDED.
     * <BR> Finally the test performs VirtualMachine.Resume command and checks all threads
     * are resumed by waiting for them to finish (the debuggee waits for them using Thread.join
     * method).
     */
    public void testResume_PartialSuspension() {
        logWriter.println("==> testResume_PartialSuspension: START...");

        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_READY);

        // Install breakpoint with EVENT_THREAD suspend policy so only some threads are
        // suspended.
        int requestID = installBreakpoint();

        // Continue debuggee.
        synchronizer.sendMessage(JPDADebuggeeSynchronizer.SGNL_CONTINUE);

        // Wait for each thread to hit our breakpoint.
        waitBreakpointHits(requestID);

        // Now some threads are suspended by a breakpoint while others are running. Check we can
        // resume all suspended threads at once with a VirtualMachine.Resume command.
        debuggeeWrapper.vmMirror.resume();

        // Check with the debuggee all threads have been resumed correctly and finished.
        boolean debuggeeReady = synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_READY);
        assertTrue("Threads did not finish", debuggeeReady);

        logWriter.println("==> testResume_PartialSuspension: END");
    }

    private int installBreakpoint() {
        long classID = getClassIDBySignature(getDebuggeeClassSignature());
        long methodID = getMethodID(classID, BREAKPOINT_METHOD_NAME);
        Location location = new Location(JDWPConstants.TypeTag.CLASS, classID, methodID, 0);
        ReplyPacket breakpointReply = debuggeeWrapper.vmMirror.setBreakpoint(location,
                JDWPConstants.SuspendPolicy.EVENT_THREAD);
        checkReplyPacket(breakpointReply, "Failed to set breakpoint");
        int requestID = breakpointReply.getNextValueAsInt();
        return requestID;
    }

    private void waitBreakpointHits(int requestID) {
        logWriter.println("Wait for all breakpoints to hit in each thread");
        for (int i = 0; i < Resume002Debuggee.THREAD_COUNT; ++i) {
            long eventThreadID = debuggeeWrapper.vmMirror.waitForBreakpoint(requestID);
            String threadName = debuggeeWrapper.vmMirror.getThreadName(eventThreadID);
            logWriter.println("Thread \"" + threadName + "\" hit breakpoint");
            checkThreadSuspendStatus(eventThreadID);
            checkThreadSuspendCount(eventThreadID);
        }
    }

    private void checkThreadSuspendStatus(long eventThreadID) {
        // Getting the thread status
        CommandPacket packet = new CommandPacket(
                JDWPCommands.ThreadReferenceCommandSet.CommandSetID,
                JDWPCommands.ThreadReferenceCommandSet.StatusCommand);
        packet.setNextValueAsThreadID(eventThreadID);

        ReplyPacket reply = debuggeeWrapper.vmMirror.performCommand(packet);
        checkReplyPacket(reply, "ThreadReference::Status command");

        reply.getNextValueAsInt();  // 'threadStatus' unused
        int suspendStatus = reply.getNextValueAsInt();
        assertAllDataRead(reply);

        assertEquals("Invalid suspend status for thread " + eventThreadID,
                     JDWPConstants.SuspendStatus.SUSPEND_STATUS_SUSPENDED,
                     suspendStatus);
    }

    private void checkThreadSuspendCount(long eventThreadID) {
        // Getting the thread suspend count
        CommandPacket packet = new CommandPacket(
                JDWPCommands.ThreadReferenceCommandSet.CommandSetID,
                JDWPCommands.ThreadReferenceCommandSet.SuspendCountCommand);
        packet.setNextValueAsThreadID(eventThreadID);

        ReplyPacket reply = debuggeeWrapper.vmMirror.performCommand(packet);
        checkReplyPacket(reply, "ThreadReference::SuspendCount command");

        int suspendCount = reply.getNextValueAsInt();
        assertAllDataRead(reply);

        assertEquals("Invalid suspend count for thread " + eventThreadID, 1, suspendCount);
    }

}
