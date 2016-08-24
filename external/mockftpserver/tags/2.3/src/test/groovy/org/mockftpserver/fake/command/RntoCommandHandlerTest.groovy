/*
 * Copyright 2010 the original author or authors.
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
class RntoCommandHandlerTest extends AbstractFakeCommandHandlerTestCase {

    private static final DIR = '/'
    private static final FROM_FILE = "/from.txt"
    private static final TO_FILE = "/file.txt"
    private static final FROM_DIR = "/subdir"

    void testHandleCommand_SingleFile() {
        createFile(FROM_FILE)
        handleCommand([TO_FILE])
        assertSessionReply(ReplyCodes.RNTO_OK, ['rnto', FROM_FILE, TO_FILE])
        assert !fileSystem.exists(FROM_FILE), FROM_FILE
        assert fileSystem.exists(TO_FILE), TO_FILE
        assertRenameFromSessionProperty(null)
    }

    void testHandleCommand_SingleFile_PathIsRelative() {
        createFile(FROM_FILE)
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

    void testHandleCommand_EmptyDirectory() {
        final TO_DIR = "/newdir"
        createDirectory(FROM_DIR)
        setRenameFromSessionProperty(FROM_DIR)
        handleCommand([TO_DIR])
        assertSessionReply(ReplyCodes.RNTO_OK, ['rnto', FROM_DIR, TO_DIR])
        assert !fileSystem.exists(FROM_DIR), FROM_DIR
        assert fileSystem.exists(TO_DIR), TO_DIR
        assertRenameFromSessionProperty(null)
    }

    void testHandleCommand_DirectoryContainingFilesAndSubdirectory() {
        final TO_DIR = "/newdir"
        createDirectory(FROM_DIR)
        createFile(FROM_DIR + "/a.txt")
        createFile(FROM_DIR + "/b.txt")
        createDirectory(FROM_DIR + "/child/grandchild")
        setRenameFromSessionProperty(FROM_DIR)
        handleCommand([TO_DIR])
        assertSessionReply(ReplyCodes.RNTO_OK, ['rnto', FROM_DIR, TO_DIR])
        assert !fileSystem.exists(FROM_DIR), FROM_DIR
        assert fileSystem.exists(TO_DIR), TO_DIR
        assert fileSystem.isFile(TO_DIR + "/a.txt")
        assert fileSystem.isFile(TO_DIR + "/b.txt")
        assert fileSystem.isDirectory(TO_DIR + "/child")
        assert fileSystem.isDirectory(TO_DIR + "/child/grandchild")
        assertRenameFromSessionProperty(null)
    }

    void testHandleCommand_ToDirectoryIsChildOfFromDirectory() {
        final TO_DIR = FROM_DIR + "/child"
        createDirectory(FROM_DIR)
        setRenameFromSessionProperty(FROM_DIR)
        handleCommand([TO_DIR])
        assertSessionReply(ReplyCodes.WRITE_FILE_ERROR, ['filesystem.renameFailed', TO_DIR])
        assertRenameFromSessionProperty(FROM_DIR)
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
        assertSessionReply(ReplyCodes.FILENAME_NOT_VALID, ['filesystem.doesNotExist', FROM_FILE])
        assertRenameFromSessionProperty(FROM_FILE)
    }

    void testHandleCommand_ToFileParentDirectoryDoesNotExist() {
        createFile(FROM_FILE)
        final BAD_DIR = p(DIR, 'SUB')
        final BAD_TO_FILE = p(BAD_DIR, 'Filename.txt')
        handleCommand([BAD_TO_FILE])
        assertSessionReply(ReplyCodes.FILENAME_NOT_VALID, ['filesystem.doesNotExist', BAD_DIR])
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
        setCurrentDirectory(DIR)
        setRenameFromSessionProperty(FROM_FILE)
    }

    private void setRenameFromSessionProperty(String renameFrom) {
        session.setAttribute(SessionKeys.RENAME_FROM, renameFrom)
    }

    private void assertRenameFromSessionProperty(String value) {
        assert session.getAttribute(SessionKeys.RENAME_FROM) == value
    }

}