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

package org.apache.harmony.jpda.tests.jdwp.ArrayReference;

import org.apache.harmony.jpda.tests.share.JPDADebuggeeSynchronizer;
import org.apache.harmony.jpda.tests.share.SyncDebuggee;

/**
 * Debuggee for JDWP {@link SetValues003Test} unit test which
 * exercises ArrayReference.SetValues command.
 */
public class SetValues003Debuggee extends SyncDebuggee {
    static final int ARRAY_LENGTH = 1;

    static interface DebuggeeInterface {
    }

    static class DebuggeeSuperClass {
    }

    static class DebuggeeSubClass extends DebuggeeSuperClass implements DebuggeeInterface {
    }

    // Defines different types of arrays.
    static DebuggeeSubClass[] arrayField;
    static DebuggeeSuperClass[] superClassArrayField;
    static DebuggeeInterface[] interfaceArrayField;

    // Defines objects of related type.
    static DebuggeeSubClass instanceField;
    static DebuggeeSuperClass superInstanceField;

    @Override
    public void run() {
        logWriter.println("--> SetValues003Debuggee: START");

        // Init fields.
        instanceField = new DebuggeeSubClass();
        superInstanceField = new DebuggeeSuperClass();
        arrayField = new DebuggeeSubClass[ARRAY_LENGTH];
        superClassArrayField = new DebuggeeSuperClass[ARRAY_LENGTH];
        interfaceArrayField = new DebuggeeInterface[ARRAY_LENGTH];

        // Execute the test.
        synchronizer.sendMessage(JPDADebuggeeSynchronizer.SGNL_READY);
        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_CONTINUE);

        logWriter.println("--> SetValues003Debuggee: FINISH");
    }

    public static void main(String [] args) {
        runDebuggee(SetValues003Debuggee.class);
    }
}
