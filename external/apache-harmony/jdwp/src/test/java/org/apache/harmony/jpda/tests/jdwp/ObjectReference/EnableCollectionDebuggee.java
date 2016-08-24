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
 * @author Anatoly F. Bondarenko
 */

/**
 * Created on 04.03.2005
 */
package org.apache.harmony.jpda.tests.jdwp.ObjectReference;

import org.apache.harmony.jpda.tests.share.JPDADebuggeeSynchronizer;
import org.apache.harmony.jpda.tests.share.SyncDebuggee;

public class EnableCollectionDebuggee extends SyncDebuggee {
    
    static EnableCollectionObject001_01 checkedObject;
    static boolean checkedObject_Finalized = false;
    static EnableCollectionObject001_02 patternObject;
    static boolean patternObject_Finalized = false;

    public void run() {
        logWriter.println("--> Debuggee: EnableCollectionDebuggee: START");
        
        // Allocates test objects to be sure there is no local reference
        // to them.
        allocateTestObjects();

        synchronizer.sendMessage(JPDADebuggeeSynchronizer.SGNL_READY);
        String messageFromTest = synchronizer.receiveMessage();
        if ( messageFromTest.equals("TO_FINISH")) {
            logWriter.println("--> Debuggee: EnableCollectionDebuggee: FINISH");
            return;
        }
        
        // Releases test objects so there is no more reference to them. We do it in a
        // separate method to avoid having any local reference to them.
        releaseTestObjects();

        // Allocates many objects to increase the heap.
        causeMemoryDepletion();

        // Requests GC and finalization of objects.
        System.gc();
        System.runFinalization();
        System.gc();

        logWriter.println("--> Debuggee: AFTER System.gc():");
        logWriter.println("--> Debuggee: checkedObject = " + checkedObject);
        logWriter.println("--> Debuggee: checkedObject_UNLOADed = " + checkedObject_Finalized);
        logWriter.println("--> Debuggee: patternObject = " + patternObject);
        logWriter.println("--> Debuggee: patternObject_UNLOADed = " + patternObject_Finalized);

        String messageForTest = buildMessage();
        logWriter.println("--> Debuggee: Send to test message: \"" + messageForTest + "\"");
        synchronizer.sendMessage(messageForTest);
        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_CONTINUE);

        logWriter.println("--> Debuggee: EnableCollectionDebuggee: FINISH");
    }

    private void allocateTestObjects() {
        checkedObject = new EnableCollectionObject001_01();
        patternObject = new EnableCollectionObject001_02();
    }

    private void releaseTestObjects() {
        logWriter.println("--> Debuggee: BEFORE System.gc():");
        logWriter.println("--> Debuggee: checkedObject = " + checkedObject);
        logWriter.println("--> Debuggee: checkedObject_UNLOADed = " + checkedObject_Finalized);
        logWriter.println("--> Debuggee: patternObject = " + patternObject);
        logWriter.println("--> Debuggee: patternObject_UNLOADed = " + patternObject_Finalized);

        checkedObject = null;
        patternObject = null;
    }

    private void causeMemoryDepletion() {
        long[][] longArray;
        try {
            longArray = new long[1000000][];
            int arraysNumberLimit = 8; // max - longArray.length
            logWriter.println
            ("--> Debuggee: memory depletion - creating 'long[1000000]' arrays (" + arraysNumberLimit + ")...");
            for (int i = 0; i < arraysNumberLimit; i++) {
                longArray[i] = new long[1000000];
            }
        } catch (OutOfMemoryError outOfMem) {
            logWriter.println("--> Debuggee: OutOfMemoryError!!!");
        }
    }

    private String buildMessage() {
        StringBuilder messageBuilder = new StringBuilder();
        messageBuilder.append("Checked Object is ");
        if (checkedObject_Finalized) {
            messageBuilder.append("UNLOADed");
        } else {
            messageBuilder.append("NOT UNLOADed");
        }
        messageBuilder.append("; Pattern Object is ");
        if (patternObject_Finalized) {
            messageBuilder.append("UNLOADed");
        } else {
            messageBuilder.append("NOT UNLOADed");
        }
        messageBuilder.append(";");
        return messageBuilder.toString();
    }

    public static void main(String [] args) {
        runDebuggee(EnableCollectionDebuggee.class);
    }

}

class EnableCollectionObject001_01 {
    protected void finalize() throws Throwable {
        EnableCollectionDebuggee.checkedObject_Finalized = true;
        super.finalize();
    }
}   

class EnableCollectionObject001_02 {
    protected void finalize() throws Throwable {
        EnableCollectionDebuggee.patternObject_Finalized = true;
        super.finalize();
    }
}   

