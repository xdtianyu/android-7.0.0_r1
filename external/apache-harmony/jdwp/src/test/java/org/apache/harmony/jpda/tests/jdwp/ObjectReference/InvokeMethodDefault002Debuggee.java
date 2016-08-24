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

package org.apache.harmony.jpda.tests.jdwp.ObjectReference;

import org.apache.harmony.jpda.tests.share.JPDADebuggeeSynchronizer;
import org.apache.harmony.jpda.tests.share.SyncDebuggee;

public class InvokeMethodDefault002Debuggee extends SyncDebuggee {

    /**
     * Interface defining the default method to be invoked.
     */
    static interface TestInterface {
        public static final int RETURN_VALUE = 123;
        /**
         * Method invoked for test
         * @param shouldThrow throws if the value is true.
         * @return the value 123
         */
        public default int testDefaultMethod(boolean shouldThrow) throws Throwable {
            if (shouldThrow) {
                throw new Throwable("testDefaultMethod");
            }
            return TestInterface.RETURN_VALUE;
        }
    }

    /**
     * The overriding class we will initialize to call the default method.
     */
    static class TestClass implements TestInterface {
        public static final int RETURN_VALUE = 456;
        @Override
        public int testDefaultMethod(boolean shouldThrow) throws Throwable {
            if (shouldThrow) {
                throw new Throwable("testDefaultMethod");
            }
            return TestClass.RETURN_VALUE;
        }
    }

    // The instance used to invoke "TestClass.testMethod".
    static TestClass invokeReceiver = new TestClass();

    void execMethod() {
        logWriter.println("InvokeMethodDefault002Debuggee.execMethod()");
    }

    public void run() {
        // Preload TestClass so it is available during the test.
        Class c = null;
        String packageName = "org.apache.harmony.jpda.tests.jdwp.";
        try {
            c = Class.forName(
                    packageName + "ObjectReference.InvokeMethodDefault002Debuggee$TestClass");
        } catch (ClassNotFoundException e) {
            e.printStackTrace();
        }
        synchronizer.sendMessage(JPDADebuggeeSynchronizer.SGNL_READY);
        logWriter.println("InvokeMethodDefault002Debuggee");
        synchronizer.receiveMessageWithoutException(
                packageName + "ObjectReference.InvokeMethodDefault002Debuggee(#1)");
        execMethod();
        synchronizer.receiveMessageWithoutException(
                packageName + "ObjectReference.InvokeMethodDefault002Debuggee(#2)");
    }

    public static void main(String[] args) {
        runDebuggee(InvokeMethodDefault002Debuggee.class);
    }
}

