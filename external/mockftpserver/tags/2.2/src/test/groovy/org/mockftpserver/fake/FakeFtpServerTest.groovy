/*
 * Copyright 2008 the original author or authors.
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
package org.mockftpserver.fake

import org.mockftpserver.core.command.Command
import org.mockftpserver.core.command.CommandHandler
import org.mockftpserver.core.command.ReplyTextBundleAware
import org.mockftpserver.core.server.AbstractFtpServer
import org.mockftpserver.core.server.AbstractFtpServerTestCase
import org.mockftpserver.core.session.Session

/**
 * Tests for FakeFtpServer.
 *
 * @version $Revision$ - $Date$
 *
 * @author Chris Mair
 */
class FakeFtpServerTest extends AbstractFtpServerTestCase {

    def commandHandler
    def commandHandler_NotServerConfigurationAware

    //-------------------------------------------------------------------------
    // Extra tests  (Standard tests defined in superclass)
    //-------------------------------------------------------------------------

    void testSetCommandHandler_NotServerConfigurationAware() {
        ftpServer.setCommandHandler("ZZZ", commandHandler_NotServerConfigurationAware)
        assert ftpServer.getCommandHandler("ZZZ") == commandHandler_NotServerConfigurationAware
    }

    void testSetCommandHandler_ServerConfigurationAware() {
        ftpServer.setCommandHandler("ZZZ", commandHandler)
        assert ftpServer.getCommandHandler("ZZZ") == commandHandler
        assert ftpServer == commandHandler.serverConfiguration
    }

    void testSetCommandHandler_ReplyTextBundleAware() {
        def cmdHandler = new TestCommandHandlerReplyTextBundleAware()
        ftpServer.setCommandHandler("ZZZ", cmdHandler)
        assert ftpServer.getCommandHandler("ZZZ") == cmdHandler
        assert ftpServer.replyTextBundle == cmdHandler.replyTextBundle
    }

    void testUserAccounts() {
        def userAccount = new UserAccount(username: 'abc')

        // addUserAccount()
        ftpServer.addUserAccount(userAccount)
        assert ftpServer.getUserAccount("abc") == userAccount

        // setUserAccounts
        def userAccounts = [userAccount]
        ftpServer.userAccounts = userAccounts
        assert ftpServer.getUserAccount("abc") == userAccount
    }

    void testHelpText() {
        ftpServer.helpText = [a: 'aaaaa', b: 'bbbbb', '': 'default']
        assert ftpServer.getHelpText('a') == 'aaaaa'
        assert ftpServer.getHelpText('b') == 'bbbbb'
        assert ftpServer.getHelpText('') == 'default'
        assert ftpServer.getHelpText('unrecognized') == null
    }

    void testSystemName() {
        assert ftpServer.systemName == "WINDOWS"
        ftpServer.systemName = "abc"
        assert ftpServer.systemName == "abc"
    }

    void testSystemStatus() {
        assert ftpServer.systemStatus == "Connected"
        ftpServer.systemStatus = "abc"
        assert ftpServer.systemStatus == "abc"
    }

    void testReplyText() {
        ftpServer.replyTextBaseName = "SampleReplyText"

        ResourceBundle resourceBundle = ftpServer.replyTextBundle
        assert resourceBundle.getString("110") == "Testing123"
    }

    //-------------------------------------------------------------------------
    // Test set up
    //-------------------------------------------------------------------------

    void setUp() {
        super.setUp();
        commandHandler = new TestCommandHandler()
        commandHandler_NotServerConfigurationAware = new TestCommandHandlerNotServerConfigurationAware()
    }

    //-------------------------------------------------------------------------
    // Abstract method implementations
    //-------------------------------------------------------------------------

    protected AbstractFtpServer createFtpServer() {
        return new FakeFtpServer();
    }

    protected CommandHandler createCommandHandler() {
        return new TestCommandHandler();
    }

    protected void verifyCommandHandlerInitialized(CommandHandler commandHandler) {
        //To change body of implemented methods use File | Settings | File Templates.
    }

}
class TestCommandHandlerReplyTextBundleAware implements CommandHandler, ReplyTextBundleAware {
    ResourceBundle replyTextBundle

    public void handleCommand(Command command, Session session) {
    }

}