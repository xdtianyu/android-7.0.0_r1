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
import org.mockftpserver.fake.filesystem.FileEntry
import org.mockftpserver.fake.filesystem.FileSystemException
import org.mockftpserver.fake.filesystem.Permissions

/**
 * Tests for RetrCommandHandler
 *
 * @version $Revision$ - $Date$
 *
 * @author Chris Mair
 */
class RetrCommandHandlerTest extends AbstractFakeCommandHandlerTest {

    def DIR = "/"
    def FILENAME = "file.txt"
    def FILE = p(DIR, FILENAME)
    def CONTENTS = "abc\ndef\nghi"
    def CONTENTS_ASCII = "abc\r\ndef\r\nghi"

    void testHandleCommand_MissingPathParameter() {
        testHandleCommand_MissingRequiredParameter([])
    }

    void testHandleCommand_AbsolutePath() {
        handleCommandAndVerifySendDataReplies([FILE])
        assertSessionData(CONTENTS_ASCII)
    }

    void testHandleCommand_AbsolutePath_NonAsciiMode() {
        session.setAttribute(SessionKeys.ASCII_TYPE, false)
        handleCommandAndVerifySendDataReplies([FILE])
        assertSessionData(CONTENTS)
    }

    void testHandleCommand_RelativePath() {
        setCurrentDirectory(DIR)
        handleCommandAndVerifySendDataReplies([FILENAME])
        assertSessionData(CONTENTS_ASCII)
    }

    void testHandleCommand_PathSpecifiesAnExistingDirectory() {
        handleCommand([DIR])
        assertSessionReply(ReplyCodes.READ_FILE_ERROR, ['filesystem.isNotAFile', DIR])
    }

    void testHandleCommand_PathDoesNotExist() {
        def path = FILE + "XXX"
        handleCommand([path])
        assertSessionReply(ReplyCodes.READ_FILE_ERROR, ['filesystem.pathDoesNotExist', path])
    }

    void testHandleCommand_NoReadAccessToFile() {
        fileSystem.getEntry(FILE).permissions = Permissions.NONE
        handleCommand([FILE])
        assertSessionReply(ReplyCodes.READ_FILE_ERROR, ['filesystem.cannotRead', FILE])
    }

    void testHandleCommand_NoExecuteAccessToDirectory() {
        fileSystem.getEntry(DIR).permissions = Permissions.NONE
        handleCommand([FILE])
        assertSessionReply(ReplyCodes.READ_FILE_ERROR, ['filesystem.cannotExecute', DIR])
    }

    void testHandleCommand_ThrowsFileSystemException() {
        fileSystem.delete(FILE)
        def fileEntry = new BadFileEntry(FILE)
        fileSystem.add(fileEntry)

        handleCommand([FILE])
        assertSessionReply(0, ReplyCodes.TRANSFER_DATA_INITIAL_OK)
        assertSessionReply(1, ReplyCodes.READ_FILE_ERROR, ERROR_MESSAGE_KEY)
    }

    void testConvertLfToCrLf() {
        // LF='\n' and CRLF='\r\n'
        assert commandHandler.convertLfToCrLf('abc'.bytes) == 'abc'.bytes
        assert commandHandler.convertLfToCrLf('abc\r\ndef'.bytes) == 'abc\r\ndef'.bytes
        assert commandHandler.convertLfToCrLf('abc\ndef'.bytes) == 'abc\r\ndef'.bytes
        assert commandHandler.convertLfToCrLf('abc\ndef\nghi'.bytes) == 'abc\r\ndef\r\nghi'.bytes
        assert commandHandler.convertLfToCrLf('\n'.bytes) == '\r\n'.bytes
        assert commandHandler.convertLfToCrLf('\r\nabc\n'.bytes) == '\r\nabc\r\n'.bytes
    }

    //-------------------------------------------------------------------------
    // Helper Methods
    //-------------------------------------------------------------------------

    CommandHandler createCommandHandler() {
        new RetrCommandHandler()
    }

    Command createValidCommand() {
        return new Command(CommandNames.RETR, [FILE])
    }

    void setUp() {
        super.setUp()
        createDirectory(DIR)
        createFile(FILE, CONTENTS)
    }

}

class BadFileEntry extends FileEntry {

    BadFileEntry(String path) {
        super(path)
    }

    InputStream createInputStream() {
        throw new FileSystemException("BAD", AbstractFakeCommandHandlerTest.ERROR_MESSAGE_KEY)
    }
}