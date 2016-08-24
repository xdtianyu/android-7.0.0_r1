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

package org.apache.harmony.jpda.tests.jdwp.ClassObjectReference;

import org.apache.harmony.jpda.tests.framework.jdwp.CommandPacket;
import org.apache.harmony.jpda.tests.framework.jdwp.JDWPCommands;
import org.apache.harmony.jpda.tests.framework.jdwp.JDWPConstants;
import org.apache.harmony.jpda.tests.framework.jdwp.ReplyPacket;
import org.apache.harmony.jpda.tests.jdwp.share.JDWPSyncTestCase;

/**
 * An abstract class for JDWP unit tests related to
 * ClassObjectReference.ReflectedType command.
 */
abstract class AbstractReflectedTypeTestCase extends JDWPSyncTestCase {
    static class TypeSignatureAndTag {
        public TypeSignatureAndTag(String typeSignature, byte typeTag) {
            this.typeSignature = typeSignature;
            this.typeTag = typeTag;
        }
        String typeSignature;
        byte typeTag;
    }

    protected void runReflectedTypeTest(TypeSignatureAndTag[] array) {
        for (int i = 0; i < array.length; i++) {
            logWriter.println("\n==> Checked class: " + array[i].typeSignature);

            // Get referenceTypeID
            logWriter.println
            ("==> Send VirtualMachine::ClassesBySignature command for checked class...");
            CommandPacket packet = new CommandPacket(
                    JDWPCommands.VirtualMachineCommandSet.CommandSetID,
                    JDWPCommands.VirtualMachineCommandSet.ClassesBySignatureCommand);
            packet.setNextValueAsString(array[i].typeSignature);
            ReplyPacket reply = debuggeeWrapper.vmMirror.performCommand(packet);
            checkReplyPacket(reply, "VirtualMachine::ClassesBySignature command");

            int classes = reply.getNextValueAsInt();
            logWriter.println("==> Number of returned classes = " + classes);
            //this class may be loaded only once
            byte refInitTypeTag = 0;
            long typeInitID = 0;
            int status = 0;

            for (int j = 0; j < classes; j++) {
                refInitTypeTag = reply.getNextValueAsByte();
                typeInitID = reply.getNextValueAsReferenceTypeID();
                status = reply.getNextValueAsInt();
                logWriter.println("==> refTypeId["+j+"] = " + typeInitID);
                logWriter.println("==> refTypeTag["+j+"] = " + refInitTypeTag + "("
                        + JDWPConstants.TypeTag.getName(refInitTypeTag) + ")");
                logWriter.println("==> classStatus["+j+"] = " + status + "("
                        + JDWPConstants.ClassStatus.getName(status) + ")");

                String classSignature = debuggeeWrapper.vmMirror.getClassSignature(typeInitID);
                logWriter.println("==> classSignature["+j+"] = " + classSignature);

                packet = new CommandPacket(
                        JDWPCommands.ReferenceTypeCommandSet.CommandSetID,
                        JDWPCommands.ReferenceTypeCommandSet.ClassLoaderCommand);
                packet.setNextValueAsReferenceTypeID(typeInitID);
                ReplyPacket reply2 = debuggeeWrapper.vmMirror.performCommand(packet);
                checkReplyPacket(reply, "ReferenceType::ClassLoader command");

                long classLoaderID = reply2.getNextValueAsObjectID();
                logWriter.println("==> classLoaderID["+j+"] = " + classLoaderID);

                if (classLoaderID != 0) {
                    String classLoaderSignature = getObjectSignature(classLoaderID);
                    logWriter.println("==> classLoaderSignature["+j+"] = " + classLoaderSignature);
                } else {
                    logWriter.println("==> classLoader is system class loader");
                }
            }

            assertEquals("VirtualMachine::ClassesBySignature returned invalid number of classes,",
                    1, classes);
            assertEquals("VirtualMachine::ClassesBySignature returned invalid TypeTag,",
                    array[i].typeTag, refInitTypeTag,
                    JDWPConstants.TypeTag.getName(array[i].typeTag),
                    JDWPConstants.TypeTag.getName(refInitTypeTag));

            // Get ClassObject
            logWriter.println
            ("==> Send ReferenceType::ClassObject command for checked class...");
            packet = new CommandPacket(
                    JDWPCommands.ReferenceTypeCommandSet.CommandSetID,
                    JDWPCommands.ReferenceTypeCommandSet.ClassObjectCommand);
            packet.setNextValueAsReferenceTypeID(typeInitID);
            reply = debuggeeWrapper.vmMirror.performCommand(packet);
            checkReplyPacket(reply, "ReferenceType::ClassObject command");

            long classObject = reply.getNextValueAsClassObjectID();
            assertAllDataRead(reply);
            logWriter.println("==> classObjectID=" + classObject);

            // Get ReflectedType
            logWriter.println
            ("==> Send ClassObjectReference::ReflectedType command for classObjectID...");
            packet = new CommandPacket(
                    JDWPCommands.ClassObjectReferenceCommandSet.CommandSetID,
                    JDWPCommands.ClassObjectReferenceCommandSet.ReflectedTypeCommand);
            packet.setNextValueAsObjectID(classObject);
            reply = debuggeeWrapper.vmMirror.performCommand(packet);
            checkReplyPacket(reply, "ClassObjectReference::ReflectedType command");

            byte refTypeTag = reply.getNextValueAsByte();
            long typeID = reply.getNextValueAsReferenceTypeID();
            logWriter.println("==> reflectedTypeId = " + typeID);
            logWriter.println("==> reflectedTypeTag = " + refTypeTag
                    + "(" + JDWPConstants.TypeTag.getName(refTypeTag) + ")");

            assertEquals("ClassObjectReference::ReflectedType returned invalid reflected TypeTag,",
                    array[i].typeTag, refTypeTag,
                    JDWPConstants.TypeTag.getName(array[i].typeTag),
                    JDWPConstants.TypeTag.getName(refTypeTag));
            assertEquals("ClassObjectReference::ReflectedType returned invalid reflected typeID,",
                    typeInitID, typeID);
            assertAllDataRead(reply);
        }
    }
}
