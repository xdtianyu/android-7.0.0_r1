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
 * @author Vitaly A. Provodin, Anatoly F. Bondarenko
 */

/**
 * Created on 10.02.2005
 */
package org.apache.harmony.jpda.tests.jdwp.VirtualMachine;

import org.apache.harmony.jpda.tests.framework.jdwp.CommandPacket;
import org.apache.harmony.jpda.tests.framework.jdwp.JDWPCommands;
import org.apache.harmony.jpda.tests.framework.jdwp.JDWPConstants;
import org.apache.harmony.jpda.tests.framework.jdwp.ReplyPacket;
import org.apache.harmony.jpda.tests.framework.jdwp.exceptions.ReplyErrorCodeException;
import org.apache.harmony.jpda.tests.jdwp.share.JDWPSyncTestCase;

import java.util.ArrayList;
import java.util.List;


/**
 * JDWP Unit test for VirtualMachine.Resume command.
 */
public class ResumeTest extends JDWPSyncTestCase {

    static final String debuggeeSignature =
        "Lorg/apache/harmony/jpda/tests/jdwp/VirtualMachine/ResumeDebuggee;";

    protected String getDebuggeeClassName() {
        return ResumeDebuggee.class.getName();
    }

    @Override
    protected void internalTearDown() {
        // We need to finish the tested threads before detaching.
        logWriter.println("Finish debuggee tested threads");
        setStaticIntField(debuggeeSignature,
                ResumeDebuggee.TO_FINISH_DEBUGGEE_FIELD_NAME, 99);
        super.internalTearDown();
    }

    /**
     * This testcase exercises VirtualMachine.Resume command.
     * <BR>At first the test starts ResumeDebuggee which starts and runs some tested threads.
     * <BR> Then the test performs VirtualMachine.Suspend command and checks with help of
     * ThreadReference.Status command that all debuggee tested threads are suspended.
     * <BR> Then the test performs VirtualMachine.Resume command and checks with help of
     * ThreadReference.Status command that all debuggee tested threads are resumed.
     */
    public void testResume001() {
        logWriter.println("==> testResume001: START...");

        // The error messages in case of test failure.
        List<String> errorMessages = new ArrayList<String>();

        // All the threads we're interested in.
        ThreadInfo[] threadInfos = createThreadInfos();

        // Suspend all threads with VirtualMachine.Suspend command.
        suspendAll();

        // Check all threads are suspended now.
        logWriter.println("\n==> Check that all tested threads are suspended " +
                "after VirtualMachine.Suspend command...");
        checkThreadStatus(threadInfos, true, errorMessages);

        // Resume all threads with VirtualMachine.Resume command.
        resumeAll();

        // Check all threads are NOT suspended anymore.
        logWriter.println("\n==> Check that all tested threads are resumed " +
                "after VirtualMachine.Resume command...");
        checkThreadStatus(threadInfos, false, errorMessages);

        if (!errorMessages.isEmpty()) {
            // Print error messages first.
            for (String errorMessage : errorMessages) {
                logWriter.printError(errorMessage + "\n");
            }
            printErrorAndFail("\ntestResume001 FAILED");
        } else {
            logWriter.println("\n==> testResume001 - OK!");
        }
    }

    /**
     * This testcase exercises VirtualMachine.Resume command.
     * <BR>At first the test starts ResumeDebuggee which starts and runs some
     * tested threads.
     * <BR> Then the test performs VirtualMachine.Suspend command twice and
     * checks, with help of ThreadReference.Status command, that all debuggee
     * tested threads are suspended.
     * <BR> Then the test performs VirtualMachine.Resume command and checks
     * that all debuggee tested threads are still suspended.
     * <BR> Then the test performs VirtualMachine.Resume command again and
     * checks that all debuggee tested threads are resumed.
     */
    public void testResume002() {
        logWriter.println("==> testResume002: START...");

        // The error messages in case of test failure.
        List<String> errorMessages = new ArrayList<String>();

        // All the threads we're interested in.
        ThreadInfo[] threadInfos = createThreadInfos();

        // Suspend all threads with VirtualMachine.Suspend command.
        suspendAll();

        // Check all threads are suspended now.
        logWriter.println("\n==> Check that all tested threads are suspended " +
                "after VirtualMachine.Suspend command...");
        checkThreadStatus(threadInfos, true, errorMessages);

        // Suspend all threads again.
        suspendAll();

        // Check all threads are still suspended.
        logWriter.println("\n==> Check that all tested threads are still " +
                "suspended after another VirtualMachine.Suspend command...");
        checkThreadStatus(threadInfos, true, errorMessages);

        // Resume all threads with VirtualMachine.Resume command.
        resumeAll();

        // Check all threads are still suspended.
        logWriter.println("\n==> Check that all tested threads are still " +
                "suspended after VirtualMachine.Resume command...");
        checkThreadStatus(threadInfos, true, errorMessages);

        // Resume all threads again.
        resumeAll();

        // Check all threads are NOT suspended anymore.
        logWriter.println("\n==> Check that all tested threads are resumed " +
                "after VirtualMachine.Resume command...");
        checkThreadStatus(threadInfos, false, errorMessages);

        if (!errorMessages.isEmpty()) {
            // Print error messages first.
            for (String errorMessage : errorMessages) {
                logWriter.printError(errorMessage + "\n");
            }
            printErrorAndFail("\ntestResume002 FAILED");
        } else {
            logWriter.println("\n==> testResume002 - OK!");
        }
    }

    /**
     * This testcase exercises VirtualMachine.Resume command.
     * <BR>At first the test starts ResumeDebuggee which starts and runs some
     * tested threads.
     * <BR> Then the test performs VirtualMachine.Resume command and checks it
     * does not cause any error if we do not perform VirtualMachine.Suspend
     * before.
     * <BR> Then the test performs VirtualMachine.Suspend command and checks
     * that all debuggee tested threads are suspended.
     * <BR> Then the test performs VirtualMachine.Resume command and checks
     * that all debuggee tested threads are resumed.
     */
    public void testResume003() {
        logWriter.println("==> testResume002: START...");

        // The error messages in case of test failure.
        List<String> errorMessages = new ArrayList<String>();

        // All the threads we're interested in.
        ThreadInfo[] threadInfos = createThreadInfos();

        // Resume all threads: should be a no-op.
        resumeAll();

        // Check all threads are NOT suspended.
        logWriter.println("\n==> Check that no tested thread is suspended " +
                "after VirtualMachine.Resume command...");
        checkThreadStatus(threadInfos, false, errorMessages);

        // Suspend all threads with VirtualMachine.Suspend command.
        suspendAll();

        // Check all threads are suspended now.
        logWriter.println("\n==> Check that all tested threads are suspended " +
                "after VirtualMachine.Suspend command...");
        checkThreadStatus(threadInfos, true, errorMessages);

        // Resume all threads with VirtualMachine.Resume command.
        resumeAll();

        // Check all threads are NOT suspended anymore.
        logWriter.println("\n==> Check that all tested threads are resumed " +
                "after VirtualMachine.Resume command...");
        checkThreadStatus(threadInfos, false, errorMessages);

        if (!errorMessages.isEmpty()) {
            // Print error messages first.
            for (String errorMessage : errorMessages) {
                logWriter.printError(errorMessage + "\n");
            }
            printErrorAndFail("\ntestResume002 FAILED");
        } else {
            logWriter.println("\n==> testResume002 - OK!");
        }
    }
    private static class ThreadInfo {
        final String threadName;
        long threadId = 0;

        public ThreadInfo(String threadName) {
            this.threadName = threadName;
        }
    }

    /**
     * Suspends all threads using VirtualMachine.Suspend command.
     */
    private void suspendAll() {
        logWriter.println("\n==> Send VirtualMachine.Suspend command...");
        CommandPacket packet = new CommandPacket(
                JDWPCommands.VirtualMachineCommandSet.CommandSetID,
                JDWPCommands.VirtualMachineCommandSet.SuspendCommand);
        ReplyPacket reply = debuggeeWrapper.vmMirror.performCommand(packet);
        checkReplyPacket(reply, "VirtualMachine.Suspend");
        logWriter.println("==> VirtualMachine.Suspend command - OK.");
    }

    /**
     * Resumes all threads using VirtualMachine.Resume command.
     */
    private void resumeAll() {
        logWriter.println("\n==> Send VirtualMachine.Resume command...");
        CommandPacket packet = new CommandPacket(
                JDWPCommands.VirtualMachineCommandSet.CommandSetID,
                JDWPCommands.VirtualMachineCommandSet.ResumeCommand);
        ReplyPacket reply = debuggeeWrapper.vmMirror.performCommand(packet);
        checkReplyPacket(reply, "VirtualMachine.Resume");
        logWriter.println("==> VirtualMachine.Resume command - OK.");
    }

    /**
     * Returns the number of threads used in the tests (including the main
     * thread).
     */
    private int getThreadsNumber() {
        String debuggeeMessage = synchronizer.receiveMessage();
        int testedThreadsNumber = 0;
        try {
            testedThreadsNumber = Integer.valueOf(debuggeeMessage).intValue();
        } catch (NumberFormatException exception) {
            logWriter.println("## FAILURE: Exception while getting number of"
                    + " started threads from debuggee = " + exception);
            printErrorAndFail("\n## Can NOT get number of started threads "
                    + "from debuggee! ");
        }
        return testedThreadsNumber + 1;  // to add debuggee main thread
    }

    /**
     * Creates ThreadInfo array containing information about each tested thread:
     * thread name and thread JDWP id.
     */
    private ThreadInfo[] createThreadInfos() {
        int testedThreadsNumber = getThreadsNumber();
        logWriter.println("==>  Number of threads in debuggee to test = "
                + testedThreadsNumber);
        ThreadInfo[] threadInfos = new ThreadInfo[testedThreadsNumber];

        String debuggeeMainThreadName = synchronizer.receiveMessage();
        // Initialize all threads
        for (int i = 0, e = threadInfos.length - 1; i < e; ++i) {
            threadInfos[i] = new ThreadInfo(ResumeDebuggee.THREAD_NAME_PATTERN + i);
        }
        threadInfos[threadInfos.length - 1] = new ThreadInfo(debuggeeMainThreadName);

        // Getting ID of the tested thread using VirtualMachine.AllThreads.
        ReplyPacket allThreadIDReply = null;
        try {
            allThreadIDReply = debuggeeWrapper.vmMirror.getAllThreadID();
        } catch (ReplyErrorCodeException exception) {
            logWriter.println
                ("## FAILURE: Exception in vmMirror.getAllThreadID() = " + exception);
            printErrorAndFail("\n## Can NOT get all ThreadID in debuggee! ");
        }
        int threads = allThreadIDReply.getNextValueAsInt();
        logWriter.println("==>  Number of all threads in debuggee = " + threads);
        for (int i = 0; i < threads; i++) {
            long threadID = allThreadIDReply.getNextValueAsThreadID();
            String threadName = null;
            try {
                threadName = debuggeeWrapper.vmMirror.getThreadName(threadID);
            } catch (ReplyErrorCodeException exception) {
                logWriter.println
                    ("==> WARNING: Can NOT get thread name for threadID = " + threadID);
                continue;
            }
            for (ThreadInfo threadInfo : threadInfos) {
                if (threadInfo.threadName.equals(threadName) ) {
                    threadInfo.threadId = threadID;
                    break;
                }
            }
        }

        // Check we found thread id for each thread.
        boolean testedThreadNotFound = false;
        for (ThreadInfo threadInfo : threadInfos) {
            if (threadInfo.threadId == 0) {
                logWriter.println("## FAILURE: Tested thread is not found out "
                        + "among debuggee threads!");
                logWriter.println("##          Thread name = "
                        + threadInfo.threadName);
                testedThreadNotFound = true;
            }
        }
        if (testedThreadNotFound) {
            printErrorAndFail("\n## Some of tested threads are not found!");
        }

        return threadInfos;
    }

    /**
     * Checks suspend status of each tested thread is the expected one.
     *
     * @param threadInfos
     *          the thread information
     * @param isSuspended
     *          if true, thread must be suspended; otherwise thread
     *          must not be suspended.
     * @param errorMessages
     *          a list of String to append error message.
     */
    private void checkThreadStatus(ThreadInfo[] threadInfos,
            boolean isSuspended, List<String> errorMessages) {
        boolean statusCommandFailed = false;
        boolean suspendStatusFailed = false;

        for (ThreadInfo threadInfo : threadInfos) {
            logWriter.println("\n==> Check for Thread: threadID = "
                    + threadInfo.threadId
                    + " (" + threadInfo.threadName + ")");

            logWriter.println("==> Send ThreadReference.Status command...");
            CommandPacket packet = new CommandPacket(
                    JDWPCommands.ThreadReferenceCommandSet.CommandSetID,
                    JDWPCommands.ThreadReferenceCommandSet.StatusCommand);
            packet.setNextValueAsThreadID(threadInfo.threadId);
            ReplyPacket reply = debuggeeWrapper.vmMirror.performCommand(packet);
            if (!checkReplyPacketWithoutFail(reply, "ThreadReference.Status command")) {
                logWriter.println("Can't get thread status for thread " +
                        threadInfo.threadId +
                        " \"" + threadInfo.threadName + "\"");
                statusCommandFailed = true;
                continue;
            }

            int threadStatus = reply.getNextValueAsInt();
            int suspendStatus = reply.getNextValueAsInt();

            logWriter.println("==> threadStatus = " + threadStatus + " ("
                    + JDWPConstants.ThreadStatus.getName(threadStatus) + ")");
            logWriter.println("==> suspendStatus = " + suspendStatus + " ("
                    + JDWPConstants.SuspendStatus.getName(suspendStatus) + ")");

            boolean isThreadSuspended =
                    (suspendStatus == JDWPConstants.SuspendStatus.SUSPEND_STATUS_SUSPENDED);
            if (isThreadSuspended != isSuspended) {
                logWriter.println("## FAILURE: Unexpected suspendStatus for " +
                        "checked thread " + threadInfo.threadId +
                        " \"" + threadInfo.threadName + "\"");
                logWriter.println("##          Expected suspendStatus  = "
                        + JDWPConstants.SuspendStatus.SUSPEND_STATUS_SUSPENDED
                        + "(" + JDWPConstants.SuspendStatus.getName
                        (JDWPConstants.SuspendStatus.SUSPEND_STATUS_SUSPENDED) +")");
                suspendStatusFailed = true;
                continue;
            }
        }

        if (statusCommandFailed) {
            errorMessages.add("## Error found out while ThreadReference.Status "
                    + "command performing!");
        }
        if (suspendStatusFailed) {
            errorMessages.add("## Unexpected suspendStatus found out!");
        }
    }
}
