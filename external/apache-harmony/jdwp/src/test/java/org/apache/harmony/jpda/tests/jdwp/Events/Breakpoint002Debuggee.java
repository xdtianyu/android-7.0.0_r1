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
 * Debuggee for Breakpoint002Test unit test.
 */
public class Breakpoint002Debuggee extends SyncDebuggee {

    int mIntField = 5;
    long mLongField = 5l;
    Object mObjectField = "this is an object field";

    private void breakpointReturnVoid() {
      return;
    }

    private int breakpointReturnIntConst() {
      return 1;
    }

    private long breakpointReturnLongConst() {
      return 1l;
    }

    private int breakpointReturnIntArg(int arg) {
      return arg;
    }

    private long breakpointReturnLongArg(long arg) {
      return arg;
    }

    private Object breakpointReturnObjectArg(Object arg) {
      return arg;
    }

    private int breakpointIntGetter() {
      return mIntField;
    }

    private long breakpointLongGetter() {
      return mLongField;
    }

    private Object breakpointObjectGetter() {
      return mObjectField;
    }

    private void breakpointIntSetter(int val) {
      mIntField = val;
    }

    private void breakpointLongSetter(long val) {
      mLongField = val;
    }

    private void breakpointObjectSetter(Object val) {
      mObjectField = val;
    }

    private void callSmallMethods() {
      logWriter.println("Calling breakpointReturnVoid");
      breakpointReturnVoid();

      logWriter.println("Calling breakpointReturnIntConst");
      int intConstant = breakpointReturnIntConst();
      logWriter.println("intConstant = " + intConstant);

      logWriter.println("Calling breakpointReturnLongConst");
      long longConstant = breakpointReturnLongConst();
      logWriter.println("longConstant = " + longConstant);

      logWriter.println("Calling breakpointReturnIntArg");
      int intArg = breakpointReturnIntArg(5);
      logWriter.println("intArg = " + intArg);

      logWriter.println("Calling breakpointReturnLongArg");
      long longArg = breakpointReturnLongArg(5l);
      logWriter.println("longArg = " + longArg);

      logWriter.println("Calling breakpointReturnLongArg");
      Object objectArg = breakpointReturnObjectArg("this is a string argument");
      logWriter.println("objectArg = " + objectArg);

      logWriter.println("Calling breakpointIntGetter");
      int intField = breakpointIntGetter();
      logWriter.println("mIntField = " + intField);

      logWriter.println("Calling breakpointLongGetter");
      long longField = breakpointLongGetter();
      logWriter.println("mLongField = " + longField);

      logWriter.println("Calling breakpointObjectGetter");
      Object objectField = breakpointObjectGetter();
      logWriter.println("mObjectField = " + objectField);

      logWriter.println("Calling breakpointIntSetter");
      breakpointIntSetter(10);

      logWriter.println("Calling breakpointLongGetter");
      breakpointLongSetter(10l);

      logWriter.println("Calling breakpointObjectSetter");
      breakpointObjectSetter("this is a new object field value");
    }

    public void run() {
        synchronizer.sendMessage(JPDADebuggeeSynchronizer.SGNL_READY);

        logWriter.println("Breakpoint002Debuggee started");

        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_CONTINUE);

        callSmallMethods();

        logWriter.println("Breakpoint002Debuggee finished");
    }

    public static void main(String[] args) {
        runDebuggee(Breakpoint002Debuggee.class);
    }

}
