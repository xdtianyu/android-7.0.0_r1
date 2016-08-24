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

/**
 * @author Anton V. Karnachuk
 */

/**
 * Created on 11.04.2005
 */
package org.apache.harmony.jpda.tests.jdwp.Events;

import org.apache.harmony.jpda.tests.share.JPDADebuggeeSynchronizer;
import org.apache.harmony.jpda.tests.share.SyncDebuggee;

/**
 * Debuggee for ExceptionUncaughtTest unit test.
 * Generates uncaught DebuggeeException exception.
 */
public class ExceptionUncaughtDebuggee extends SyncDebuggee {

    public static void main(String[] args) {
        runDebuggee(ExceptionUncaughtDebuggee.class);
    }

    public void run() {
        logWriter.println("-- ExceptionUncaughtDebuggee: STARTED");

        // Cause loading of DebuggeeException so it is visible from the test.
        new DebuggeeException("dummy exception");

        synchronizer.sendMessage(JPDADebuggeeSynchronizer.SGNL_READY);

        logWriter.println("-- ExceptionUncaughtDebuggee: Wait for SGNL_CONTINUE...");
        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_CONTINUE);
        logWriter.println("-- ExceptionUncaughtDebuggee: SGNL_CONTINUE has been received!");

        Thread t = new Thread(new Runnable() {
            public void run() {
                // Catch a different exception to test catch location is properly reported.
                try {
                    throwDebuggeeException();
                } catch (NullPointerException e) {
                    // Unexpect exception
                    logWriter.printError("Unexpected exception", e);
                }
            }
        });
        t.start();

        try {
            t.join();
        } catch (InterruptedException e) {
            logWriter.printError(e);
        }

        logWriter.println("-- ExceptionUncaughtDebuggee: FINISHing...");
    }

    public static void throwDebuggeeException() {
        throw new DebuggeeException("Uncaught debuggee exception");
    }
}
