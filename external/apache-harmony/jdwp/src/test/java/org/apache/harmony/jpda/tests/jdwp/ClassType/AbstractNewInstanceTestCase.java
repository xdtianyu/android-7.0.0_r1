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

import org.apache.harmony.jpda.tests.framework.jdwp.CommandPacket;
import org.apache.harmony.jpda.tests.framework.jdwp.JDWPCommands;
import org.apache.harmony.jpda.tests.framework.jdwp.JDWPConstants;
import org.apache.harmony.jpda.tests.framework.jdwp.ReplyPacket;
import org.apache.harmony.jpda.tests.framework.jdwp.TaggedObject;
import org.apache.harmony.jpda.tests.framework.jdwp.Value;
import org.apache.harmony.jpda.tests.jdwp.share.JDWPSyncTestCase;
import org.apache.harmony.jpda.tests.jdwp.share.JDWPTestConstants;
import org.apache.harmony.jpda.tests.share.JPDADebuggeeSynchronizer;

import java.util.ArrayList;
import java.util.List;

/**
 * Base class to test ClassType.NewInstance for specific reference types.
 */
public abstract class AbstractNewInstanceTestCase extends JDWPSyncTestCase {

    protected static final String CONSTRUCTOR_NAME = "<init>";

    /**
     * A provider is responsible for giving the arguments passed to the tested
     * constructor.
     */
    protected static interface ConstructorArgumentsProvider {
        /**
         * This method is called to provide the arguments to the constructor
         * called to create the java.lang.String instance. This is called when
         * the debuggee is suspended on a breakpoint.
         *
         * @param constructorArguments
         *            the list of arguments passed to the constructor
         */
        public void provideConstructorArguments(List<Value> constructorArguments);
    }

    /**
     * Default implementation for constructor without argument.
     */
    protected static class NoConstructorArgumentProvider implements ConstructorArgumentsProvider {
        @Override
        public void provideConstructorArguments(List<Value> constructorArguments) {
        }
    }

    /**
     * Checks that ClassType.NewInstance command for the given type and constructor returns the
     * expected tag.
     * At first, the test starts the debuggee. Then request a breakpoint and wait for it.
     * Once the debuggee is suspended on the breakpoint, send ClassType.NewInstance command
     * for the given type using the constructor whose signature is given as parameter. A
     * provider is responsible to provide the arguments for the specified
     * constructor as JDWP values.
     * Finally, the test verifies that the returned object is not null, the exception object
     * is null and the returned object tag is the expected one.
     *
     * @param typeSignature the type signature of the created object
     * @param constructorSignature the constructor signature
     * @param provider the arguments provider
     * @param expectedTag the expected JDWP tag
     */
    protected void checkNewInstanceTag(String typeSignature, String constructorSignature,
            ConstructorArgumentsProvider provider, byte expectedTag) {
        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_READY);

        long debuggeeClassId = getClassIDBySignature(getDebuggeeClassSignature());
        logWriter.println("Debuggee class: " + getDebuggeeClassSignature());
        logWriter.println("Debuggee class ID: " + debuggeeClassId);

        // Request breakpoint.
        int breakpointRequestId = debuggeeWrapper.vmMirror
                .setBreakpointAtMethodBegin(debuggeeClassId, "breakpointMethod");

        // Continue debuggee.
        synchronizer.sendMessage(JPDADebuggeeSynchronizer.SGNL_CONTINUE);

        // Wait for breakpoint.
        long threadId = debuggeeWrapper.vmMirror.waitForBreakpoint(breakpointRequestId);

        // Clear breakpoint
        debuggeeWrapper.vmMirror.clearBreakpoint(breakpointRequestId);

        long referenceTypeId = getClassIDBySignature(typeSignature);
        assertTrue("Failed to find " + typeSignature, referenceTypeId != -1);
        logWriter.println(typeSignature + " class ID: " + referenceTypeId);

        final String methodName = CONSTRUCTOR_NAME;
        final String methodSignature = constructorSignature;
        final String fullMethodName = methodName + methodSignature;
        long constructorId = getMethodID(referenceTypeId, methodName, methodSignature);
        assertTrue("Failed to find constructor " + fullMethodName, constructorId != -1);
        logWriter.println(fullMethodName + " method ID: " + constructorId);

        // Request provider to fill the arguments list.
        List<Value> argumentsList = new ArrayList<Value>();
        provider.provideConstructorArguments(argumentsList);

        logWriter
                .println("Sending ClassType.NewInstance command for constructor " + fullMethodName);
        CommandPacket packet = new CommandPacket(JDWPCommands.ClassTypeCommandSet.CommandSetID,
                JDWPCommands.ClassTypeCommandSet.NewInstanceCommand);
        packet.setNextValueAsReferenceTypeID(referenceTypeId);
        packet.setNextValueAsThreadID(threadId);
        packet.setNextValueAsMethodID(constructorId);
        packet.setNextValueAsInt(argumentsList.size()); // argCount
        for (Value value : argumentsList) {
            packet.setNextValueAsValue(value);
        }
        packet.setNextValueAsInt(0); // invoke options
        ReplyPacket reply = debuggeeWrapper.vmMirror.performCommand(packet);
        checkReplyPacket(reply, "ClassType.NewInstance command");

        // Check result.
        TaggedObject objectResult = reply.getNextValueAsTaggedObject();
        TaggedObject exceptionResult = reply.getNextValueAsTaggedObject();
        assertAllDataRead(reply);

        assertNotNull("objectResult is null", objectResult);
        assertNotNull("exceptionResult is null", exceptionResult);
        assertTrue(objectResult.objectID != JDWPTestConstants.NULL_OBJECT_ID);
        assertTrue(exceptionResult.tag == JDWPConstants.Tag.OBJECT_TAG);
        assertEquals(exceptionResult.objectID, JDWPTestConstants.NULL_OBJECT_ID);
        assertTagEquals("Invalid tag", expectedTag, objectResult.tag);

        // Debuggee is suspended on the breakpoint: resume it now.
        resumeDebuggee();
    }
}
