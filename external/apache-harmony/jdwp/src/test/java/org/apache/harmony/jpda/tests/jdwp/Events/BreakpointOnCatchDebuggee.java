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

import org.apache.harmony.jpda.tests.share.JPDADebuggeeSynchronizer;
import org.apache.harmony.jpda.tests.share.SyncDebuggee;

/**
 * Debuggee for BreakpointOnCatchTest unit test.
 */
public class BreakpointOnCatchDebuggee extends SyncDebuggee {
  // This variable must be set to the name of the method where we set a breakpoint.
  public static String BREAKPOINT_METHOD_NAME = "breakpointOnCatch";

  public static class BreakpointOnCatchDebuggeeException extends Exception {
    public BreakpointOnCatchDebuggeeException(String detailMessage) {
      super(detailMessage);
    }
  }

    @Override
    public void run() {
        // Force loading of exception class.
        new BreakpointOnCatchDebuggeeException("");

        synchronizer.sendMessage(JPDADebuggeeSynchronizer.SGNL_READY);

        logWriter.println("BreakpointOnCatchDebuggee started");

        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_CONTINUE);

        // First run to know the catch location.
        logWriter.println("Execute breakpointOnCatch for EXCEPTION event");
        breakpointOnCatch();

        // Second run to test the breakpoint on a catch location.
        logWriter.println("Execute breakpointOnCatch for BREAKPOINT event");
        breakpointOnCatch();

        logWriter.println("BreakpointOnCatchDebuggee finished");
    }

    private void breakpointOnCatch() {
      try {
        throw new BreakpointOnCatchDebuggeeException("Expected exception");
      } catch (BreakpointOnCatchDebuggeeException e) {
        logWriter.printError("Caught the expected exception", e);
      }
    }

    public static void main(String[] args) {
        runDebuggee(BreakpointOnCatchDebuggee.class);
    }

}
