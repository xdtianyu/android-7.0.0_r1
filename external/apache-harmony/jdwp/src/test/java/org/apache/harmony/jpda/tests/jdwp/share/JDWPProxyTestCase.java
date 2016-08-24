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

package org.apache.harmony.jpda.tests.jdwp.share;

import org.apache.harmony.jpda.tests.framework.jdwp.Location;
import org.apache.harmony.jpda.tests.framework.jdwp.ReplyPacket;
import org.apache.harmony.jpda.tests.framework.jdwp.Value;
import org.apache.harmony.jpda.tests.jdwp.share.debuggee.ProxyDebuggee;
import org.apache.harmony.jpda.tests.share.JPDADebuggeeSynchronizer;

public abstract class JDWPProxyTestCase extends JDWPSyncTestCase {

    @Override
    protected String getDebuggeeClassName() {
        return ProxyDebuggee.class.getName();
    }

    protected static class EventContext {
        private final long threadId;
        private final long frameId;
        private final Location location;

        public EventContext(long threadId, long frameId, Location location) {
            this.threadId = threadId;
            this.frameId = frameId;
            this.location = location;
        }

        public long getThreadId() {
            return threadId;
        }

        public long getFrameId() {
            return frameId;
        }

        public Location getLocation() {
            return location;
        }
    }

    protected EventContext stopInProxyMethod() {
        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_READY);

        // Set breakpoint.
        String debuggeeSignature = getDebuggeeClassSignature();
        long classId = getClassIDBySignature(debuggeeSignature);
        int requestId = debuggeeWrapper.vmMirror.setBreakpointAtMethodBegin(
                classId, "breakpointMethodFromProxy");

        // Signal debuggee to continue execution.
        synchronizer.sendMessage(JPDADebuggeeSynchronizer.SGNL_CONTINUE);

        long eventThreadId = debuggeeWrapper.vmMirror.waitForBreakpoint(
                requestId);

        String proxyClassSignature = "Ljava/lang/reflect/Proxy;";
        long proxyClassId = getClassIDBySignature(proxyClassSignature);
        assertTrue("No class " + proxyClassSignature, proxyClassId != -1);

        EventContext context = getFirstProxyFrameId(eventThreadId, proxyClassId);
        assertNotNull("Failed to find proxy frame", context);

        return context;
    }

    private EventContext getFirstProxyFrameId(long eventThreadId, long proxyClassId) {
        int framesCount = debuggeeWrapper.vmMirror.getFrameCount(eventThreadId);
        ReplyPacket reply = debuggeeWrapper.vmMirror.getThreadFrames(eventThreadId, 0, framesCount);
        framesCount = reply.getNextValueAsInt();
        for (int i = 0; i < framesCount; ++i) {
            long frameId = reply.getNextValueAsFrameID();
            Location location = reply.getNextValueAsLocation();
            // TODO dump frame for context.
            String className = getClassSignature(location.classID);
            String methodName = getMethodName(location.classID, location.methodID);
            logWriter.println("Frame #" + i + ": " + className + "." + methodName + "@" + location.index);
            if (location.classID != proxyClassId &&
                    IsProxy(location.classID, proxyClassId)) {
                return new EventContext(eventThreadId, frameId, location);
            }
        }
        return null;
    }

    private boolean IsProxy(long typeId, long proxyClassId) {
        if (typeId == proxyClassId) {
            return true;
        } else if (typeId == 0) {
            return false;
        } else {
            return IsProxy(debuggeeWrapper.vmMirror.getSuperclassId(typeId),
                    proxyClassId);
        }
    }

    protected Value getExpectedProxyObjectValue() {
        String debuggeeSignature = getDebuggeeClassSignature();
        long classId = getClassIDBySignature(debuggeeSignature);
        long checkedProxyObjectFieldId = checkField(classId, "checkedProxyObject");
        long[] fieldIDs = new long[] { checkedProxyObjectFieldId };
        Value[] fieldValues = debuggeeWrapper.vmMirror.getReferenceTypeValues(classId, fieldIDs);
        Value value = fieldValues[0];
        logWriter.println("Proxy object is " + value.toString());
        return value;
    }
}
