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
 * @author Viacheslav G. Rybalov
 */

/**
 * Created on 05.03.2005
 */
package org.apache.harmony.jpda.tests.jdwp.ClassObjectReference;

import org.apache.harmony.jpda.tests.framework.jdwp.CommandPacket;
import org.apache.harmony.jpda.tests.framework.jdwp.JDWPCommands;
import org.apache.harmony.jpda.tests.framework.jdwp.JDWPConstants;
import org.apache.harmony.jpda.tests.framework.jdwp.ReplyPacket;
import org.apache.harmony.jpda.tests.jdwp.ClassObjectReference.AbstractReflectedTypeTestCase.TypeSignatureAndTag;
import org.apache.harmony.jpda.tests.jdwp.share.JDWPSyncTestCase;
import org.apache.harmony.jpda.tests.share.JPDADebuggeeSynchronizer;


/**
 * JDWP unit test for ClassObjectReference.ReflectedType command.
 */

public class ReflectedTypeTest extends AbstractReflectedTypeTestCase {

    private static final String DEBUGGEE_CLASS_NAME =
            "org.apache.harmony.jpda.tests.jdwp.share.debuggee.HelloWorld";

    /**
     * Returns full name of debuggee class which is used by this test.
     * @return full name of debuggee class.
     */
    protected String getDebuggeeClassName() {
        return DEBUGGEE_CLASS_NAME;
    }

    /**
     * This testcase exercises ClassObjectReference.ReflectedType command.
     * <BR>Starts <A HREF="../share/debuggee/HelloWorld.html">HelloWorld</A> debuggee.
     * <BR>Then checks the following four classes:
     * <BR>&nbsp;&nbsp; - java/lang/Object;
     * <BR>&nbsp;&nbsp; - java/lang/String;
     * <BR>&nbsp;&nbsp; - java/lang/Runnable;
     * <BR>&nbsp;&nbsp; - HelloWorld.
     * <BR>&nbsp;&nbsp;
     * <BR>The following statements are checked:
     * <BR>&nbsp;It is expected:
     * <BR>&nbsp;&nbsp; - refTypeTag takes one of the TypeTag constants: CLASS, INTERFACE;
     * <BR>&nbsp;&nbsp; - refTypeTag equals to refTypeTag returned by command
     *  VirtualMachine.ClassesBySignature;
     * <BR>&nbsp;&nbsp; - typeID equals to typeID returned by the JDWP command
     * VirtualMachine.ClassesBySignature;
     * <BR>&nbsp;&nbsp; - All data were read;
     */
    public void testReflectedType001() {
        logWriter.println("==> testReflectedType001 START...");
        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_READY);

        // Builds debuggee class signature string.
        String debuggeeClassSignature =
                "L" + DEBUGGEE_CLASS_NAME.replace('.', '/') + ";";

        TypeSignatureAndTag[] array = new TypeSignatureAndTag[] {
            new TypeSignatureAndTag("Ljava/lang/Object;",
                                    JDWPConstants.TypeTag.CLASS),
            new TypeSignatureAndTag("Ljava/lang/String;",
                                    JDWPConstants.TypeTag.CLASS),
            new TypeSignatureAndTag("Ljava/lang/Runnable;",
                                    JDWPConstants.TypeTag.INTERFACE),
            new TypeSignatureAndTag(debuggeeClassSignature,
                                    JDWPConstants.TypeTag.CLASS)
        };

        runReflectedTypeTest(array);

        synchronizer.sendMessage(JPDADebuggeeSynchronizer.SGNL_CONTINUE);
    }
}
