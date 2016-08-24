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
 * Created on 07.02.2005
 */

package org.apache.harmony.jpda.tests.jdwp.StringReference;

import java.io.UnsupportedEncodingException;

import org.apache.harmony.jpda.tests.framework.jdwp.CommandPacket;
import org.apache.harmony.jpda.tests.framework.jdwp.JDWPCommands;
import org.apache.harmony.jpda.tests.framework.jdwp.JDWPConstants;
import org.apache.harmony.jpda.tests.framework.jdwp.ReplyPacket;
import org.apache.harmony.jpda.tests.jdwp.share.JDWPSyncTestCase;
import org.apache.harmony.jpda.tests.jdwp.share.JDWPTestConstants;
import org.apache.harmony.jpda.tests.share.JPDADebuggeeSynchronizer;


/**
 * JDWP Unit test for StringReference.Value command.
 */
public class ValueTest extends JDWPSyncTestCase {

    protected String getDebuggeeClassName() {
        return "org.apache.harmony.jpda.tests.jdwp.share.debuggee.HelloWorld";
    }

    protected void checkString(String testString) throws UnsupportedEncodingException {
        logWriter.println("=> Test string: \"" + testString + "\"");

        // create new string
        long stringID = createString(testString);

        // get string from StringID
        String returnedTestString = getStringValue(stringID);
        logWriter.println("=> Returned string: \"" + returnedTestString + "\"");

        assertString("StringReference::Value command returned invalid string,",
                testString, returnedTestString);
    }

    /**
     * This testcase exercises StringReference.Value command.
     * <BR>The test starts HelloWorld debuggee, create some strings
     * with VirtualMachine.CreateString command.
     * <BR> Then the test checks that for each created string
     * StringReference.Value command returns string which is
     * equal to string used for CreateString command.
     */
    public void testStringReferenceValueTest001() throws UnsupportedEncodingException {
        logWriter.println("StringReferenceValueTest001 started");
        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_READY);

        checkString("");
        checkString("1234567890");
        checkString("Some test string with various ASCII symbols "
            + "~!@#$%^&*()_+|}{\"'?><,./");
        checkString("Some test string with various national symbols "
            + "\u00a9\u0436\u0433\u0404\u0490\u00ad\u0408\u0438\u0439\u00a7\u0435"
            + "\u043a\u043d\u00a6\u00a4\u00ab\u00ae\u0430\u0407\u00a0\u045e\u043b"
            + "\u0434\u043f\u0437\u0431\u00ac\u0401\u0432\u043c\u040e\u043e.");

        synchronizer.sendMessage(JPDADebuggeeSynchronizer.SGNL_CONTINUE);
    }

    /**
     * This testcase exercises StringReference.Value command.
     * <BR>The test starts HelloWorld debuggee then checks INVALID_OBJECT error
     * is returned if we pass a null id.
     */
    public void testStringReferenceValueTest001_NullString() {
        logWriter.println("testStringReferenceValueTest001_NullString started");
        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_READY);

        checkCommandError(JDWPTestConstants.NULL_OBJECT_ID,
                          JDWPConstants.Error.INVALID_OBJECT);

        synchronizer.sendMessage(JPDADebuggeeSynchronizer.SGNL_CONTINUE);
    }

    /**
     * This testcase exercises StringReference.Value command.
     * <BR>The test starts HelloWorld debuggee then checks INVALID_OBJECT error
     * is returned if we pass an invalid object id.
     */
    public void testStringReferenceValueTest001_InvalidObject() {
        logWriter.println("testStringReferenceValueTest001_InvalidObject started");
        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_READY);

        checkCommandError(JDWPTestConstants.INVALID_OBJECT_ID,
                          JDWPConstants.Error.INVALID_OBJECT);

        synchronizer.sendMessage(JPDADebuggeeSynchronizer.SGNL_CONTINUE);
    }

    /**
     * This testcase exercises StringReference.Value command.
     * <BR>The test starts HelloWorld debuggee then checks INVALID_STRING error
     * is returned if we pass a valid object id but the corresponding object
     * is not a java.lang.String.
     */
    public void testStringReferenceValueTest001_InvalidString() {
        logWriter.println("testStringReferenceValueTest001_InvalidString started");
        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_READY);

        String signature = "Lorg/apache/harmony/jpda/tests/jdwp/share/debuggee/HelloWorld;";
        long debuggeeClassID = getClassIDBySignature(signature);
        checkCommandError(debuggeeClassID, JDWPConstants.Error.INVALID_STRING);

        synchronizer.sendMessage(JPDADebuggeeSynchronizer.SGNL_CONTINUE);
    }

    private void checkCommandError(long stringID, int expectedError) {
        logWriter.println("Send StringReference.Value command with id " + stringID);

        CommandPacket packet = new CommandPacket(
                JDWPCommands.StringReferenceCommandSet.CommandSetID,
                JDWPCommands.StringReferenceCommandSet.ValueCommand);
        packet.setNextValueAsObjectID(stringID);
        ReplyPacket reply = debuggeeWrapper.vmMirror.performCommand(packet);

        checkReplyPacket(reply, "StringReference::Value command", expectedError);
    }
}
