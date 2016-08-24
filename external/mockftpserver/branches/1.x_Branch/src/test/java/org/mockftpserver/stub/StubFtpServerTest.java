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
package org.mockftpserver.stub;

import java.util.HashMap;
import java.util.Map;
import java.util.ResourceBundle;


import org.apache.log4j.Logger;
import org.mockftpserver.core.command.Command;
import org.mockftpserver.core.command.CommandHandler;
import org.mockftpserver.core.command.InvocationRecord;
import org.mockftpserver.core.session.Session;
import org.mockftpserver.core.util.AssertFailedException;
import org.mockftpserver.stub.StubFtpServer;
import org.mockftpserver.stub.command.AbstractStubCommandHandler;
import org.mockftpserver.stub.command.CwdCommandHandler;
import org.mockftpserver.test.AbstractTest;

/**
 * Unit tests for StubFtpServer. Also see {@link StubFtpServer_StartTest} 
 * and {@link StubFtpServerIntegrationTest}.
 * 
 * @version $Revision$ - $Date$
 * 
 * @author Chris Mair
 */
public final class StubFtpServerTest extends AbstractTest {

    private static final Logger LOG = Logger.getLogger(StubFtpServerTest.class);
    
    private StubFtpServer stubFtpServer;
    private AbstractStubCommandHandler commandHandler;
    private CommandHandler commandHandler_NoReplyTextBundle;

    /**
     * Test the setCommandHandlers() method
     */
    public void testSetCommandHandlers() {
        Map mapping = new HashMap();
        mapping.put("AAA", commandHandler);
        mapping.put("BBB", commandHandler_NoReplyTextBundle);
        
        stubFtpServer.setCommandHandlers(mapping);
        assertSame("commandHandler1", commandHandler, stubFtpServer.getCommandHandler("AAA"));
        assertSame("commandHandler2", commandHandler_NoReplyTextBundle, stubFtpServer.getCommandHandler("BBB"));
        
        assertSame("replyTextBundle", stubFtpServer.replyTextBundle, commandHandler.getReplyTextBundle());
        
        // Make sure default CommandHandlers are still set
        assertEquals("CwdCommandHandler", CwdCommandHandler.class, stubFtpServer.getCommandHandler("CWD").getClass());
    }
    
    /**
     * Test the setCommandHandlers() method, when the Map is null
     */
    public void testSetCommandHandlers_Null() {
        try {
            stubFtpServer.setCommandHandlers(null);
            fail("Expected AssertFailedException");
        }
        catch (AssertFailedException expected) {
            LOG.info("Expected: " + expected);
        }
    }
    
    /**
     * Test the setCommandHandler() method, for a CommandHandler that does not implement ResourceBundleAware
     */
    public void testSetCommandHandler_NotReplyTextBundleAware() {
        stubFtpServer.setCommandHandler("ZZZ", commandHandler_NoReplyTextBundle);
        assertSame("commandHandler", commandHandler_NoReplyTextBundle, stubFtpServer.getCommandHandler("ZZZ"));
    }
    
    /**
     * Test the setCommandHandler() method, for a CommandHandler that implements ReplyTextBundleAware,
     * and whose replyTextBundle attribute is null.
     */
    public void testSetCommandHandler_NullReplyTextBundle() {
        stubFtpServer.setCommandHandler("ZZZ", commandHandler);
        assertSame("commandHandler", commandHandler, stubFtpServer.getCommandHandler("ZZZ"));
        assertSame("replyTextBundle", stubFtpServer.replyTextBundle, commandHandler.getReplyTextBundle());
    }
    
    /**
     * Test the setCommandHandler() method, when the commandName is null
     */
    public void testSetCommandHandler_NullCommandName() {
        CommandHandler commandHandler = (CommandHandler) createMock(CommandHandler.class);
        try {
            stubFtpServer.setCommandHandler(null, commandHandler);
            fail("Expected AssertFailedException");
        }
        catch (AssertFailedException expected) {
            LOG.info("Expected: " + expected);
        }
    }
    
    /**
     * Test the setCommandHandler() method, when the commandHandler is null
     */
    public void testSetCommandHandler_NullCommandHandler() {
        try {
            stubFtpServer.setCommandHandler("ZZZ", null);
            fail("Expected AssertFailedException");
        }
        catch (AssertFailedException expected) {
            LOG.info("Expected: " + expected);
        }
    }
    
    /**
     * Test setReplyTextBaseName() method 
     */
    public void testSetReplyTextBaseName() {
        stubFtpServer.setReplyTextBaseName("SampleReplyText");
        CwdCommandHandler commandHandler = new CwdCommandHandler();

        // The resource bundle is passed along to new CommandHandlers (if they don't already have one) 
        stubFtpServer.setCommandHandler("CWD", commandHandler);
        ResourceBundle resourceBundle = commandHandler.getReplyTextBundle();
        assertEquals("110", "Testing123", resourceBundle.getString("110"));
    }
    
    /**
     * Test the setCommandHandler() and getCommandHandler() methods for commands in lower case or mixed case
     */
    public void testLowerCaseOrMixedCaseCommandNames() {
        stubFtpServer.setCommandHandler("XXX", commandHandler);
        assertSame("ZZZ", commandHandler, stubFtpServer.getCommandHandler("XXX"));
        assertSame("Zzz", commandHandler, stubFtpServer.getCommandHandler("Xxx"));
        assertSame("zzz", commandHandler, stubFtpServer.getCommandHandler("xxx"));

        stubFtpServer.setCommandHandler("YyY", commandHandler);
        assertSame("ZZZ", commandHandler, stubFtpServer.getCommandHandler("YYY"));
        assertSame("Zzz", commandHandler, stubFtpServer.getCommandHandler("Yyy"));
        assertSame("zzz", commandHandler, stubFtpServer.getCommandHandler("yyy"));

        stubFtpServer.setCommandHandler("zzz", commandHandler);
        assertSame("ZZZ", commandHandler, stubFtpServer.getCommandHandler("ZZZ"));
        assertSame("Zzz", commandHandler, stubFtpServer.getCommandHandler("zzZ"));
        assertSame("zzz", commandHandler, stubFtpServer.getCommandHandler("zzz"));
    }
    
    //-------------------------------------------------------------------------
    // Test setup
    //-------------------------------------------------------------------------
    
    /**
     * @see org.mockftpserver.test.AbstractTest#setUp()
     */
    protected void setUp() throws Exception {
        super.setUp();

        stubFtpServer = new StubFtpServer();
        
        // Create a CommandHandler instance that also implements ResourceBundleAware
        commandHandler = new AbstractStubCommandHandler() {
            protected void handleCommand(Command command, Session session, InvocationRecord invocationRecord) throws Exception {
            }
        };

        // Create a CommandHandler instance that does NOT implement ResourceBundleAware
        commandHandler_NoReplyTextBundle = new CommandHandler() {
            public void handleCommand(Command command, Session session) throws Exception {
            }
        };
    }

}
