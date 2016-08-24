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

package org.apache.harmony.jpda.tests.jdwp.StackFrame;

import org.apache.harmony.jpda.tests.framework.jdwp.CommandPacket;
import org.apache.harmony.jpda.tests.framework.jdwp.JDWPCommands;
import org.apache.harmony.jpda.tests.framework.jdwp.ReplyPacket;
import org.apache.harmony.jpda.tests.framework.jdwp.TaggedObject;
import org.apache.harmony.jpda.tests.framework.jdwp.Value;
import org.apache.harmony.jpda.tests.jdwp.share.JDWPProxyTestCase;

/**
 * JDWP Unit test for StackFrame.ThisObject command on proxy method.
 */
public class ProxyThisObjectTest extends JDWPProxyTestCase {

    public void testThisObject() {
        logWriter.println("testThisObject started");

        EventContext context = stopInProxyMethod();

        Value proxyObjectValue = getExpectedProxyObjectValue();

        logWriter.println("==> Send StackFrame::ThisObject command...");
        CommandPacket packet = new CommandPacket(
                JDWPCommands.StackFrameCommandSet.CommandSetID,
                JDWPCommands.StackFrameCommandSet.ThisObjectCommand);
        packet.setNextValueAsThreadID(context.getThreadId());
        packet.setNextValueAsLong(context.getFrameId());
        ReplyPacket reply = debuggeeWrapper.vmMirror.performCommand(packet);
        checkReplyPacket(reply, "StackFrame.ThisObject");
        TaggedObject taggedObject = reply.getNextValueAsTaggedObject();
        assertAllDataRead(reply);

        assertEquals("Unexpected object id", proxyObjectValue.getLongValue(),
                taggedObject.objectID);
        assertEquals("Unexpected object tag", proxyObjectValue.getTag(),
                taggedObject.tag);

        // Resume debuggee before leaving.
        resumeDebuggee();

        logWriter.println("testThisObject finished");
    }
}
