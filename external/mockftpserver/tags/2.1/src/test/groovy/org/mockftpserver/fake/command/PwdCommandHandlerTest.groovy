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
 * Tests for PwdCommandHandler
 *
 * @version $Revision$ - $Date$
 *
 * @author Chris Mair
 */
class PwdCommandHandlerTest extends AbstractFakeCommandHandlerTest {

    static final DIR = "/usr/abc"

    boolean testNotLoggedIn = false

    void testHandleCommand() {
        session.setAttribute(SessionKeys.CURRENT_DIRECTORY, DIR)
        handleCommand([])
        assertSessionReply(ReplyCodes.PWD_OK, ["pwd", DIR])
    }

    void testHandleCommand_CurrentDirectoryNotSet() {
        handleCommand([])
        assertSessionReply(ReplyCodes.READ_FILE_ERROR, 'filesystem.currentDirectoryNotSet')
    }

    //-------------------------------------------------------------------------
    // Helper Methods
    //-------------------------------------------------------------------------

    void setUp() {
        super.setUp()
    }

    CommandHandler createCommandHandler() {
        new PwdCommandHandler()
    }

    Command createValidCommand() {
        return new Command(CommandNames.PWD, [])
    }

}