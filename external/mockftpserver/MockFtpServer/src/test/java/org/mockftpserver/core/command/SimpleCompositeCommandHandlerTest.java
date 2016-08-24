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

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.mockftpserver.core.session.Session;
import org.mockftpserver.core.util.AssertFailedException;
import org.mockftpserver.test.AbstractTestCase;

import java.util.ArrayList;
import java.util.List;
import java.util.ListResourceBundle;
import java.util.ResourceBundle;

/**
 * Tests for SimpleCompositeCommandHandler
 * 
 * @version $Revision$ - $Date$
 * 
 * @author Chris Mair
 */
public class SimpleCompositeCommandHandlerTest extends AbstractTestCase {

    private static final Logger LOG = LoggerFactory.getLogger(SimpleCompositeCommandHandlerTest.class);
    
    private SimpleCompositeCommandHandler simpleCompositeCommandHandler;
    private Session session;
    private Command command;
    private CommandHandler commandHandler1;
    private CommandHandler commandHandler2;
    private CommandHandler commandHandler3;
    
    /**
     * Test the handleCommand() method 
     */
    public void testHandleCommand_OneHandler_OneInvocation() throws Exception {
        simpleCompositeCommandHandler.addCommandHandler(commandHandler1);
        
        commandHandler1.handleCommand(command, session);
        replay(commandHandler1);
        
        simpleCompositeCommandHandler.handleCommand(command, session);
        verify(commandHandler1);
    }
    
    /**
     * Test the handleCommand() method, with two CommandHandler defined, but with multiple invocation 
     */
    public void testHandleCommand_TwoHandlers() throws Exception {
        simpleCompositeCommandHandler.addCommandHandler(commandHandler1);
        simpleCompositeCommandHandler.addCommandHandler(commandHandler2);
        
        commandHandler1.handleCommand(command, session);
        commandHandler2.handleCommand(command, session);
        replayAll();
        
        simpleCompositeCommandHandler.handleCommand(command, session);
        simpleCompositeCommandHandler.handleCommand(command, session);
        verifyAll();
    }
    
    /**
     * Test the handleCommand() method, with three CommandHandler defined, and multiple invocation 
     */
    public void testHandleCommand_ThreeHandlers() throws Exception {
        
        List list = new ArrayList();
        list.add(commandHandler1);
        list.add(commandHandler2);
        list.add(commandHandler3);
        simpleCompositeCommandHandler.setCommandHandlers(list);
        
        commandHandler1.handleCommand(command, session);
        commandHandler2.handleCommand(command, session);
        commandHandler3.handleCommand(command, session);
        replayAll();
        
        simpleCompositeCommandHandler.handleCommand(command, session);
        simpleCompositeCommandHandler.handleCommand(command, session);
        simpleCompositeCommandHandler.handleCommand(command, session);
        verifyAll();
    }
    
    /**
     * Test the handleCommand() method, with a single CommandHandler defined, but too many invocations 
     */
    public void testHandleCommand_OneHandler_TooManyInvocations() throws Exception {
        simpleCompositeCommandHandler.addCommandHandler(commandHandler1);
        
        commandHandler1.handleCommand(command, session);
        replay(commandHandler1);
        
        simpleCompositeCommandHandler.handleCommand(command, session);

        // Second invocation throws an exception
        try {
            simpleCompositeCommandHandler.handleCommand(command, session);
            fail("Expected AssertFailedException");
        }
        catch (AssertFailedException expected) {
            LOG.info("Expected: " + expected);
        }
    }
    
    /**
     * Test the handleCommand_NoHandlersDefined() method 
     */
    public void testHandleCommand_NoHandlersDefined() throws Exception {
        try {
            simpleCompositeCommandHandler.handleCommand(command, session);
            fail("Expected AssertFailedException");
        }
        catch(AssertFailedException expected) {
            LOG.info("Expected: " + expected);
        }
    }
    
    /**
     * Test the handleCommand(Command,Session) method, passing in a null Command
     */
    public void testHandleCommand_NullCommand() throws Exception {
        try {
            simpleCompositeCommandHandler.handleCommand(null, session);
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
            simpleCompositeCommandHandler.handleCommand(command, null);
            fail("Expected AssertFailedException");
        }
        catch (AssertFailedException expected) {
            LOG.info("Expected: " + expected);
        }
    }

    /**
     * Test the addCommandHandler(CommandHandler) method, passing in a null CommandHandler
     */
    public void testAddCommandHandler_NullCommandHandler() throws Exception {
        try {
            simpleCompositeCommandHandler.addCommandHandler(null);
            fail("Expected AssertFailedException");
        }
        catch (AssertFailedException expected) {
            LOG.info("Expected: " + expected);
        }
    }
    
    /**
     * Test the setCommandHandlers(List) method, passing in a null
     */
    public void testSetCommandHandlers_Null() throws Exception {
        try {
            simpleCompositeCommandHandler.setCommandHandlers(null);
            fail("Expected AssertFailedException");
        }
        catch (AssertFailedException expected) {
            LOG.info("Expected: " + expected);
        }
    }
    
    /**
     * Test the getCommandHandler(int) method, passing in an index for which no CommandHandler is defined
     */
    public void testGetCommandHandler_UndefinedIndex() throws Exception {
        simpleCompositeCommandHandler.addCommandHandler(commandHandler1);
        try {
            simpleCompositeCommandHandler.getCommandHandler(1);
            fail("Expected AssertFailedException");
        }
        catch (AssertFailedException expected) {
            LOG.info("Expected: " + expected);
        }
    }
    
    /**
     * Test the getCommandHandler(int) method
     */
    public void testGetCommandHandler() throws Exception {
        simpleCompositeCommandHandler.addCommandHandler(commandHandler1);
        simpleCompositeCommandHandler.addCommandHandler(commandHandler2);
        assertSame("index 0", commandHandler1, simpleCompositeCommandHandler.getCommandHandler(0));
        assertSame("index 1", commandHandler2, simpleCompositeCommandHandler.getCommandHandler(1));
    }
    
    /**
     * Test the getCommandHandler(int) method, passing in a negative index
     */
    public void testGetCommandHandler_NegativeIndex() throws Exception {
        simpleCompositeCommandHandler.addCommandHandler(commandHandler1);
        try {
            simpleCompositeCommandHandler.getCommandHandler(-1);
            fail("Expected AssertFailedException");
        }
        catch (AssertFailedException expected) {
            LOG.info("Expected: " + expected);
        }
    }
    
    /**
     * Test the getReplyTextBundle() method
     */
    public void testGetReplyTextBundle() {
        assertNull(simpleCompositeCommandHandler.getReplyTextBundle());
    }
    
    /**
     * Test the setReplyTextBundle() method
     */
    public void testSetReplyTextBundle() {
        
        AbstractTrackingCommandHandler replyTextBundleAwareCommandHandler1 = new StaticReplyCommandHandler();
        AbstractTrackingCommandHandler replyTextBundleAwareCommandHandler2 = new StaticReplyCommandHandler();
        simpleCompositeCommandHandler.addCommandHandler(replyTextBundleAwareCommandHandler1);
        simpleCompositeCommandHandler.addCommandHandler(commandHandler1);
        simpleCompositeCommandHandler.addCommandHandler(replyTextBundleAwareCommandHandler2);
        
        ResourceBundle resourceBundle = new ListResourceBundle() {
            protected Object[][] getContents() {
                return null;
            }
        };
        
        simpleCompositeCommandHandler.setReplyTextBundle(resourceBundle);
        assertSame("1", resourceBundle, replyTextBundleAwareCommandHandler1.getReplyTextBundle());
        assertSame("2", resourceBundle, replyTextBundleAwareCommandHandler1.getReplyTextBundle());
    }
    
    //-------------------------------------------------------------------------
    // Test setup
    //-------------------------------------------------------------------------
    
    /**
     * Perform initialization before each test
     * @see org.mockftpserver.test.AbstractTestCase#setUp()
     */
    protected void setUp() throws Exception {
        super.setUp();
        simpleCompositeCommandHandler = new SimpleCompositeCommandHandler();
        session = (Session) createMock(Session.class);
        command = new Command("cmd", EMPTY);
        commandHandler1 = (CommandHandler) createMock(CommandHandler.class);
        commandHandler2 = (CommandHandler) createMock(CommandHandler.class);
        commandHandler3 = (CommandHandler) createMock(CommandHandler.class);
    }
    
}
