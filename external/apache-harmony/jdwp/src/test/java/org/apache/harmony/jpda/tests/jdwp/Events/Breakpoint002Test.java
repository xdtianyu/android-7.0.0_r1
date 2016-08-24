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
import org.apache.harmony.jpda.tests.framework.jdwp.ParsedEvent.EventThread;
import org.apache.harmony.jpda.tests.share.JPDADebuggeeSynchronizer;

/**
 * JDWP Unit test for BREAKPOINT event in methods possibly inlined.
 */
public class Breakpoint002Test extends JDWPEventTestCase {
    protected String getDebuggeeClassName() {
        return Breakpoint002Debuggee.class.getName();
    }

    /**
     * This testcase is for BREAKPOINT event.
     * <BR>It runs Breakpoint002Debuggee and sets breakpoint in the
     * breakpointReturnVoid method, then verifies that requested BREAKPOINT
     * event occurs.
     */
    public void testInlinedReturnVoid() {
        testBreakpointIn("testInlinedReturnVoid", "breakpointReturnVoid");
    }

    /**
     * This testcase is for BREAKPOINT event.
     * <BR>It runs Breakpoint002Debuggee and sets breakpoint in the
     * breakpointReturnIntConst method, then verifies that requested BREAKPOINT
     * event occurs.
     */
    public void testInlinedReturnIntConstant() {
        testBreakpointIn("testInlinedReturnIntConstant",
                         "breakpointReturnIntConst");
    }

    /**
     * This testcase is for BREAKPOINT event.
     * <BR>It runs Breakpoint002Debuggee and sets breakpoint in the
     * breakpointReturnLongConst method, then verifies that requested BREAKPOINT
     * event occurs.
     */
    public void testInlinedReturnLongConstant() {
        testBreakpointIn("testInlinedReturnLongConstant",
                         "breakpointReturnLongConst");
    }

    /**
     * This testcase is for BREAKPOINT event.
     * <BR>It runs Breakpoint002Debuggee and sets breakpoint in the
     * breakpointReturnIntArg method, then verifies that requested BREAKPOINT
     * event occurs.
     */
    public void testInlinedReturnIntArgument() {
        testBreakpointIn("testInlinedReturnIntArgument",
                         "breakpointReturnIntArg");
    }

    /**
     * This testcase is for BREAKPOINT event.
     * <BR>It runs Breakpoint002Debuggee and sets breakpoint in the
     * breakpointReturnLongArg method, then verifies that requested BREAKPOINT
     * event occurs.
     */
    public void testInlinedReturnLongArgument() {
        testBreakpointIn("testInlinedReturnLongArgument",
                         "breakpointReturnLongArg");
    }

    /**
     * This testcase is for BREAKPOINT event.
     * <BR>It runs Breakpoint002Debuggee and sets breakpoint in the
     * breakpointReturnObjectArg method, then verifies that requested BREAKPOINT
     * event occurs.
     */
    public void testInlinedReturnObjectArgument() {
        testBreakpointIn("testInlinedReturnObjectArgument",
                         "breakpointReturnObjectArg");
    }

    /**
     * This testcase is for BREAKPOINT event.
     * <BR>It runs Breakpoint002Debuggee and sets breakpoint in the
     * breakpointIntGetter method, then verifies that requested BREAKPOINT
     * event occurs.
     */
    public void testInlinedIntGetter() {
        testBreakpointIn("testInlinedIntGetter", "breakpointIntGetter");
    }

    /**
     * This testcase is for BREAKPOINT event.
     * <BR>It runs Breakpoint002Debuggee and sets breakpoint in the
     * breakpointLongGetter method, then verifies that requested BREAKPOINT
     * event occurs.
     */
    public void testInlinedLongGetter() {
        testBreakpointIn("testInlinedLongGetter", "breakpointLongGetter");
    }

    /**
     * This testcase is for BREAKPOINT event.
     * <BR>It runs Breakpoint002Debuggee and sets breakpoint in the
     * breakpointObjectGetter method, then verifies that requested BREAKPOINT
     * event occurs.
     */
    public void testInlinedObjectGetter() {
        testBreakpointIn("testInlinedObjectGetter", "breakpointObjectGetter");
    }

    /**
     * This testcase is for BREAKPOINT event.
     * <BR>It runs Breakpoint002Debuggee and sets breakpoint in the
     * breakpointIntSetter method, then verifies that requested BREAKPOINT
     * event occurs.
     */
    public void testInlinedIntSetter() {
        testBreakpointIn("testInlinedIntSetter", "breakpointIntSetter");
    }

    /**
     * This testcase is for BREAKPOINT event.
     * <BR>It runs Breakpoint002Debuggee and sets breakpoint in the
     * breakpointLongSetter method, then verifies that requested BREAKPOINT
     * event occurs.
     */
    public void testInlinedLongSetter() {
        testBreakpointIn("testInlinedLongSetter", "breakpointLongSetter");
    }

    /**
     * This testcase is for BREAKPOINT event.
     * <BR>It runs Breakpoint002Debuggee and sets breakpoint in the
     * breakpointObjectSetter method, then verifies that requested BREAKPOINT
     * event occurs.
     */
    public void testInlinedObjectSetter() {
        testBreakpointIn("testInlinedObjectSetter", "breakpointObjectSetter");
    }

    private void testBreakpointIn(String testName, String methodName) {
      logWriter.println(testName + " started");
      long classID = debuggeeWrapper.vmMirror.getClassID(getDebuggeeClassSignature());
      assertTrue("Failed to find debuggee class", classID != -1);

      synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_READY);
      int breakpointReqID = debuggeeWrapper.vmMirror.setBreakpointAtMethodBegin(classID, methodName);
      assertTrue("Failed to install breakpoint in method " + methodName, breakpointReqID != -1);
      synchronizer.sendMessage(JPDADebuggeeSynchronizer.SGNL_CONTINUE);

      long eventThreadID = debuggeeWrapper.vmMirror.waitForBreakpoint(breakpointReqID);
      checkThreadState(eventThreadID, JDWPConstants.ThreadStatus.RUNNING,
              JDWPConstants.SuspendStatus.SUSPEND_STATUS_SUSPENDED);

      logWriter.println(testName + " done");
    }
}
