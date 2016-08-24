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
 * Tests for RnfrCommandHandler
 *
 * @version $Revision$ - $Date$
 *
 * @author Chris Mair
 */
class RnfrCommandHandlerTest extends AbstractFakeCommandHandlerTest {

    def FILE = "/file.txt"

    void testHandleCommand() {
        createFile(FILE)
        handleCommand([FILE])
        assertSessionReply(ReplyCodes.RNFR_OK, 'rnfr')
        assert session.getAttribute(SessionKeys.RENAME_FROM) == FILE
    }

    void testHandleCommand_PathIsRelative() {
        createFile(FILE)
        session.setAttribute(SessionKeys.CURRENT_DIRECTORY, "/")
        handleCommand(["file.txt"])
        assertSessionReply(ReplyCodes.RNFR_OK, 'rnfr')
        assert session.getAttribute(SessionKeys.RENAME_FROM) == FILE
    }

    void testHandleCommand_PathDoesNotExistInFileSystem() {
        handleCommand([FILE])
        assertSessionReply(ReplyCodes.READ_FILE_ERROR, ['filesystem.doesNotExist', FILE])
        assert session.getAttribute(SessionKeys.RENAME_FROM) == null
    }

    void testHandleCommand_PathSpecifiesADirectory() {
        createDirectory(FILE)
        handleCommand([FILE])
        assertSessionReply(ReplyCodes.READ_FILE_ERROR, ['filesystem.isNotAFile', FILE])
        assert session.getAttribute(SessionKeys.RENAME_FROM) == null
    }

    void testHandleCommand_NoReadAccessToFile() {
        createFile(FILE)
        fileSystem.getEntry(FILE).permissions = new Permissions('-wx-wx-wx')
        handleCommand([FILE])
        assertSessionReply(ReplyCodes.READ_FILE_ERROR, ['filesystem.cannotRead', FILE])
    }

    void testHandleCommand_MissingPathParameter() {
        testHandleCommand_MissingRequiredParameter([])
    }

    //-------------------------------------------------------------------------
    // Helper Methods
    //-------------------------------------------------------------------------

    CommandHandler createCommandHandler() {
        new RnfrCommandHandler()
    }

    Command createValidCommand() {
        return new Command(CommandNames.RNFR, [FILE])
    }

}