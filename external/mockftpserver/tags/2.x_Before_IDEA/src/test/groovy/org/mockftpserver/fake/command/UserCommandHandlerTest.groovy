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

import org.mockftpserver.test.AbstractGroovyTest
import org.mockftpserver.core.command.Command
import org.mockftpserver.core.command.CommandHandler
import org.mockftpserver.core.command.CommandNamesimport org.mockftpserver.core.session.StubSession
import org.mockftpserver.core.session.SessionKeys
import org.mockftpserver.fake.StubServerConfiguration
import org.mockftpserver.fake.user.UserAccount
import org.apache.log4j.Loggerimport org.mockftpserver.core.command.ReplyCodes
/**
 * Tests for UserCommandHandler
 * 
 * @version $Revision: $ - $Date: $
 *
 * @author Chris Mair
 */
class UserCommandHandlerTest extends AbstractFakeCommandHandlerTest {

    def USERNAME = "user123"
    
    void testHandleCommand_UserExists() {
        serverConfiguration.userAccounts[USERNAME] = new UserAccount()
		commandHandler.handleCommand(createCommand([USERNAME]), session)
        assertSessionReply(ReplyCodes.USER_NEED_PASSWORD_OK)
        assertUsernameInSession(true)
	}

    void testHandleCommand_NoSuchUser() {
		commandHandler.handleCommand(createCommand([USERNAME]), session)
		// Will return OK, even if username is not recognized
        assertSessionReply(ReplyCodes.USER_NEED_PASSWORD_OK) 
        assertUsernameInSession(true)
	}

    void testHandleCommand_PasswordNotRequiredForLogin() {
        def userAccount = new UserAccount(passwordRequiredForLogin:false)
        serverConfiguration.userAccounts[USERNAME] = userAccount

        commandHandler.handleCommand(createCommand([USERNAME]), session)
        assertSessionReply(ReplyCodes.USER_LOGGED_IN_OK)
        assert session.getAttribute(SessionKeys.USER_ACCOUNT) == userAccount
        assertUsernameInSession(false)
	}

    void testHandleCommand_MissingUsernameParameter() {
        testHandleCommand_MissingRequiredParameter([])
        assertUsernameInSession(false)
    }
    
    void testHandleCommand_EmptyUsernameParameter() {
        testHandleCommand_MissingRequiredParameter([""])
        assertUsernameInSession(false)
    }

    //-------------------------------------------------------------------------
    // Helper Methods
    //-------------------------------------------------------------------------
    
	CommandHandler createCommandHandler() {
	    new UserCommandHandler()
	}
	
    Command createValidCommand() {
        return new Command(CommandNames.USER, [USERNAME])
    }
    
    /**
     * Assert that the Username is stored in the session, depending on the value of isUsernameInSession.
     * @param isUsernameInSession - true if the Username is expected in the session; false if it is not expected
     */
    private void assertUsernameInSession(boolean isUsernameInSession) {
        def expectedValue = isUsernameInSession ? USERNAME : null
        assert session.getAttribute(SessionKeys.USERNAME) == expectedValue
    }
}