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

import org.apache.log4j.Logger;
import org.mockftpserver.core.util.AssertFailedException;

/**
 * Tests for the StaticReplyCommandHandler class
 * 
 * @version $Revision$ - $Date$
 * 
 * @author Chris Mair
 */
public final class StaticReplyCommandHandlerTest extends AbstractCommandHandlerTest {

    private static final Logger LOG = Logger.getLogger(StaticReplyCommandHandlerTest.class);
    private static final int REPLY_CODE = 999;
    private static final String REPLY_TEXT = "some text 123";
    private static final Command COMMAND = new Command("ANY", EMPTY);
    
    private StaticReplyCommandHandler commandHandler;
    
    /**
     * Test the constructor that takes a replyCode, passing in a null
     */
    public void testConstructor_String_InvalidReplyCode() {
        try {
            new StaticReplyCommandHandler(-1);
            fail("Expected AssertFailedException");
        }
        catch (AssertFailedException expected) {
            LOG.info("Expected: " + expected);
        }
    }
    
    /**
     * Test the constructor that takes a replyCode and replyText, passing in a null replyCode
     */
    public void testConstructor_StringString_InvalidReplyCode() {
        try {
            new StaticReplyCommandHandler(-99, "text");
            fail("Expected AssertFailedException");
        }
        catch (AssertFailedException expected) {
            LOG.info("Expected: " + expected);
        }
    }
    
    /**
     * Test the setReplyCode() method, passing in a null
     */
    public void testSetReplyCode_Invalid() {
        try {
            commandHandler.setReplyCode(-1);
            fail("Expected AssertFailedException");
        }
        catch (AssertFailedException expected) {
            LOG.info("Expected: " + expected);
        }
    }
    
    /**
     * Test the handleCommand() method when the replyText attribute has not been set.
     * So, use whatever replyText has been configured in the replyCodeMapping
     * @throws Exception
     */
    public void testHandleCommand_ReplyTextNotSet() throws Exception {
        commandHandler.setReplyCode(250);
        
        session.sendReply(250, replyTextFor(250));
        replay(session);
        
        commandHandler.handleCommand(COMMAND, session);
        verify(session);
        
        verifyNumberOfInvocations(commandHandler, 1);
        verifyNoDataElements(commandHandler.getInvocation(0));
    }
    
    /**
     * Test the handleCommand() method, when the replyCode and replyText are both set
     * @throws Exception
     */
    public void testHandleCommand_SetReplyText() throws Exception {
        commandHandler.setReplyCode(REPLY_CODE);
        commandHandler.setReplyText(REPLY_TEXT);
        
        session.sendReply(REPLY_CODE, REPLY_TEXT);
        replay(session);
        
        commandHandler.handleCommand(COMMAND, session);
        verify(session);
        
        verifyNumberOfInvocations(commandHandler, 1);
        verifyNoDataElements(commandHandler.getInvocation(0));
    }
    
    /**
     * Test the handleCommand() method when the replyCode attribute has not been set
     * @throws Exception
     */
    public void testHandleCommand_ReplyCodeNotSet() throws Exception {

        try {
            commandHandler.handleCommand(COMMAND, session);
            fail("Expected AssertFailedException");
        }
        catch (AssertFailedException expected) {
            LOG.info("Expected: " + expected);
        }
        
        verifyNumberOfInvocations(commandHandler, 1);
        verifyNoDataElements(commandHandler.getInvocation(0));
    }
    
    /**
     * @see AbstractCommandHandlerTest#setUp()
     */
    protected void setUp() throws Exception {
        super.setUp();
        commandHandler = new StaticReplyCommandHandler();
        commandHandler.setReplyTextBundle(replyTextBundle);
    }
    
}
