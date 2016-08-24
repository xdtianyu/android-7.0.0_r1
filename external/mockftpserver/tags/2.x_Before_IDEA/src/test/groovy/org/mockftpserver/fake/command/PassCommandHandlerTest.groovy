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
 * Tests for PassCommandHandler
 * 
 * @version $Revision: $ - $Date: $
 *
 * @author Chris Mair
 */
class PassCommandHandlerTest extends AbstractFakeCommandHandlerTest {

    def USERNAME = "user123"
    def PASSWORD = "password123"
    def userAccount
    
    void testHandleCommand_UserExists_PasswordCorrect() {
        serverConfiguration.userAccounts[USERNAME] = userAccount
		commandHandler.handleCommand(createCommand([PASSWORD]), session)
        assertSessionReply(ReplyCodes.PASS_OK)
        assertUserAccountInSession(true)
	}

    void testHandleCommand_UserExists_PasswordIncorrect() {
        serverConfiguration.userAccounts[USERNAME] = userAccount
		commandHandler.handleCommand(createCommand(["wrong"]), session)
        assertSessionReply(ReplyCodes.PASS_LOG_IN_FAILED)
        assertUserAccountInSession(false)
    }

    void testHandleCommand_UserExists_PasswordWrongButIgnored() {
        userAccount.passwordCheckedDuringValidation = false
        serverConfiguration.userAccounts[USERNAME] = userAccount
		commandHandler.handleCommand(createCommand(["wrong"]), session)
        assertSessionReply(ReplyCodes.PASS_OK)
        assertUserAccountInSession(true)
    }
        
    void testHandleCommand_UserDoesNotExist() {
		commandHandler.handleCommand(createCommand([PASSWORD]), session)
        assertSessionReply(ReplyCodes.PASS_LOG_IN_FAILED)
        assertUserAccountInSession(false)
    }

    void testHandleCommand_UsernameNotSetInSession() {
        session.removeAttribute(SessionKeys.USERNAME)
        testHandleCommand_MissingRequiredSessionAttribute()
        assertUserAccountInSession(false)
	}

    void testHandleCommand_MissingPasswordParameter() {
        testHandleCommand_MissingRequiredParameter([])
        assertUserAccountInSession(false)
    }
    
    void testHandleCommand_EmptyPasswordParameter() {
        testHandleCommand_MissingRequiredParameter([""])
        assertUserAccountInSession(false)
    }
    
    //-------------------------------------------------------------------------
    // Helper Methods
    //-------------------------------------------------------------------------
    
	void setUp() {
	    super.setUp()

	    userAccount = new UserAccount()
        userAccount.username = USERNAME
        userAccount.password = PASSWORD
        
        session.setAttribute(SessionKeys.USERNAME, USERNAME)
	}

	CommandHandler createCommandHandler() {
	    new PassCommandHandler()
	}
	
    Command createValidCommand() {
        return new Command(CommandNames.PASS, [PASSWORD])
    }
    
    /**
     * Assert that the UserAccount object is in the session, depending on the value of isUserAccountInSession.
     * @param isUserAccountInSession - true if the UserAccount is expected in the session; false if it is not expected
     */
    private void assertUserAccountInSession(boolean isUserAccountInSession) {
        def expectedValue = isUserAccountInSession ? userAccount : null
        assert session.getAttribute(SessionKeys.USER_ACCOUNT) == expectedValue
    }
}