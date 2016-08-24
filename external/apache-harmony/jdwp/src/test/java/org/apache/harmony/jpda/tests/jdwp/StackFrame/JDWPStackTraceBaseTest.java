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

package org.apache.harmony.jpda.tests.jdwp.StackFrame;

import org.apache.harmony.jpda.tests.framework.LogWriter;
import org.apache.harmony.jpda.tests.framework.jdwp.JDWPConstants;
import org.apache.harmony.jpda.tests.share.JPDADebuggeeSynchronizer;

import java.util.ArrayList;
import java.util.Arrays;

/**
 * Base class for GetValuesTest and SetValuesTest classes.
 */
public class JDWPStackTraceBaseTest extends JDWPStackFrameTestCase {
    String testedMethodName = "nestledMethod3";
    String testedThreadName = "";

    VarInfo[] varInfos;

    String varSignatures[] = {
            "Lorg/apache/harmony/jpda/tests/jdwp/StackFrame/StackTraceDebuggee;",
            "Z",
            "I",
            "Ljava/lang/String;"
    };

    String varNames[] = {
            "this",
            "boolLocalVariable",
            "intLocalVariable",
            "strLocalVariable"
    };

    byte varTags[] = {
            JDWPConstants.Tag.OBJECT_TAG,
            JDWPConstants.Tag.BOOLEAN_TAG,
            JDWPConstants.Tag.INT_TAG,
            JDWPConstants.Tag.STRING_TAG
    };

    @Override
    protected void internalSetUp() throws Exception {
        super.internalSetUp();

        logWriter.println("==> " + getName() + " started...");
        testedThreadName = synchronizer.receiveMessage();
        // release on run()
        synchronizer.sendMessage(JPDADebuggeeSynchronizer.SGNL_CONTINUE);

        // pass nestledMethod1()
        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_READY);
        synchronizer.sendMessage(JPDADebuggeeSynchronizer.SGNL_CONTINUE);

        // enter nestledMethod2()
        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_READY);

        // release debuggee
        synchronizer.sendMessage(JPDADebuggeeSynchronizer.SGNL_CONTINUE);
        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_READY);
    }

    @Override
    protected void internalTearDown() {
        // signal to finish debuggee
        synchronizer.sendMessage(JPDADebuggeeSynchronizer.SGNL_CONTINUE);
        logWriter.println("==> " + getName() + " - OK.");

        super.internalTearDown();
    }

    protected long getThreadID() {
        long threadID = debuggeeWrapper.vmMirror.getThreadID(testedThreadName);
        logWriter.println("=> testedThreadID = " + threadID);
        if (threadID == -1) {
            printErrorAndFail("testedThread is not found!");
        }
        return threadID;
    }

    protected FrameInfo findFrameInfo(long threadID) {
        // get number of frames
        int frameCount = jdwpGetFrameCount(threadID);
        logWriter.println("=> frames count = " + frameCount);

        // get frames info
        FrameInfo[] frameIDs = jdwpGetFrames(threadID, 0, frameCount);
        if (frameIDs.length != frameCount) {
            printErrorAndFail("Received number of frames = "
                    + frameIDs.length + " differ from expected number = "
                    + frameCount);
        }

        // check and print methods info
        FrameInfo frameInfo = null;
        for (int i = 0; i < frameCount; i++) {
            logWriter.println("\n");
            String methodName = getMethodName(frameIDs[i].location.classID,
                    frameIDs[i].location.methodID);
            logWriter.println("=> method name = " + methodName);
            logWriter.println("=> methodID = " + frameIDs[i].location.methodID);
            logWriter.println("=> frameID = " + frameIDs[i].frameID);
            logWriter.println("\n");
            if (methodName.equals(testedMethodName)) {
                frameInfo = frameIDs[i];
            }
        }
        if (frameInfo != null) {
            logWriter.println("=> Tested method is found");
            logWriter.println("=> method name = " + testedMethodName);
            logWriter.println("=> methodID = " + frameInfo.location.methodID);
            logWriter.println("=> frameID = " + frameInfo.frameID);
        } else {
            printErrorAndFail("Tested method is not found");
        }
        return frameInfo;
    }

    // prints variables info, checks signatures
    protected static boolean checkVarTable(LogWriter logWriter, VarInfo[] varInfos,
            byte[] varTags, String[] varSignatures, String[] varNames) {
        logWriter.println("==> Number of variables = " + varInfos.length);
        if (varInfos.length != varTags.length) {
            logWriter.printError("Unexpected number of variables: "
                    + varInfos.length + " instead of " + varTags.length);
            return false;
        }
        boolean success = true;
        ArrayList<String> remaining = new ArrayList<String>(Arrays.asList(varNames));
        for (int i = 0; i < varInfos.length; i++) {
            logWriter.println("");
            logWriter.println("=> Name = " + varInfos[i].getName());
            logWriter.println("=> Slot = " + varInfos[i].getSlot());
            logWriter.println("=> Signature = " + varInfos[i].getSignature());
            // TODO: check codeIndex and length too

            int index = remaining.indexOf(varInfos[i].getName());
            if (index == -1) {
                logWriter.printError("unknown variable: " + varInfos[i].getName());
            }
            remaining.set(index, null);

            if (!varSignatures[index].equals(varInfos[i].getSignature())) {
                logWriter
                        .printError("Unexpected signature of variable = "
                                + varInfos[i].getName()
                                + ", on slot = "
                                + varInfos[i].getSlot()
                                + ", with unexpected signature = "
                                + varInfos[i].getSignature()
                                + " instead of signature = " + varSignatures[index]);
                success = false;
            }
            if (!varNames[index].equals(varInfos[i].getName())) {
                logWriter.printError("Unexpected name of variable  "
                        + varInfos[i].getName() + ", on slot = "
                        + varInfos[i].getSlot() + " instead of name = "
                        + varNames[index]);
                success = false;
            }

            logWriter.println("");
        }
        return success;
    }
}
