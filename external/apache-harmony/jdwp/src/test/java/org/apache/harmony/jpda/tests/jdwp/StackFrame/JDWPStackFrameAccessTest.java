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

import org.apache.harmony.jpda.tests.framework.TestErrorException;
import org.apache.harmony.jpda.tests.framework.jdwp.CommandPacket;
import org.apache.harmony.jpda.tests.framework.jdwp.JDWPCommands;
import org.apache.harmony.jpda.tests.framework.jdwp.JDWPConstants;
import org.apache.harmony.jpda.tests.framework.jdwp.ReplyPacket;
import org.apache.harmony.jpda.tests.framework.jdwp.Value;
import org.apache.harmony.jpda.tests.share.JPDADebuggeeSynchronizer;

import java.util.ArrayList;
import java.util.List;

/**
 * Base class for testing StackFrame.GetValues and StackFrame.SetValues commands.
 */
public class JDWPStackFrameAccessTest extends JDWPStackFrameTestCase {
    @Override
    protected final String getDebuggeeClassName() {
        return StackTrace002Debuggee.class.getName();
    }

    static class VariableInfo {
        /**
         * The name of the local variable to test.
         */
        private final String variableName;

        /**
         * The initial value of the tested local variable, before we change its value.
         */
        private final Value valueOnFirstSuspension;

        /**
         * The new value of the tested local variable that we set during the test.
         */
        private final Value valueToSet;

        /**
         * The new value of the tested local variable, after change its value.
         */
        private final Value valueOnSecondSuspension;

        VariableInfo(String variableName, Value initialValue, Value setValue, Value newValue) {
            this.variableName = variableName;
            this.valueOnFirstSuspension = initialValue;
            this.valueToSet = setValue;
            this.valueOnSecondSuspension = newValue;
        }

        public String getVariableName() {
            return variableName;
        }

        public Value getValueOnFirstSuspension() {
            return valueOnFirstSuspension;
        }

        public Value getValueToSet() {
            return valueToSet;
        }

        public Value getValueOnSecondSuspension() {
            return valueOnSecondSuspension;
        }
    }

    static class MethodInfo {
        private final String methodName;
        private final List<VariableInfo> variables = new ArrayList<VariableInfo>();

        MethodInfo(String methodName) {
            this.methodName = methodName;
        }

        public String getMethodName() {
            return methodName;
        }

        public List<? extends VariableInfo> getVariables() {
            return variables;
        }

        public void addVariable(String variableName, Value initialValue, Value setValue,
                                Value newValue) {
            variables.add(new VariableInfo(variableName, initialValue, setValue, newValue));
        }

        public void addVariable(String variableName, Value initialValue, Value setValue) {
            Value newValue;
            if (setValue == null) {
                // We test GetValue so we expect to read the same value on second suspension.
                newValue = initialValue;
            } else {
                // We test SetValue so we expect to read the new value on second suspension.
                newValue = setValue;
            }
            variables.add(new VariableInfo(variableName, initialValue, setValue, newValue));
        }

        public void addVariable(String variableName, Value initialValue) {
            addVariable(variableName, initialValue, null);
        }
    }

    static class StackFrameTester {
        private final String signalValue;
        private final List<MethodInfo> testedMethods = new ArrayList<MethodInfo>();

        public StackFrameTester(String signalValue) {
            this.signalValue = signalValue;
        }

        public String getSignalValue() {
            return signalValue;
        }

        public List<? extends MethodInfo> getTestedMethods() {
            return testedMethods;
        }

        public MethodInfo addTestMethod(String methodName) {
            MethodInfo methodInfo = new MethodInfo(methodName);
            testedMethods.add(methodInfo);
            return methodInfo;
        }
    }

    @Override
    protected void internalSetUp() throws Exception {
        super.internalSetUp();

        printTestLog("STARTED");

        // Wait for debuggee to start.
        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_READY);
    }

    @Override
    protected void internalTearDown() {
        printTestLog("FINISHED");

        super.internalTearDown();
    }

    protected void runStackFrameTest(StackFrameTester tester, MethodInfo suspensionMethodInfo) {
        // Get variable information.
        long classID = getClassIDBySignature(getDebuggeeClassSignature());

        // Signal debuggee with a custom message to execute the right method.
        synchronizer.sendMessage(tester.getSignalValue());

        // Wait for 1st suspension.
        long eventThreadID = suspendDebuggee();

        // Check every local variable of every method.
        checkStackFrame(classID, eventThreadID, tester, suspensionMethodInfo, true);

        // Resume debuggee.
        resumeTest(eventThreadID);

        // Wait for 2nd suspension.
        eventThreadID = suspendDebuggee();

        // Check every local variable of every method.
        checkStackFrame(classID, eventThreadID, tester, suspensionMethodInfo, false);

        resumeTest(eventThreadID);
    }

    private long suspendDebuggee() {
        String threadName = synchronizer.receiveMessage();
        long threadId = getThreadIdFromName(threadName);
        // We need the thread to be suspended so we suspend it explicitly now.
        debuggeeWrapper.vmMirror.suspendThread(threadId);
        return threadId;
    }

    private void resumeTest(long threadId) {
        // We suspended the thread so let's resume it before sending the signal.
        debuggeeWrapper.vmMirror.resumeThread(threadId);
        synchronizer.sendMessage(JPDADebuggeeSynchronizer.SGNL_CONTINUE);
    }

    private long getThreadIdFromName(String threadName) {
        long[] allThreadIds = jdwpGetAllThreads();
        for (long threadId : allThreadIds) {
            String currentThreadName = jdwpGetThreadName(threadId);
            if (threadName.equals(currentThreadName)) {
                return threadId;
            }
        }
        throw new TestErrorException("Could not find thread id of thread \"" + threadName + "\"");
    }

    /**
     * Checks the stack frame contains the values we expect during the test.
     *
     * @param classID
     *          the debuggee class ID
     * @param eventThreadID
     *          the thread ID of the event thread
     * @param tester
     *          an instance holding test logic
     * @param suspensionMethodInfo
     *          an instance providing the local variables (and their value) for the method
     *          used for suspension. This is a special method because we do not update variables,
     *          we only suspend the current thread.
     * @param firstSuspension
     *          true if the execution is suspended by the first breakpoint, false otherwise.
     */
    private void checkStackFrame(long classID, long eventThreadID, StackFrameTester tester,
                                 MethodInfo suspensionMethodInfo, boolean firstSuspension) {
        for (MethodInfo methodInfo : tester.getTestedMethods()) {
            String testMethodName = methodInfo.getMethodName();
            long testMethodID = getMethodID(classID, testMethodName);
            assertTrue("No method " + testMethodName, testMethodID != -1);
            boolean isSuspensionMethod = (methodInfo == suspensionMethodInfo);

            VarInfo[] variables = jdwpGetVariableTable(classID, testMethodID);
            assertNotNull("No variable table for method " + testMethodName, variables);

            // Find the frame for the tested method.
            FrameInfo testMethodFrame = getFrameInfo(eventThreadID, classID, testMethodID);
            assertNotNull("Cannot find frame for method " + testMethodName, testMethodFrame);
            logWriter.println("Found frame for " + testMethodName + ": " + testMethodFrame.frameID);

            // Test all variables.
            List<? extends VariableInfo> testedVariables = methodInfo.getVariables();
            assertTrue("Not enough variables in variable table",
                    variables.length >= testedVariables.size());
            for (VariableInfo variableInfo : testedVariables) {
                String variableName = variableInfo.getVariableName();

                VarInfo testVarInfo = getVariableInfo(variables, variableName);
                assertNotNull("No variable info for \"" + variableName + "\"", testVarInfo);

                logWriter.println("Checking value for variable \"" + variableName + "\"");

                // Check the current variable value is correct.
                Value expectedValue;
                if (firstSuspension) {
                    expectedValue = variableInfo.getValueOnFirstSuspension();
                } else {
                    expectedValue = variableInfo.getValueOnSecondSuspension();
                }
                logWriter.println("Check variable \"" + variableName + "\" contains value \"" + expectedValue + "\"");
                Value actual = getVariableValue(eventThreadID, testMethodFrame.frameID,
                        testVarInfo.getSlot(), expectedValue.getTag());
                assertNotNull("No value for variable \"" + variableName + "\"", actual);
                assertEquals("Incorrect value variable \"" + variableName + "\"",
                        expectedValue, actual);

                Value newValue = variableInfo.getValueToSet();
                if (firstSuspension && newValue != null && !isSuspensionMethod) {
                    // Sets the new value in the tested variable.
                    setVariableValue(eventThreadID, testMethodFrame.frameID,
                                     testVarInfo.getSlot(), newValue);

                    // Checks the variable has been properly set.
                    actual = getVariableValue(eventThreadID, testMethodFrame.frameID,
                            testVarInfo.getSlot(), newValue.getTag());
                    assertNotNull("No value for variable \"" + variableName + "\"", actual);
                    assertEquals("Failed to set variable \"" + variableName + "\"",
                            newValue, actual);
                }
            }
        }
    }

    /**
     * Base class for checking stack frame operations.
     */
    static abstract class StackFrameChecker {
        static class VariableNameAndTag {
            public VariableNameAndTag(String testVariableName, byte testVariableJdwpTag) {
                this.testVariableName = testVariableName;
                this.testVariableJdwpTag = testVariableJdwpTag;
            }

            /**
             * Returns the name of the variable (in the tested method) for which
             * we want to check the value.
             */
            public String getTestVariableName() {
                return testVariableName;
            }

            /**
             * Returns the JDWP tag of the tested variable. This matches the
             * declared type of the variable in the tested method.
             *
             * Note: it can be different from the value's tag we retrieve from
             * the stack in the case of Object variable (like String).
             */
            public byte getTestVariableJdwpTag() {
                return testVariableJdwpTag;
            }

            private final String testVariableName;
            private final byte testVariableJdwpTag;

        }

        protected StackFrameChecker(String breakpointMethodName, String testMethodName,
                String testVariableName, byte testVariableJdwpTag) {
            this(breakpointMethodName, testMethodName);
            addTestedVariable(testVariableName, testVariableJdwpTag);
        }

        protected StackFrameChecker(String breakpointMethodName, String testMethodName) {
            this.breakpointMethodName = breakpointMethodName;
            this.testMethodName = testMethodName;
            this.testedVariables = new ArrayList<VariableNameAndTag>();
        }

        /**
         * Returns the name of the method where a breakpoint is installed
         * to suspend the debuggee.
         */
        public String getBreakpointMethodName() {
            return breakpointMethodName;
        }

        /**
         * Returns the name of the method calling the "breakpoint" method.
         */
        public String getTestMethodName() {
            return testMethodName;
        }

        public int addTestedVariable(String name, byte tag) {
            testedVariables.add(new VariableNameAndTag(name, tag));
            return testedVariables.size() - 1;
        }

        public List<? extends VariableNameAndTag> getTestedVariables() {
            return testedVariables;
        }

        protected String getTestVariableName(int idx) {
            return getTestedVariables().get(idx).getTestVariableName();
        }

        private final String breakpointMethodName;
        private final String testMethodName;
        private final List<VariableNameAndTag> testedVariables;
    }

    /**
     * Returns the {@link VarInfo} of the given variable in the given method.
     *
     * @param classID
     *          the ID of the declaring class of the method
     * @param methodID
     *          the ID of the method
     * @param variableName
     *          the name of the variable we look for
     */
    protected VarInfo getVariableInfo(long classID, long methodID, String variableName) {
        VarInfo[] variables = jdwpGetVariableTable(classID, methodID);
        return getVariableInfo(variables, variableName);
    }

    protected VarInfo getVariableInfo(VarInfo[] variables, String variableName) {
        for (VarInfo variable : variables) {
            if (variable.name.equals(variableName)) {
                return variable;
            }
        }
        return null;
    }

    /**
     * Returns the {@link FrameInfo} of the most recent frame matching the given method.
     *
     * @param threadID
     *          the ID of the thread where to look for the frame
     * @param classID
     *          the ID of the declaring class of the method
     * @param methodID
     *          the ID of the method
     */
    protected FrameInfo getFrameInfo(long threadID, long classID, long methodID) {
        int frameCount = jdwpGetFrameCount(threadID);

        // There should be at least 2 frames: the breakpoint method and its caller.
        assertTrue("Not enough frames", frameCount > 2);

        FrameInfo[] frames = jdwpGetFrames(threadID, 0, frameCount);
        for (FrameInfo frameInfo : frames) {
            if (frameInfo.location.classID == classID &&
                    frameInfo.location.methodID == methodID) {
                return frameInfo;
            }
        }
        return null;
    }

    /**
     * Returns the value of a local variable in the stack.
     *
     * @param threadID
     *          the ID of the thread of the stack
     * @param frameID
     *          the ID of the frame of the stack
     * @param slot
     *          the slot of the variable in the stack
     * @param tag
     *          the type of the value
     */
    protected Value getVariableValue(long threadID, long frameID, int slot, byte tag) {
        logWriter.println(" Send StackFrame::GetValues: threadID=" + threadID +
                          ", frameID=" + frameID + ", slot=" + slot +
                          ", tag=" + JDWPConstants.Tag.getName(tag));

        // Send StackFrame::GetValues command.
        CommandPacket packet = new CommandPacket(
                JDWPCommands.StackFrameCommandSet.CommandSetID,
                JDWPCommands.StackFrameCommandSet.GetValuesCommand);
        packet.setNextValueAsThreadID(threadID);
        packet.setNextValueAsFrameID(frameID);
        packet.setNextValueAsInt(1);
        packet.setNextValueAsInt(slot);
        packet.setNextValueAsByte(tag);

        // Check reply has no error.
        ReplyPacket reply = debuggeeWrapper.vmMirror.performCommand(packet);
        checkReplyPacket(reply, "StackFrame::GetValues command");

        // Check we have 1 value.
        int numberOfValues = reply.getNextValueAsInt();
        assertEquals("Incorrect number of values", 1, numberOfValues);

        // Check the value tag is correct.
        Value value = reply.getNextValueAsValue();
        logWriter.println(" Received value " + value);
        assertEquals("Invalid value tag", tag, value.getTag());

        assertAllDataRead(reply);
        return value;
    }

    /**
     * Sets the value of a local variable in the stack.
     *
     * @param threadID
     *          the ID of the thread of the stack
     * @param frameID
     *          the ID of the frame of the stack
     * @param slot
     *          the slot of the variable in the stack
     * @param newValue
     *          the new value to set
     */
    protected void setVariableValue(long threadID, long frameID, int slot, Value newValue) {
        logWriter.println(" Send StackFrame::SetValues: threadID=" + threadID +
                          ", frameID=" + frameID + ", slot=" + slot +
                          ", value=" + newValue);

        // Send StackFrame::SetValues command.
        CommandPacket packet = new CommandPacket(JDWPCommands.StackFrameCommandSet.CommandSetID,
                                                 JDWPCommands.StackFrameCommandSet.SetValuesCommand);
        packet.setNextValueAsThreadID(threadID);
        packet.setNextValueAsFrameID(frameID);
        packet.setNextValueAsInt(1);
        packet.setNextValueAsInt(slot);
        packet.setNextValueAsValue(newValue);

        // Check reply has no error.
        ReplyPacket reply = debuggeeWrapper.vmMirror.performCommand(packet);
        checkReplyPacket(reply, "StackFrame::SetValues command");
    }

    /**
     * Returns the value of the static field identified by the given name in
     * the given class.
     *
     * @param classID
     *          the ID of the declaring class of the static field
     * @param fieldName
     *          the name of the static field in the class.
     */
    protected Value getStaticFieldValue(long classID, String fieldName) {
        long fieldID = debuggeeWrapper.vmMirror.getFieldID(classID, fieldName);
        long[] fieldIDs = new long[]{ fieldID };
        Value[] fieldValues = debuggeeWrapper.vmMirror.getReferenceTypeValues(classID, fieldIDs);
        assertNotNull("No field values", fieldValues);
        assertEquals("Invalid field values count", fieldValues.length, 1);
        return fieldValues[0];
    }
}
