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
 * Tests for PassCommandHandler
 *
 * @version $Revision$ - $Date$
 *
 * @author Chris Mair
 */
class PassCommandHandlerTest extends AbstractFakeCommandHandlerTest {

    def USERNAME = "user123"
    def PASSWORD = "password123"
    def HOME_DIRECTORY = "/"
    UserAccount userAccount

    boolean testNotLoggedIn = false

    void testHandleCommand_UserExists_PasswordCorrect() {
        serverConfiguration.userAccounts[USERNAME] = userAccount
        handleCommand([PASSWORD])
        assertSessionReply(ReplyCodes.PASS_OK, 'pass')
        assertUserAccountInSession(true)
        assertCurrentDirectory(HOME_DIRECTORY)
    }

    void testHandleCommand_UserExists_PasswordCorrect_AccountRequired() {
        serverConfiguration.userAccounts[USERNAME] = userAccount
        userAccount.accountRequiredForLogin = true
        handleCommand([PASSWORD])
        assertSessionReply(ReplyCodes.PASS_NEED_ACCOUNT, 'pass.needAccount')
        assertUserAccountInSession(true)
        assertCurrentDirectory(HOME_DIRECTORY)
    }

    void testHandleCommand_UserExists_PasswordIncorrect() {
        serverConfiguration.userAccounts[USERNAME] = userAccount
        handleCommand(["wrong"])
        assertSessionReply(ReplyCodes.PASS_LOG_IN_FAILED, 'pass.loginFailed')
        assertUserAccountInSession(false)
        assertCurrentDirectory(null)
    }

    void testHandleCommand_UserExists_PasswordWrongButIgnored() {
        userAccount.passwordCheckedDuringValidation = false
        serverConfiguration.userAccounts[USERNAME] = userAccount
        handleCommand(["wrong"])
        assertSessionReply(ReplyCodes.PASS_OK, 'pass')
        assertUserAccountInSession(true)
        assertCurrentDirectory(HOME_DIRECTORY)
    }

    void testHandleCommand_UserExists_HomeDirectoryNotDefinedForUserAccount() {
        userAccount.homeDirectory = ''
        serverConfiguration.userAccounts[USERNAME] = userAccount
        handleCommand([PASSWORD])
        assertSessionReply(ReplyCodes.USER_ACCOUNT_NOT_VALID, "login.userAccountNotValid")
        assertUserAccountInSession(false)
        assertCurrentDirectory(null)
    }

    void testHandleCommand_UserExists_HomeDirectoryDoesNotExist() {
        userAccount.homeDirectory = '/abc/def'
        serverConfiguration.userAccounts[USERNAME] = userAccount
        handleCommand([PASSWORD])
        assertSessionReply(ReplyCodes.USER_ACCOUNT_NOT_VALID, "login.homeDirectoryNotValid")
        assertUserAccountInSession(false)
        assertCurrentDirectory(null)
    }

    void testHandleCommand_UserDoesNotExist() {
        handleCommand([PASSWORD])
        assertSessionReply(ReplyCodes.USER_ACCOUNT_NOT_VALID, "login.userAccountNotValid")
        assertUserAccountInSession(false)
        assertCurrentDirectory(null)
    }

    void testHandleCommand_UsernameNotSetInSession() {
        session.removeAttribute(SessionKeys.USERNAME)
        testHandleCommand_MissingRequiredSessionAttribute()
        assertUserAccountInSession(false)
        assertCurrentDirectory(null)
    }

    void testHandleCommand_MissingPasswordParameter() {
        testHandleCommand_MissingRequiredParameter([])
        assertUserAccountInSession(false)
        assertCurrentDirectory(null)
    }

    //-------------------------------------------------------------------------
    // Abstract and Overridden Methods
    //-------------------------------------------------------------------------

    void setUp() {
        super.setUp()

        createDirectory(HOME_DIRECTORY)

        userAccount = new UserAccount(USERNAME, PASSWORD, HOME_DIRECTORY)

        session.setAttribute(SessionKeys.USERNAME, USERNAME)
        session.removeAttribute(SessionKeys.USER_ACCOUNT)
    }

    CommandHandler createCommandHandler() {
        new PassCommandHandler()
    }

    Command createValidCommand() {
        return new Command(CommandNames.PASS, [PASSWORD])
    }

    //-------------------------------------------------------------------------
    // Helper Methods
    //-------------------------------------------------------------------------

    /**
     * Assert that the UserAccount object is in the session, depending on the value of isUserAccountInSession.
     * @param isUserAccountInSession - true if the UserAccount is expected in the session; false if it is not expected
     */
    private void assertUserAccountInSession(boolean isUserAccountInSession) {
        def expectedValue = isUserAccountInSession ? userAccount : null
        assert session.getAttribute(SessionKeys.USER_ACCOUNT) == expectedValue
    }

    /**
     * Assert that the current directory is set in the session, but only if currentDirectory is not null.
     * @param currentDirectory - the curent directory expected in the session; null if it is not expected
     */
    private void assertCurrentDirectory(String currentDirectory) {
        assert session.getAttribute(SessionKeys.CURRENT_DIRECTORY) == currentDirectory
    }
}