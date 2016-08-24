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
 * Tests for ReinCommandHandler
 *
 * @version $Revision$ - $Date$
 *
 * @author Chris Mair
 */
class ReinCommandHandlerTest extends AbstractFakeCommandHandlerTest {

    boolean testNotLoggedIn = false

    UserAccount userAccount

    void testHandleCommand_AlreadyLoggedIn() {
        session.setAttribute(SessionKeys.USER_ACCOUNT, userAccount)
        assert isLoggedIn()
        handleCommand([])
        assertSessionReply(ReplyCodes.REIN_OK, 'rein')
        assert !isLoggedIn()
    }

    void testHandleCommand_NotLoggedIn() {
        handleCommand([])
        assertSessionReply(ReplyCodes.REIN_OK, 'rein')
        assert !isLoggedIn()
    }

    //-------------------------------------------------------------------------
    // Helper Methods
    //-------------------------------------------------------------------------

    CommandHandler createCommandHandler() {
        new ReinCommandHandler()
    }

    Command createValidCommand() {
        return new Command(CommandNames.REIN, [])
    }

    void setUp() {
        super.setUp()
        userAccount = new UserAccount(username: 'user')
    }

    private boolean isLoggedIn() {
        return session.getAttribute(SessionKeys.USER_ACCOUNT) != null
    }
}