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

/**
 * Tests for AcctCommandHandler
 *
 * @version $Revision$ - $Date$
 *
 * @author Chris Mair
 */
class AcctCommandHandlerTest extends AbstractFakeCommandHandlerTestCase {

    def USERNAME = "user123"
    def ACCOUNT_NAME = "account123"

    boolean testNotLoggedIn = false

    void testHandleCommand() {
        handleCommand([ACCOUNT_NAME])
        assertSessionReply(ReplyCodes.ACCT_OK, ['acct', USERNAME])
        assertAccountNameInSession(true)
    }

    void testHandleCommand_UsernameNotSetInSession() {
        session.removeAttribute(SessionKeys.USERNAME)
        testHandleCommand_MissingRequiredSessionAttribute()
        assertAccountNameInSession(false)
    }

    void testHandleCommand_MissingAccountNameParameter() {
        testHandleCommand_MissingRequiredParameter([])
        assertAccountNameInSession(false)
    }

    //-------------------------------------------------------------------------
    // Abstract and Overridden Methods
    //-------------------------------------------------------------------------

    void setUp() {
        super.setUp()
        session.setAttribute(SessionKeys.USERNAME, USERNAME)
    }

    CommandHandler createCommandHandler() {
        new AcctCommandHandler()
    }

    Command createValidCommand() {
        return new Command(CommandNames.ACCT, [ACCOUNT_NAME])
    }

    //-------------------------------------------------------------------------
    // Helper Methods
    //-------------------------------------------------------------------------

    /**
     * Assert that the account name is stored in the session, depending on the value of isAccountNameInSession.
     * @param isAccountNameInSession - true if the account name is expected in the session; false if it is not expected
     */
    private void assertAccountNameInSession(boolean isAccountNameInSession) {
        def expectedValue = isAccountNameInSession ? ACCOUNT_NAME : null
        assert session.getAttribute(SessionKeys.ACCOUNT_NAME) == expectedValue
    }

}