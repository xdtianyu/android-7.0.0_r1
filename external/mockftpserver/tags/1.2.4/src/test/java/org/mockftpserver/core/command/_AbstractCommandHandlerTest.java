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
package org.mockftpserver.core.command;

import java.util.ListResourceBundle;
import java.util.ResourceBundle;

import org.apache.log4j.Logger;
import org.easymock.MockControl;
import org.mockftpserver.core.session.Session;
import org.mockftpserver.core.util.AssertFailedException;
import org.mockftpserver.stub.command.AbstractStubCommandHandler;
import org.mockftpserver.test.AbstractTest;

/**
 * Tests for the AbstractCommandHandler class. The class name is prefixed with an underscore
 * so that it is not filtered out by Maven's Surefire test plugin.
 *  
 * @version $Revision$ - $Date$
 * 
 * @author Chris Mair
 */
public final class _AbstractCommandHandlerTest extends AbstractTest {

    private static final Logger LOG = Logger.getLogger(_AbstractCommandHandlerTest.class);
    private static final String COMMAND_NAME = "abc";
    private static final Object ARG = "123";
    private static final Object[] ARGS = { ARG };
    private static final Command COMMAND = new Command(COMMAND_NAME, EMPTY);
    private static final Command COMMAND_WITH_ARGS = new Command(COMMAND_NAME, EMPTY);
    private static final int REPLY_CODE1 = 777;
    private static final int REPLY_CODE2 = 888;
    private static final int REPLY_CODE3 = 999;
    private static final String REPLY_TEXT1 = "reply1 ... abcdef";
    private static final String REPLY_TEXT2 = "abc {0} def";
    private static final String REPLY_TEXT2_FORMATTED = "abc 123 def";
    private static final String OVERRIDE_REPLY_TEXT = "overridden reply ... abcdef";
    private static final String MESSAGE_KEY = "key.123";
    private static final String MESSAGE_TEXT = "message.123";
    
    private AbstractCommandHandler commandHandler;
    private Session session;
    private ResourceBundle replyTextBundle; 
    
    /**
     * Test the handleCommand(Command,Session) method
     */
    public void testHandleCommand() throws Exception {
        assertEquals("before", 0, commandHandler.numberOfInvocations());
        commandHandler.handleCommand(COMMAND, session);
        assertEquals("after", 1, commandHandler.numberOfInvocations());
        assertTrue("locked", commandHandler.getInvocation(0).isLocked());
    }

    /**
     * Test the handleCommand(Command,Session) method, passing in a null Command
     */
    public void testHandleCommand_NullCommand() throws Exception {
        try {
            commandHandler.handleCommand(null, session);
            fail("Expected AssertFailedException");
        }
        catch (AssertFailedException expected) {
            LOG.info("Expected: " + expected);
        }
    }
    
    /**
     * Test the handleCommand(Command,Session) method, passing in a null Session
     */
    public void testHandleCommand_NullSession() throws Exception {
        try {
            commandHandler.handleCommand(COMMAND, null);
            fail("Expected AssertFailedException");
        }
        catch (AssertFailedException expected) {
            LOG.info("Expected: " + expected);
        }
    }
    
    /**
     * Test the numberOfInvocations(), addInvocationRecord() and clearInvocationRecord() methods
     */
    public void testInvocationHistory() throws Exception {
        control(session).expectAndDefaultReturn(session.getClientHost(), DEFAULT_HOST);
        replay(session);

        assertEquals("none", 0, commandHandler.numberOfInvocations());
        commandHandler.handleCommand(COMMAND, session);
        assertEquals("1", 1, commandHandler.numberOfInvocations());
        commandHandler.handleCommand(COMMAND, session);
        assertEquals("2", 2, commandHandler.numberOfInvocations());
        commandHandler.clearInvocations();
        assertEquals("cleared", 0, commandHandler.numberOfInvocations());
    }

    /**
     * Test the getInvocation() method
     * @throws Exception 
     */
    public void testGetInvocation() throws Exception {
        control(session).expectAndDefaultReturn(session.getClientHost(), DEFAULT_HOST);
        replay(session);

        commandHandler.handleCommand(COMMAND, session);
        commandHandler.handleCommand(COMMAND_WITH_ARGS, session);
        assertSame("1", COMMAND, commandHandler.getInvocation(0).getCommand());
        assertSame("2", COMMAND_WITH_ARGS, commandHandler.getInvocation(1).getCommand());
    }

    /**
     * Test the getInvocation() method, passing in an invalid index
     */
    public void testGetInvocation_IndexOutOfBounds() throws Exception {
        commandHandler.handleCommand(COMMAND, session);
        try {
            commandHandler.getInvocation(2);
            fail("Expected IndexOutOfBoundsException");
        }
        catch (IndexOutOfBoundsException expected) {
            LOG.info("Expected: " + expected);
        }
    }

    /**
     * Test the quotes utility method 
     */
    public void testQuotes() {
        assertEquals("abc", "\"abc\"", AbstractStubCommandHandler.quotes("abc"));
        assertEquals("<empty>", "\"\"", AbstractStubCommandHandler.quotes(""));
    }
    
    /**
     * Test the quotes utility method, passing in a null 
     */
    public void testQuotes_Null() {
        try {
            AbstractStubCommandHandler.quotes(null);
            fail("Expected AssertFailedException");
        }
        catch (AssertFailedException expected) {
            LOG.info("Expected: " + expected);
        }
    }
    
    /**
     * Test the assertValidReplyCode() method 
     */
    public void testAssertValidReplyCode() {
        // These are valid, so expect no exceptions
        commandHandler.assertValidReplyCode(1);
        commandHandler.assertValidReplyCode(100);

        // These are invalid
        testAssertValidReplyCodeWithInvalid(0);
        testAssertValidReplyCodeWithInvalid(-1);
    }

    /**
     * Test the assertValidReplyCode() method , passing in an invalid replyCode value
     */
    private void testAssertValidReplyCodeWithInvalid(int invalidReplyCode) {
        try {
            commandHandler.assertValidReplyCode(invalidReplyCode);
            fail("Expected AssertFailedException");
        }
        catch (AssertFailedException expected) {
            LOG.info("Expected: " + expected);
        }
    }

    /**
     * Test the sendReply() method, when no message arguments are specified
     */
    public void testSendReply() {
        session.sendReply(REPLY_CODE1, REPLY_TEXT1);
        session.sendReply(REPLY_CODE1, MESSAGE_TEXT);
        session.sendReply(REPLY_CODE1, OVERRIDE_REPLY_TEXT);
        session.sendReply(REPLY_CODE3, null);
        replay(session);
        
        commandHandler.sendReply(session, REPLY_CODE1, null, null, null);
        commandHandler.sendReply(session, REPLY_CODE1, MESSAGE_KEY, null, null);
        commandHandler.sendReply(session, REPLY_CODE1, MESSAGE_KEY, OVERRIDE_REPLY_TEXT, null);
        commandHandler.sendReply(session, REPLY_CODE3, null, null, null);
        
        verify(session);
    }
    
    /**
     * Test the sendReply() method, passing in message arguments 
     */
    public void testSendReply_WithMessageArguments() {
        session.sendReply(REPLY_CODE1, REPLY_TEXT2_FORMATTED);
        replay(session);
        
        commandHandler.sendReply(session, REPLY_CODE1, null, REPLY_TEXT2, ARGS);
        
        verify(session);
    }
    
    /**
     * Test the sendReply() method, passing in a null Session 
     */
    public void testSendReply_NullSession() {
        try {
            commandHandler.sendReply(null, REPLY_CODE1, REPLY_TEXT1, null, null);
            fail("Expected AssertFailedException");
        }
        catch (AssertFailedException expected) {
            LOG.info("Expected: " + expected);
        }
    }

    /**
     * Test the sendReply() method, passing in an invalid replyCode 
     */
    public void testSendReply_InvalidReplyCode() {
        try {
            commandHandler.sendReply(session, 0, REPLY_TEXT1, null, null);
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
     * @see org.mockftpserver.test.AbstractTest#setUp()
     */
    protected void setUp() throws Exception {
        super.setUp();
        session = (Session) createMock(Session.class);
        control(session).setDefaultMatcher(MockControl.ARRAY_MATCHER);
        commandHandler = new AbstractCommandHandler() {
            public void handleCommand(Command command, Session session, InvocationRecord invocationRecord) throws Exception {
            } 
        };
        replyTextBundle = new ListResourceBundle() {
            protected Object[][] getContents() {
                return new Object[][] { 
                        { Integer.toString(REPLY_CODE1), REPLY_TEXT1 }, 
                        { Integer.toString(REPLY_CODE2), REPLY_TEXT2 }, 
                        { MESSAGE_KEY, MESSAGE_TEXT } 
                };
            }
        };
        commandHandler.setReplyTextBundle(replyTextBundle);
    }
    
}
