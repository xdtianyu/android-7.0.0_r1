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

public class NewInstanceStringDebuggee extends SyncDebuggee {
    // Fields used as constructor arguments during the test.
    public static final String TEST_STRING = "my test string";
    public static final byte[] BYTE_ARRAY = TEST_STRING.getBytes();
    public static final char[] CHAR_ARRAY = new char[TEST_STRING.length()];
    public static final int[] INT_ARRAY = new int[TEST_STRING.length()];
    public static final Charset CHARSET = Charset.defaultCharset();
    public static final String STRING_CHARSET = CHARSET.displayName();
    public static final StringBuffer STRING_BUFFER = new StringBuffer(TEST_STRING);
    public static final StringBuilder STRING_BUILDER = new StringBuilder(TEST_STRING);

    static {
        TEST_STRING.getChars(0, TEST_STRING.length(), CHAR_ARRAY, 0);
        for (int i = 0; i < INT_ARRAY.length; ++i) {
            INT_ARRAY[i] = TEST_STRING.codePointAt(i);
        }
    }

    @Override
    public void run() {
        logWriter.println("NewInstanceStringDebuggee starts");

        synchronizer.sendMessage(JPDADebuggeeSynchronizer.SGNL_READY);
        // ... Let debugger prepare breakpoint request ...
        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_CONTINUE);

        // Suspend the debuggee on breakpoint so the test can send ClassType.NewInstance command.
        breakpointMethod();

        logWriter.println("NewInstanceStringDebuggee ends");
    }

    void breakpointMethod() {
        logWriter.println("NewInstanceStringDebuggee.breakpointMethod()");
    }

    public static void main(String[] args) {
        runDebuggee(NewInstanceStringDebuggee.class);
    }
}
