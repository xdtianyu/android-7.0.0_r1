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
import org.mockftpserver.core.command.CommandNames
import org.mockftpserver.core.command.ReplyCodes
import org.mockftpserver.fake.filesystem.FileEntry
import org.mockftpserver.fake.filesystem.FileSystemEntry
import org.mockftpserver.fake.filesystem.FileSystemException
import org.mockftpserver.fake.filesystem.Permissions

/**
 * Abstract superclass for tests of Fake CommandHandlers that store a file (STOR, STOU, APPE)
 *
 * @version $Revision$ - $Date$
 *
 * @author Chris Mair
 */
abstract class AbstractStoreFileCommandHandlerTest extends AbstractFakeCommandHandlerTest {

    protected static final DIR = "/"
    protected static final FILENAME = "file.txt"
    protected static final FILE = p(DIR, FILENAME)
    protected static final CONTENTS = "abc"

    //-------------------------------------------------------------------------
    // Tests Common to All Subclasses
    //-------------------------------------------------------------------------

    void testHandleCommand_NoWriteAccessToExistingFile() {
        fileSystem.add(new FileEntry(path: FILE))
        fileSystem.getEntry(FILE).permissions = Permissions.NONE
        handleCommand([FILE])
        assertSessionReply(ReplyCodes.WRITE_FILE_ERROR, ['filesystem.cannotWrite', FILE])
    }

    void testHandleCommand_NoWriteAccessToDirectoryForNewFile() {
        fileSystem.getEntry(DIR).permissions = new Permissions('r-xr-xr-x')
        handleCommand([FILE])
        assertSessionReply(ReplyCodes.WRITE_FILE_ERROR, ['filesystem.cannotWrite', DIR])
    }

    void testHandleCommand_NoExecuteAccessToDirectory() {
        fileSystem.add(new FileEntry(path: FILE))
        fileSystem.getEntry(DIR).permissions = new Permissions('rw-rw-rw-')
        handleCommand([FILE])
        assertSessionReply(ReplyCodes.WRITE_FILE_ERROR, ['filesystem.cannotExecute', DIR])
    }

    void testHandleCommand_ThrowsFileSystemException() {
        fileSystem.addMethodException = new FileSystemException("bad", ERROR_MESSAGE_KEY)

        handleCommand([FILE])
        assertSessionReply(0, ReplyCodes.TRANSFER_DATA_INITIAL_OK)
        assertSessionReply(1, ReplyCodes.WRITE_FILE_ERROR, ERROR_MESSAGE_KEY)
    }

    //-------------------------------------------------------------------------
    // Abstract Method Declarations
    //-------------------------------------------------------------------------

    /**
     * Verify the created output file and return its full path
     * @return the full path to the created output file; the path may be absolute or relative
     */
    protected abstract String verifyOutputFile()

    //-------------------------------------------------------------------------
    // Helper Methods
    //-------------------------------------------------------------------------

    protected void testHandleCommand(List parameters, String messageKey, String contents) {
        session.dataToRead = CONTENTS.bytes
        handleCommand(parameters)
        assertSessionReply(0, ReplyCodes.TRANSFER_DATA_INITIAL_OK)
        assertSessionReply(1, ReplyCodes.TRANSFER_DATA_FINAL_OK, messageKey)

        def outputFile = verifyOutputFile()

        FileSystemEntry fileEntry = fileSystem.getEntry(outputFile)
        def actualContents = fileEntry.createInputStream().text
        assert actualContents == contents
        assert fileEntry.permissions == userAccount.defaultPermissionsForNewFile
    }

    Command createValidCommand() {
        return new Command(CommandNames.APPE, [FILE])
    }

    void setUp() {
        super.setUp()
        createDirectory(DIR)
    }

}