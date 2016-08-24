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

package org.apache.harmony.jpda.tests.jdwp.InterfaceType;

import org.apache.harmony.jpda.tests.framework.LogWriter;
import org.apache.harmony.jpda.tests.share.JPDADebuggeeSynchronizer;
import org.apache.harmony.jpda.tests.share.JPDATestOptions;
import org.apache.harmony.jpda.tests.share.SyncDebuggee;

/**
 * Used for InterfaceType.InvokeMethodTest.testInvokeMethodStatic
 */
public class InvokeMethodDebuggee extends SyncDebuggee implements InvokeMethodTestInterface {

    void execMethod() {
        logWriter.println("InvokeMethodDebuggee.execMethod()");
    }

    @Override
    public void run() {
        synchronizer.sendMessage(JPDADebuggeeSynchronizer.SGNL_READY);
        logWriter.println("InvokeMethodDebuggee");
        synchronizer.receiveMessageWithoutException(
            "org.apache.harmony.jpda.tests.jdwp.InterfaceType.InvokeMethodDebuggee(#1)");
        execMethod();
        synchronizer.receiveMessageWithoutException(
            "org.apache.harmony.jpda.tests.jdwp.InterfaceType.InvokeMethodDebuggee(#2)");
    }
    public static void main(String[] args) {
        runDebuggee(InvokeMethodDebuggee.class);
    }
}
