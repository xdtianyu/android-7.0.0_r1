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
import org.apache.harmony.jpda.tests.framework.jdwp.ReplyPacket;
import org.apache.harmony.jpda.tests.framework.jdwp.TaggedObject;
import org.apache.harmony.jpda.tests.framework.jdwp.Value;
import org.apache.harmony.jpda.tests.jdwp.share.JDWPInvokeMethodWithSuspensionTestCase;
import org.apache.harmony.jpda.tests.jdwp.share.debuggee.InvokeMethodWithSuspensionDebuggee;

/**
 * JDWP unit test for ClassType.InvokeCommand command with a thread suspension.
 */
public class InvokeMethodWithSuspensionTest extends JDWPInvokeMethodWithSuspensionTestCase {
    public void testInvokeWithMultipleEvents001() {
        runInvokeMethodTest(InvokeMethodWithSuspensionDebuggee.STATIC_METHOD_NAME);
    }

    @Override
    protected CommandPacket buildInvokeCommand(long threadId, long classID,
            long methodId, int invoke_options) {
        CommandPacket command = new CommandPacket(
                JDWPCommands.ClassTypeCommandSet.CommandSetID,
                JDWPCommands.ClassTypeCommandSet.InvokeMethodCommand);
        command.setNextValueAsClassID(classID);
        command.setNextValueAsThreadID(threadId);
        command.setNextValueAsMethodID(methodId);
        command.setNextValueAsInt(0);
        command.setNextValueAsInt(invoke_options);
        return command;
    }

    @Override
    protected String getInvokeCommandName() {
        return "ClassType.InvokeCommand";
    }

    @Override
    protected void checkInvokeReply(ReplyPacket reply) {
        // Check result is 'void'
        Value invokeResult = reply.getNextValueAsValue();
        assertNull("Expect null result value for 'void'", invokeResult);

        // Check exception is null.
        TaggedObject invokeException = reply.getNextValueAsTaggedObject();
        assertEquals("Invalid exception object id", 0, invokeException.objectID);
        assertAllDataRead(reply);

    }

}
