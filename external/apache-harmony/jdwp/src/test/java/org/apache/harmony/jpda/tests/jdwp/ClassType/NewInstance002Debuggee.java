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

import org.apache.harmony.jpda.tests.framework.LogWriter;
import org.apache.harmony.jpda.tests.share.JPDADebuggeeSynchronizer;
import org.apache.harmony.jpda.tests.share.SyncDebuggee;

public class NewInstance002Debuggee extends SyncDebuggee {

    static class TestClass {
        // Field used to check correct execution of constructor.
        private final int checkField;

        // Constructor invoked from JDWP ClassType.NewInstance command.
        public TestClass(Object obj) {
            if (obj == null) {
              checkField = 123;
            } else {
              checkField = 456;
            }
        }
    }

    void execMethod() {
        logWriter.println("NewInstance002Debuggee.execMethod()");
    }

    public void run() {
        // Preload TestClass so it is available during the test.
        Class c = null;
        try {
            c = Class.forName("org.apache.harmony.jpda.tests.jdwp.ClassType.NewInstance002Debuggee$TestClass");
        } catch (ClassNotFoundException e) {
            e.printStackTrace();
        }
        synchronizer.sendMessage(JPDADebuggeeSynchronizer.SGNL_READY);
        logWriter.println("InvokeMethod002Debuggee");
        synchronizer.receiveMessageWithoutException("org.apache.harmony.jpda.tests.jdwp.ClassType.NewInstance002Debuggee(#1)");
        execMethod();
        synchronizer.receiveMessageWithoutException("org.apache.harmony.jpda.tests.jdwp.ClassType.NewInstance002Debuggee(#2)");
    }

    public static void main(String[] args) {
        runDebuggee(NewInstance002Debuggee.class);
    }
}

