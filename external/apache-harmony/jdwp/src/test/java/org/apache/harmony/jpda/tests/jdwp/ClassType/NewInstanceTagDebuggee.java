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

import org.apache.harmony.jpda.tests.share.JPDADebuggeeSynchronizer;
import org.apache.harmony.jpda.tests.share.SyncDebuggee;

import java.nio.charset.Charset;

public class NewInstanceTagDebuggee extends SyncDebuggee {
    static class MyObject extends Object {
    }

    static class MyClassLoader extends ClassLoader {
    }

    static class MyThread extends Thread {
    }

    static class MyThreadGroup extends ThreadGroup {
        public MyThreadGroup() {
            super("test");
        }
    }

    @Override
    public void run() {
        // Preload tested subclasses.
        new MyObject();
        new MyClassLoader();
        new MyThread();
        new MyThreadGroup();

        logWriter.println("NewInstanceTagDebuggee starts");

        synchronizer.sendMessage(JPDADebuggeeSynchronizer.SGNL_READY);
        // ... Let debugger prepare breakpoint request ...
        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_CONTINUE);

        // Suspend the debuggee on breakpoint so the test can send ClassType.NewInstance command.
        breakpointMethod();

        logWriter.println("NewInstanceTagDebuggee ends");
    }

    void breakpointMethod() {
        logWriter.println("NewInstanceTagDebuggee.breakpointMethod()");
    }

    public static void main(String[] args) {
        runDebuggee(NewInstanceTagDebuggee.class);
    }
}
