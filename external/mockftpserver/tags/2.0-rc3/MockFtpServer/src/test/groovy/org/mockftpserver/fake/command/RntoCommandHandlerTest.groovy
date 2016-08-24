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
import org.mockftpserver.fake.filesystem.FileSystemException
import org.mockftpserver.fake.filesystem.Permissions

/**
 * Tests for RntoCommandHandler
 *
 * @version $Revision$ - $Date$
 *
 * @author Chris Mair
 */
class RntoCommandHandlerTest extends AbstractFakeCommandHandlerTest {

    static final DIR = '/'
    static final FROM_FILE = "/from.txt"
    static final TO_FILE = "/file.txt"

    void testHandleCommand() {
        createFile(FROM_FILE)
        handleCommand([TO_FILE])
        assertSessionReply(ReplyCodes.RNTO_OK, ['rnto', FROM_FILE, TO_FILE])
        assert !fileSystem.exists(FROM_FILE), FROM_FILE
        assert fileSystem.exists(TO_FILE), TO_FILE
        assertRenameFromSessionProperty(null)
    }

    void testHandleCommand_PathIsRelative() {
        createFile(FROM_FILE)
        session.setAttribute(SessionKeys.CURRENT_DIRECTORY, "/")
        handleCommand(["file.txt"])
        assertSessionReply(ReplyCodes.RNTO_OK, ['rnto', FROM_FILE, 'file.txt'])
        assert !fileSystem.exists(FROM_FILE), FROM_FILE
        assert fileSystem.exists(TO_FILE), TO_FILE
        assertRenameFromSessionProperty(null)
    }

    void testHandleCommand_FromFileNotSetInSession() {
        session.removeAttribute(SessionKeys.RENAME_FROM)
        testHandleCommand_MissingRequiredSessionAttribute()
    }

    void testHandleCommand_ToFilenameNotValid() {
        createFile(FROM_FILE)
        handleCommand([""])
        assertSessionReply(ReplyCodes.FILENAME_NOT_VALID, "")
        assertRenameFromSessionProperty(FROM_FILE)
    }

    void testHandleCommand_ToFilenameSpecifiesADirectory() {
        createDirectory(TO_FILE)
        handleCommand([TO_FILE])
        assertSessionReply(ReplyCodes.WRITE_FILE_ERROR, ['filesystem.isDirectory', TO_FILE])
        assertRenameFromSessionProperty(FROM_FILE)
    }

    void testHandleCommand_NoWriteAccessToDirectory() {
        createFile(FROM_FILE)
        fileSystem.getEntry(DIR).permissions = new Permissions('r-xr-xr-x')
        handleCommand([TO_FILE])
        assertSessionReply(ReplyCodes.WRITE_FILE_ERROR, ['filesystem.cannotWrite', DIR])
        assertRenameFromSessionProperty(FROM_FILE)
    }

    void testHandleCommand_FromFileDoesNotExist() {
        createDirectory(DIR)
        handleCommand([TO_FILE])
        assertSessionReply(ReplyCodes.FILENAME_NOT_VALID, ['filesystem.pathDoesNotExist', FROM_FILE])
        assertRenameFromSessionProperty(FROM_FILE)
    }

    void testHandleCommand_ToFileParentDirectoryDoesNotExist() {
        createFile(FROM_FILE)
        final BAD_DIR = p(DIR, 'SUB')
        final BAD_TO_FILE = p(BAD_DIR, 'Filename.txt')
        handleCommand([BAD_TO_FILE])
        assertSessionReply(ReplyCodes.FILENAME_NOT_VALID, ['filesystem.pathDoesNotExist', BAD_DIR])
        assertRenameFromSessionProperty(FROM_FILE)
    }

    void testHandleCommand_RenameThrowsException() {
        createDirectory(DIR)
        fileSystem.renameMethodException = new FileSystemException("bad", ERROR_MESSAGE_KEY)

        handleCommand([TO_FILE])
        assertSessionReply(ReplyCodes.WRITE_FILE_ERROR, ERROR_MESSAGE_KEY)
        assertRenameFromSessionProperty(FROM_FILE)
    }

    void testHandleCommand_MissingPathParameter() {
        testHandleCommand_MissingRequiredParameter([])
    }

    //-------------------------------------------------------------------------
    // Helper Methods
    //-------------------------------------------------------------------------

    CommandHandler createCommandHandler() {
        new RntoCommandHandler()
    }

    Command createValidCommand() {
        return new Command(CommandNames.RNTO, [TO_FILE])
    }

    void setUp() {
        super.setUp()
        session.setAttribute(SessionKeys.RENAME_FROM, FROM_FILE)
    }

    private void assertRenameFromSessionProperty(String value) {
        assert session.getAttribute(SessionKeys.RENAME_FROM) == value
    }

}