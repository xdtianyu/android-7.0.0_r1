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

import org.mockftpserver.core.command.Command;
import org.mockftpserver.core.command.CommandHandler;
import org.mockftpserver.core.command.InvocationRecord;
import org.mockftpserver.core.server.*;
import org.mockftpserver.core.server.AbstractFtpServerTestCase;
import org.mockftpserver.core.session.Session;
import org.mockftpserver.stub.command.AbstractStubCommandHandler;
import org.mockftpserver.stub.command.CwdCommandHandler;

import java.util.ResourceBundle;

/**
 * Unit tests for StubFtpServer. Also see {@link StubFtpServer_StartTest}
 * and {@link StubFtpServerIntegrationTest}.
 *
 * @author Chris Mair
 * @version $Revision$ - $Date$
 */
public final class StubFtpServerTest extends AbstractFtpServerTestCase {

    private StubFtpServer stubFtpServer;
    private AbstractStubCommandHandler commandHandler;
    private CommandHandler commandHandler_NoReplyTextBundle;

    //-------------------------------------------------------------------------
    // Extra tests  (Standard tests defined in superclass)
    //-------------------------------------------------------------------------

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
        assertSame("replyTextBundle", stubFtpServer.getReplyTextBundle(), commandHandler.getReplyTextBundle());
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

    //-------------------------------------------------------------------------
    // Test setup
    //-------------------------------------------------------------------------

    /**
     * @see org.mockftpserver.test.AbstractTestCase#setUp()
     */
    protected void setUp() throws Exception {
        super.setUp();

        stubFtpServer = (StubFtpServer) ftpServer;

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

    //-------------------------------------------------------------------------
    // Abstract method implementations
    //-------------------------------------------------------------------------

    protected AbstractFtpServer createFtpServer() {
        return new StubFtpServer();
    }

    protected CommandHandler createCommandHandler() {
        return new AbstractStubCommandHandler() {
            protected void handleCommand(Command command, Session session, InvocationRecord invocationRecord) throws Exception {
            }
        };
    }

    protected void verifyCommandHandlerInitialized(CommandHandler commandHandler) {
        AbstractStubCommandHandler stubCommandHandler = (AbstractStubCommandHandler) commandHandler;
        assertSame("replyTextBundle", stubFtpServer.getReplyTextBundle(), stubCommandHandler.getReplyTextBundle());
    }

}
