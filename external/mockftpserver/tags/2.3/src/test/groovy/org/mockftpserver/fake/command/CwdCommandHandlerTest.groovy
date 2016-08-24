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
import org.mockftpserver.fake.filesystem.Permissions

/**
 * Tests for CwdCommandHandler
 *
 * @version $Revision$ - $Date$
 *
 * @author Chris Mair
 */
class CwdCommandHandlerTest extends AbstractFakeCommandHandlerTestCase {

    def DIR = "/usr"

    void testHandleCommand() {
        createDirectory(DIR)
        handleCommand([DIR])
        assertSessionReply(ReplyCodes.CWD_OK, ['cwd', DIR])
        assert session.getAttribute(SessionKeys.CURRENT_DIRECTORY) == DIR
    }

    void testHandleCommand_PathIsRelative() {
        def SUB = "sub"
        createDirectory(p(DIR, SUB))
        session.setAttribute(SessionKeys.CURRENT_DIRECTORY, DIR)
        handleCommand([SUB])
        assertSessionReply(ReplyCodes.CWD_OK, ['cwd', SUB])
        assert session.getAttribute(SessionKeys.CURRENT_DIRECTORY) == p(DIR, SUB)
    }

    void testHandleCommand_PathDoesNotExistInFileSystem() {
        handleCommand([DIR])
        assertSessionReply(ReplyCodes.READ_FILE_ERROR, ['filesystem.doesNotExist', DIR])
        assert session.getAttribute(SessionKeys.CURRENT_DIRECTORY) == null
    }

    void testHandleCommand_PathSpecifiesAFile() {
        createFile(DIR)
        handleCommand([DIR])
        assertSessionReply(ReplyCodes.READ_FILE_ERROR, ['filesystem.isNotADirectory', DIR])
        assert session.getAttribute(SessionKeys.CURRENT_DIRECTORY) == null
    }

    void testHandleCommand_NoExecuteAccessToParentDirectory() {
        def dir = createDirectory(DIR)
        dir.permissions = new Permissions('rw-rw-rw-')
        handleCommand([DIR])
        assertSessionReply(ReplyCodes.READ_FILE_ERROR, ['filesystem.cannotExecute', DIR])
        assert session.getAttribute(SessionKeys.CURRENT_DIRECTORY) == null
    }

    void testHandleCommand_MissingPathParameter() {
        testHandleCommand_MissingRequiredParameter([])
    }

    //-------------------------------------------------------------------------
    // Helper Methods
    //-------------------------------------------------------------------------

    CommandHandler createCommandHandler() {
        new CwdCommandHandler()
    }

    Command createValidCommand() {
        return new Command(CommandNames.CWD, [DIR])
    }

    void setUp() {
        super.setUp()
        userAccount.username = 'user'
    }

}