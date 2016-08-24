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
 * @author Aleksander V. Budniy

 /**
 * Created on 15.08.2005
 */
package org.apache.harmony.jpda.tests.jdwp.StackFrame;

import org.apache.harmony.jpda.tests.framework.jdwp.CommandPacket;
import org.apache.harmony.jpda.tests.framework.jdwp.JDWPCommands;
import org.apache.harmony.jpda.tests.framework.jdwp.JDWPCommands.StackFrameCommandSet;
import org.apache.harmony.jpda.tests.framework.jdwp.JDWPConstants;
import org.apache.harmony.jpda.tests.framework.jdwp.ReplyPacket;
import org.apache.harmony.jpda.tests.framework.jdwp.Value;

import java.util.Arrays;

/**
 * JDWP Unit test for StackFrame.SetValues command.
 */
public class SetValuesTest extends JDWPStackTraceBaseTest {
    /**
     * This testcase exercises StackFrame.SetValues command.
     * <BR>The test starts StackTraceDebuggee, sets breakpoint at the beginning of
     * the tested method - 'nestledMethod3' and stops at the breakpoint.
     * <BR> Then the test performs Method.VariableTable command and checks
     * returned VariableTable.
     * <BR> Next, the test performs StackFrame.SetValues command and checks
     * result of this command with help of StackFrame.GetValues command -
     * returned values of variables should be equal to values which were set.
     *
     */
    public void testSetValues001() {
        // Sets and checks local variables of tested method
        examineGetValues();
    }

    /**
     * This testcase exercises StackFrame.SetValues command.
     * <BR>The test starts StackTraceDebuggee and waits for it to reach the
     * tested method - 'nestledMethod3'.
     * <BR> Then it performs StackFrame.SetValues command with an invalid
     * thread ID and checks the command returns error INVALID_OBJECT.
     */
    public void testSetValues002_InvalidObjectError() {
        long invalidThreadID = -1;
        logWriter.println("Send StackFrame.SetValues with invalid thread " + invalidThreadID);
        CommandPacket packet = new CommandPacket(StackFrameCommandSet.CommandSetID,
                StackFrameCommandSet.SetValuesCommand);
        packet.setNextValueAsThreadID(invalidThreadID);
        packet.setNextValueAsFrameID(0);
        packet.setNextValueAsInt(0);
        ReplyPacket replyPacket = debuggeeWrapper.vmMirror.performCommand(packet);
        checkReplyPacket(replyPacket, "StackFrame.SetValues",
                JDWPConstants.Error.INVALID_OBJECT);
    }

    /**
     * This testcase exercises StackFrame.SetValues command.
     * <BR>The test starts StackTraceDebuggee and waits for it to reach the
     * tested method - 'nestledMethod3'.
     * <BR> Then it performs StackFrame.SetValues command to a non-suspended
     * thread and checks the command returns error THREAD_NOT_SUSPENDED.
     */
    public void testSetValues003_ThreadNotSuspendedError() {
        long threadID = getThreadID();

        CommandPacket packet = new CommandPacket(StackFrameCommandSet.CommandSetID,
                StackFrameCommandSet.SetValuesCommand);
        packet.setNextValueAsThreadID(threadID);
        packet.setNextValueAsFrameID(0);
        packet.setNextValueAsInt(0);
        ReplyPacket replyPacket = debuggeeWrapper.vmMirror.performCommand(packet);
        checkReplyPacket(replyPacket, "StackFrame.SetValues",
                JDWPConstants.Error.THREAD_NOT_SUSPENDED);
    }

    /**
     * This testcase exercises StackFrame.SetValues command.
     * <BR>The test starts StackTraceDebuggee and waits for it to reach the
     * tested method - 'nestledMethod3'.
     * <BR> Then it performs StackFrame.SetValues command to an invalid
     * frame ID and checks the command returns error INVALID_FRAMEID.
     */
    public void testSetValues004_InvalidFrameIDError() {
        long threadID = getThreadID();

        // suspend thread
        jdwpSuspendThread(threadID);

        long invalidFrameID = -1;
        logWriter.println("Send StackFrame.SetValues with invalid frameID " + invalidFrameID);
        CommandPacket packet = new CommandPacket(StackFrameCommandSet.CommandSetID,
                StackFrameCommandSet.SetValuesCommand);
        packet.setNextValueAsThreadID(threadID);
        packet.setNextValueAsFrameID(invalidFrameID);
        packet.setNextValueAsInt(0);
        ReplyPacket replyPacket = debuggeeWrapper.vmMirror.performCommand(packet);
        checkReplyPacket(replyPacket, "StackFrame.SetValues",
                JDWPConstants.Error.INVALID_FRAMEID);
    }

    /**
     * This testcase exercises StackFrame.SetValues command.
     * <BR>The test starts StackTraceDebuggee and waits for it to reach the
     * tested method - 'nestledMethod3'.
     * <BR> Then it performs StackFrame.SetValues command to an invalid
     * slot ID and checks the command returns error INVALID_SLOT.
     */
    public void testSetValues005_InvalidSlotError() {
        //sets and checks local variables of tested method

        long threadID = getThreadID();

        // suspend thread
        jdwpSuspendThread(threadID);

        FrameInfo frameInfo = findFrameInfo(threadID);

        //getting Variable Table
        logWriter.println("");
        logWriter.println("=> Getting Variable Table...");
        long refTypeID = getClassIDBySignature(getDebuggeeClassSignature());
        logWriter.println("=> Debuggee class = " + getDebuggeeClassName());
        logWriter.println("=> referenceTypeID for Debuggee class = "
                + refTypeID);
        varInfos = jdwpGetVariableTable(refTypeID, frameInfo.location.methodID);
        if (GetValuesTest.checkVarTable(logWriter, varInfos, varTags, varSignatures, varNames)) {
            logWriter.println("=> Variable table check passed.");
        } else {
            printErrorAndFail("Variable table check failed.");
        }

        int invalidSlotId = -1;
        logWriter.println("Send StackFrame.SetValues with invalid slot " + invalidSlotId);
        CommandPacket packet = new CommandPacket(StackFrameCommandSet.CommandSetID,
                StackFrameCommandSet.SetValuesCommand);
        packet.setNextValueAsThreadID(threadID);
        packet.setNextValueAsFrameID(frameInfo.frameID);
        packet.setNextValueAsInt(1);
        packet.setNextValueAsInt(invalidSlotId);
        packet.setNextValueAsByte(JDWPConstants.Tag.OBJECT_TAG);
        packet.setNextValueAsObjectID(0 /* null */);
        ReplyPacket replyPacket = debuggeeWrapper.vmMirror.performCommand(packet);
        checkReplyPacket(replyPacket, "StackFrame.SetValues",
                JDWPConstants.Error.INVALID_SLOT);
    }

    private void examineGetValues() {
        long refTypeID = getClassIDBySignature(getDebuggeeClassSignature());
        logWriter.println("=> Debuggee class = " + getDebuggeeClassName());
        logWriter.println("=> referenceTypeID for Debuggee class = "
                + refTypeID);
        long threadID = debuggeeWrapper.vmMirror.getThreadID(testedThreadName);

        logWriter.println("=> testedThreadID = " + threadID);
        if (threadID == -1) {
            printErrorAndFail("testedThread is not found!");
        }

        // suspend thread
        jdwpSuspendThread(threadID);

        // find frame for the tested method
        FrameInfo frameInfo = findFrameInfo(threadID);

        //getting Variable Table
        logWriter.println("");
        logWriter.println("=> Getting Variable Table...");
        varInfos = jdwpGetVariableTable(refTypeID, frameInfo.location.methodID);
        if (checkVarTable(logWriter, varInfos, varTags, varSignatures, varNames)) {
            logWriter.println("=> Variable table check passed.");
        } else {
            printErrorAndFail("Variable table check failed.");
        }

        //prepare and perform SetValues command
        logWriter.println("");
        logWriter.println("==> Send StackFrame::SetValues command...");
        CommandPacket packet = new CommandPacket(
                JDWPCommands.StackFrameCommandSet.CommandSetID,
                JDWPCommands.StackFrameCommandSet.SetValuesCommand);
        packet.setNextValueAsThreadID(threadID);
        packet.setNextValueAsFrameID(frameInfo.frameID);

        packet.setNextValueAsInt(varTags.length-2);
        packet.setNextValueAsInt(varInfoByName("boolLocalVariable").getSlot());
        packet.setNextValueAsValue(new Value(false));
        packet.setNextValueAsInt(varInfoByName("intLocalVariable").getSlot());
        packet.setNextValueAsValue(new Value((int)12345));

        ReplyPacket reply = debuggeeWrapper.vmMirror.performCommand(packet);
        checkReplyPacket(reply, "StackFrame::SetValues command");

        //prepare and perform GetValues command
        logWriter.println("");
        logWriter.println("=> Send StackFrame::GetValues command...");
        packet = new CommandPacket(
                JDWPCommands.StackFrameCommandSet.CommandSetID,
                JDWPCommands.StackFrameCommandSet.GetValuesCommand);
        packet.setNextValueAsThreadID(threadID);
        packet.setNextValueAsFrameID(frameInfo.frameID);

        logWriter.println("=> Thread: " + threadID);
        logWriter.println("=> Frame: " + frameInfo.frameID);
        packet.setNextValueAsInt(varTags.length);
        for (int i = 0; i < varTags.length; i++) {
            logWriter.println("");
            logWriter.println("=> For variable #"+i+" " + varInfos[i].getName() + ":");
            packet.setNextValueAsInt(varInfos[i].getSlot());
            logWriter.println("=> Slot = "+varInfos[i].getSlot());
            byte tag = varTags[Arrays.asList(varNames).indexOf(varInfos[i].getName())];
            packet.setNextValueAsByte(tag);
            logWriter.println("=> Tag = "+JDWPConstants.Tag.getName(tag));
            logWriter.println("");
        }

        //check reply for errors
        reply = debuggeeWrapper.vmMirror.performCommand(packet);
        checkReplyPacket(reply, "StackFrame::GetValues command");

        //check number of retrieved values
        int numberOfValues = reply.getNextValueAsInt();
        logWriter.println("=> Number of values = " + numberOfValues);
        if (numberOfValues != varTags.length) {
            logWriter.println("##FAILURE: unexpected number of values: "
                    + numberOfValues + " instead of "+varTags.length);
            assertTrue(false);
        }

        boolean success = true;
        //print and check values of variables
        logWriter.println("=> Values of variables: ");

        for (int i = 0; i < numberOfValues; ++i ) {
            Value val = reply.getNextValueAsValue();
            switch (val.getTag()) {
                case JDWPConstants.Tag.BOOLEAN_TAG: {
                    logWriter.println("=>Tag is correct");
                    boolean boolValue = val.getBooleanValue();
                    if (!boolValue) {
                        logWriter.println("=> "+varInfos[1].getName() + " = " + boolValue);
                        logWriter.println("");
                    } else {
                        logWriter
                        .printError("Unexpected value of boolean variable: "
                                + boolValue + " instead of: false");
                        logWriter.printError("");
                        success = false;
                    }
                    break;
                }

                case JDWPConstants.Tag.INT_TAG: {
                    logWriter.println("=>Tag is correct");
                    int intValue = val.getIntValue();
                    if (intValue == 12345) {
                        logWriter.println("=> "+varInfos[2].getName() + " = " + intValue);
                        logWriter.println("");
                    } else {
                        logWriter
                        .printError("Unexpected value of int variable: "
                                + intValue + " instead of: 12345");
                        logWriter.printError("");
                        success = false;
                    }
                    break;
                }

                case JDWPConstants.Tag.STRING_TAG: {
                    logWriter.println("=>Tag is correct");
                    long strLocalVariableID = val.getLongValue();
                    String strLocalVariable = getStringValue(strLocalVariableID);
                    if (strLocalVariable.equals("test string")) {
                        logWriter.println("=> "+varInfos[2].getName() + " = "
                                + strLocalVariable);
                        logWriter.println("");
                    } else {
                        logWriter
                        .printError("Unexpected value of string variable: "
                                + strLocalVariable
                                + " instead of: "
                                + "test string");
                        logWriter.printError("");
                        success = false;
                    }
                    break;
                }

                case JDWPConstants.Tag.OBJECT_TAG: {
                    logWriter.println("=> Tag is correct");
                    logWriter.println("");
                    break;
                }
                default:
                    logWriter.printError("Unexpected tag of variable: "
                            + JDWPConstants.Tag.getName(val.getTag()));
                    logWriter.printError("");
                    success = false;
                    break;
            }  // switch
        }  // for

        assertTrue(logWriter.getErrorMessage(), success);
    }

    private VarInfo varInfoByName(String name) {
      for (int i = 0; i < varInfos.length; ++i) {
        if (varInfos[i].getName().equals(name)) {
          return varInfos[i];
        }
      }
      return null;
    }
}
