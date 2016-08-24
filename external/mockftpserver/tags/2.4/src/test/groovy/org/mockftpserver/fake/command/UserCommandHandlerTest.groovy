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
package org.mockftpserver.fake.command

import org.mockftpserver.core.command.Command
import org.mockftpserver.core.command.CommandHandler
import org.mockftpserver.core.command.CommandNames
import org.mockftpserver.core.command.ReplyCodes
import org.mockftpserver.core.session.SessionKeys
import org.mockftpserver.fake.UserAccount

/**
 * Tests for UserCommandHandler
 *
 * @version $Revision$ - $Date$
 *
 * @author Chris Mair
 */
class UserCommandHandlerTest extends AbstractFakeCommandHandlerTestCase {

    static final USERNAME = "user123"
    static final HOME_DIRECTORY = "/"
    UserAccount userAccount

    boolean testNotLoggedIn = false

    void testHandleCommand_UserExists() {
        serverConfiguration.userAccounts[USERNAME] = userAccount
        handleCommand([USERNAME])
        assertSessionReply(ReplyCodes.USER_NEED_PASSWORD_OK, 'user.needPassword')
        assertUsernameInSession(true)
        assertCurrentDirectory(null)
    }

    void testHandleCommand_NoSuchUser() {
        handleCommand([USERNAME])
        // Will return OK, even if username is not recognized
        assertSessionReply(ReplyCodes.USER_NEED_PASSWORD_OK, 'user.needPassword')
        assertUsernameInSession(true)
        assertCurrentDirectory(null)
    }

    void testHandleCommand_PasswordNotRequiredForLogin() {
        userAccount.passwordRequiredForLogin = false
        serverConfiguration.userAccounts[USERNAME] = userAccount

        handleCommand([USERNAME])
        assertSessionReply(ReplyCodes.USER_LOGGED_IN_OK, 'user.loggedIn')
        assert session.getAttribute(SessionKeys.USER_ACCOUNT) == userAccount
        assertUsernameInSession(false)
        assertCurrentDirectory(HOME_DIRECTORY)
    }

    void testHandleCommand_UserExists_HomeDirectoryNotDefinedForUser() {
        userAccount.homeDirectory = ''
        serverConfiguration.userAccounts[USERNAME] = userAccount
        handleCommand([USERNAME])
        assertSessionReply(ReplyCodes.USER_ACCOUNT_NOT_VALID, "login.userAccountNotValid")
        assertUsernameInSession(false)
        assertCurrentDirectory(null)
    }

    void testHandleCommand_UserExists_HomeDirectoryDoesNotExist() {
        userAccount.homeDirectory = '/abc/def'
        serverConfiguration.userAccounts[USERNAME] = userAccount
        handleCommand([USERNAME])
        assertSessionReply(ReplyCodes.USER_ACCOUNT_NOT_VALID, "login.homeDirectoryNotValid")
        assertUsernameInSession(false)
        assertCurrentDirectory(null)
    }

    void testHandleCommand_MissingUsernameParameter() {
        testHandleCommand_MissingRequiredParameter([])
        assertUsernameInSession(false)
        assertCurrentDirectory(null)
    }

    //-------------------------------------------------------------------------
    // Abstract and Overridden Methods
    //-------------------------------------------------------------------------

    void setUp() {
        super.setUp()

        createDirectory(HOME_DIRECTORY)
        userAccount = new UserAccount(username: USERNAME, homeDirectory: HOME_DIRECTORY)
    }

    CommandHandler createCommandHandler() {
        new UserCommandHandler()
    }

    Command createValidCommand() {
        return new Command(CommandNames.USER, [USERNAME])
    }

    //-------------------------------------------------------------------------
    // Helper Methods
    //-------------------------------------------------------------------------

    /**
     * Assert that the Username is stored in the session, depending on the value of isUsernameInSession.
     * @param isUsernameInSession - true if the Username is expected in the session; false if it is not expected
     */
    private void assertUsernameInSession(boolean isUsernameInSession) {
        def expectedValue = isUsernameInSession ? USERNAME : null
        assert session.getAttribute(SessionKeys.USERNAME) == expectedValue
    }

    /**
     * Assert that the current directory is set in the session, but only if currentDirectory is not null.
     * @param currentDirectory - the curent directory expected in the session; null if it is not expected
     */
    private void assertCurrentDirectory(String currentDirectory) {
        assert session.getAttribute(SessionKeys.CURRENT_DIRECTORY) == currentDirectory
    }
}