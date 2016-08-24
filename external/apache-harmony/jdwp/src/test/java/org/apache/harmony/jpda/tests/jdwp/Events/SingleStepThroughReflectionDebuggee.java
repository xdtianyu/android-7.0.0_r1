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

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;

/**
 * Debuggee for SingleStepThroughReflectionTest unit test.
 */
public class SingleStepThroughReflectionDebuggee extends SyncDebuggee {

    public static void main(String[] args) {
        runDebuggee(SingleStepThroughReflectionDebuggee.class);
    }

    public static void breakpointTest(Method m, Object receiver)
            throws IllegalAccessException, IllegalArgumentException,
            InvocationTargetException {
        m.invoke(receiver);  // method is void: ignore result.
    }

    public void methodCalledThroughReflection() {
        logWriter.println("methodCalledThroughReflection");
    }

    public void run() {
        synchronizer.sendMessage(JPDADebuggeeSynchronizer.SGNL_READY);

        logWriter.println("SingleStepThroughReflectionDebuggee started");

        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_CONTINUE);

        try {
            Method m = getMethod("methodCalledThroughReflection");
            System.out.println("Invoking through reflection");
            breakpointTest(m, this);
            System.out.println("Returned from reflection");
        } catch (RuntimeException e) {
            throw e;
        } catch (Exception e) {
            // Wrap exception into a RuntimeException.
            throw new RuntimeException(e);
        }

        logWriter.println("SingleStepThroughReflectionDebuggee finished");
    }

    private static Method getMethod(String name) throws NoSuchMethodException {
      return SingleStepThroughReflectionDebuggee.class.getDeclaredMethod(name);
    }

}
