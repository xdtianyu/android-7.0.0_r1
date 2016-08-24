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

package org.apache.harmony.jpda.tests.jdwp.EventModifiers;

import org.apache.harmony.jpda.tests.share.JPDADebuggeeSynchronizer;
import org.apache.harmony.jpda.tests.share.SyncDebuggee;

/**
 * Debuggee for InstanceOnlyModifierTest.
 */
public class InstanceOnlyModifierDebuggee extends SyncDebuggee {
    static class TestException extends Exception {
    }

    static class TestClass {
        int watchedField = 0;

        public void eventTestMethod() {
            System.out.println(
                    "InstanceOnlyModifierDebuggee.TestClass.eventTestMethod()");
        }

        public void throwAndCatchException() {
            try {
                throwException();
            } catch (TestException e) {
                System.out.println("Catch TestException");
            }
        }

        public void readAndWriteField() {
            System.out.println(
                    "InstanceOnlyModifierDebuggee.TestClass.readAndWriteField()");
            watchedField = watchedField + 1;
        }

        private void throwException() throws TestException {
            System.out.println(
                    "InstanceOnlyModifierDebuggee.TestClass.throwException()");
            throw new TestException();
        }
    }

    static TestClass INSTANCE_ONLY;

    @Override
    public void run() {
        new TestException();  // force class loading.

        logWriter.println("Create instances");
        final TestClass[] instances = new TestClass[10];
        for (int i = 0; i < instances.length; ++i) {
            instances[i] = new TestClass();
        }

        System.out.println("Set INSTANCE_ONLY to the last instance");
        INSTANCE_ONLY = instances[instances.length - 1];
        System.out.println("Set INSTANCE_ONLY to the last instance");

        synchronizer.sendMessage(JPDADebuggeeSynchronizer.SGNL_READY);

        logWriter.println("InstanceOnlyModifierDebuggee started");

        // Wait for test setup.
        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_CONTINUE);

        System.out.println("Fire breakpoint, method entry/exit events");
        for (int i = 0; i < instances.length; ++i) {
            instances[i].eventTestMethod();
        }

        System.out.println("Fire caught exception events");
        for (int i = 0; i < instances.length; ++i) {
            instances[i].throwAndCatchException();
        }

        System.out.println("Fire field access/modification events");
        for (int i = 0; i < instances.length; ++i) {
            instances[i].readAndWriteField();
        }

        logWriter.println("InstanceOnlyModifierDebuggee finished");
    }

    public static void main(String[] args) {
        runDebuggee(InstanceOnlyModifierDebuggee.class);
    }

}
