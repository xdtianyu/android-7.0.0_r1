/*
 * Copyright 2007 the original author or authors.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *      http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package org.mockftpserver.stub.command;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.mockftpserver.core.command.Command;
import org.mockftpserver.core.command.InvocationRecord;
import org.mockftpserver.core.session.Session;
import org.mockftpserver.core.util.AssertFailedException;
import org.mockftpserver.test.AbstractTestCase;

import java.util.ListResourceBundle;
import java.util.ResourceBundle;

/**
 * Tests for AbstractStubDataCommandHandler. The class name is prefixed with an underscore
 * so that it is not filtered out by Maven's Surefire test plugin.
 * 
 * @version $Revision$ - $Date$
 * 
 * @author Chris Mair
 */
public final class _AbstractStubDataCommandHandlerTest extends AbstractTestCase {

    private static final Logger LOG = LoggerFactory.getLogger(_AbstractStubDataCommandHandlerTest.class);
    private static final Command COMMAND = new Command("command", EMPTY);
    private static final InvocationRecord INVOCATION_RECORD = new InvocationRecord(COMMAND, DEFAULT_HOST);

    private static final String REPLY_TEXT150 = "reply 150 ... abcdef";
    private static final String REPLY_TEXT226 = "reply 226 ... abcdef";
    private static final String REPLY_TEXT222 = "reply 222 ... abcdef";
    private static final String REPLY_TEXT333 = "reply 333 ... abcdef";
    private static final String REPLY_TEXT444 = "reply 444 ... abcdef";
    
    private Session session;
    private ResourceBundle replyTextBundle;
    private AbstractStubDataCommandHandler commandHandler;

    /**
     * Test the handleCommand() method
     */
    public void testHandleCommand() throws Exception {

        session.sendReply(150, REPLY_TEXT150);
        session.openDataConnection();
        session.sendReply(222, REPLY_TEXT222);
        session.sendReply(333, REPLY_TEXT333);
        session.sendReply(444, REPLY_TEXT444);
        session.closeDataConnection();
        session.sendReply(226, REPLY_TEXT226);
        replay(session);
        
        // Define CommandHandler test subclass
        commandHandler = new AbstractStubDataCommandHandler() {
            protected void beforeProcessData(Command c, Session s, InvocationRecord ir) {
                verifyParameters(c, s, ir);
                // Send unique reply code so that we can verify proper method invocation and ordering
                session.sendReply(222, REPLY_TEXT222);
            }

            protected void processData(Command c, Session s, InvocationRecord ir) {
                verifyParameters(c, s, ir);
                // Send unique reply code so that we can verify proper method invocation and ordering
                session.sendReply(333, REPLY_TEXT333);
            }

            protected void afterProcessData(Command c, Session s, InvocationRecord ir) {
                verifyParameters(c, s, ir);
                // Send unique reply code so that we can verify proper method invocation and ordering
                session.sendReply(444, REPLY_TEXT444);
            }

            private void verifyParameters(Command c, Session s, InvocationRecord ir) {
                assertSame("command", COMMAND, c);
                assertSame("session", session, s);
                assertSame("invocationRecord", INVOCATION_RECORD, ir);
            }
        };

        commandHandler.setReplyTextBundle(replyTextBundle);
        commandHandler.handleCommand(COMMAND, session, INVOCATION_RECORD);
        
        verify(session);
    }

    /**
     * Test the handleCommand() method, overriding the initial reply code and text
     */
    public void testHandleCommand_OverrideInitialReplyCodeAndText() throws Exception {

        final int OVERRIDE_REPLY_CODE = 333;
        final String OVERRIDE_REPLY_TEXT = "reply text";
        
        session.sendReply(OVERRIDE_REPLY_CODE, OVERRIDE_REPLY_TEXT);
        session.openDataConnection();
        session.closeDataConnection();
        session.sendReply(226, REPLY_TEXT226);
        replay(session);
        
        commandHandler.setPreliminaryReplyCode(OVERRIDE_REPLY_CODE);
        commandHandler.setPreliminaryReplyText(OVERRIDE_REPLY_TEXT);
        commandHandler.setReplyTextBundle(replyTextBundle);
        commandHandler.handleCommand(COMMAND, session, INVOCATION_RECORD);
        
        verify(session);
    }

    /**
     * Test the setPreliminaryReplyCode() method, passing in an invalid value 
     */
    public void testSetPreliminaryReplyCode_Invalid() {
        try {
            commandHandler.setPreliminaryReplyCode(0);
            fail("Expected AssertFailedException");
        }
        catch (AssertFailedException expected) {
            LOG.info("Expected: " + expected);
        }
    }

    /**
     * Test the setFinalReplyCode() method, passing in an invalid value 
     */
    public void testSetFinalReplyCode_Invalid() {
        try {
            commandHandler.setFinalReplyCode(0);
            fail("Expected AssertFailedException");
        }
        catch (AssertFailedException expected) {
            LOG.info("Expected: " + expected);
        }
    }

    //-------------------------------------------------------------------------
    // Test setup
    //-------------------------------------------------------------------------
    
    /**
     * Perform initialization before each test
     * 
     * @see org.mockftpserver.test.AbstractTestCase#setUp()
     */
    protected void setUp() throws Exception {
        super.setUp();
        session = (Session) createMock(Session.class);
        replyTextBundle = new ListResourceBundle() {
            protected Object[][] getContents() {
                return new Object[][] { 
                        { Integer.toString(150), REPLY_TEXT150 }, 
                        { Integer.toString(222), REPLY_TEXT222 }, 
                        { Integer.toString(226), REPLY_TEXT226 }, 
                        { Integer.toString(333), REPLY_TEXT333 }, 
                        { Integer.toString(444), REPLY_TEXT444 }, 
                };
            }
        };
        commandHandler = new AbstractStubDataCommandHandler() {
            protected void processData(Command c, Session s, InvocationRecord ir) {
            }
        };
    }
    
}
