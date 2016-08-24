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
 * Tests for NlstCommandHandler
 *
 * @version $Revision$ - $Date$
 *
 * @author Chris Mair
 */
class NlstCommandHandlerTest extends AbstractFakeCommandHandlerTest {

    def DIR = "/usr"

    void testHandleCommand_SingleFile() {
        createFile("/usr/f1.txt")
        handleCommandAndVerifySendDataReplies([DIR])
        assertSessionDataWithEndOfLine("f1.txt")
    }

    void testHandleCommand_FilesAndDirectories() {
        createFile("/usr/f1.txt")
        createDirectory("/usr/OtherFiles")
        createFile("/usr/f2.txt")
        createDirectory("/usr/Archive")
        handleCommandAndVerifySendDataReplies([DIR])

        def EXPECTED = ["f1.txt", "OtherFiles", "f2.txt", "Archive"] as Set
        def actualLines = session.sentData[0].tokenize(endOfLine()) as Set
        LOG.info("actualLines=$actualLines")
        assert actualLines == EXPECTED
        assertSessionDataEndsWithEndOfLine()
    }

    void testHandleCommand_NoPath_UseCurrentDirectory() {
        createFile("/usr/f1.txt")
        session.setAttribute(SessionKeys.CURRENT_DIRECTORY, DIR)
        handleCommandAndVerifySendDataReplies([])
        assertSessionDataWithEndOfLine("f1.txt")
    }

    void testHandleCommand_EmptyDirectory() {
        handleCommandAndVerifySendDataReplies([DIR])
        assertSessionData("")
    }

    void testHandleCommand_PathSpecifiesAFile() {
        createFile("/usr/f1.txt")
        handleCommandAndVerifySendDataReplies(["/usr/f1.txt"])
        assertSessionDataWithEndOfLine("f1.txt")
    }

    void testHandleCommand_PathDoesNotExist() {
        handleCommandAndVerifySendDataReplies(["/DoesNotExist"])
        assertSessionData("")
    }

    void testHandleCommand_NoReadAccessToDirectory() {
        fileSystem.getEntry(DIR).permissions = new Permissions('-wx-wx-wx')
        handleCommand([DIR])
        assertSessionReply(0, ReplyCodes.TRANSFER_DATA_INITIAL_OK)
        assertSessionReply(1, ReplyCodes.READ_FILE_ERROR, ['filesystem.cannotRead', DIR])
    }

    void testHandleCommand_ListNamesThrowsException() {
        fileSystem.listNamesMethodException = new FileSystemException("bad", ERROR_MESSAGE_KEY)
        handleCommand([DIR])
        assertSessionReply(0, ReplyCodes.TRANSFER_DATA_INITIAL_OK)
        assertSessionReply(1, ReplyCodes.SYSTEM_ERROR, ERROR_MESSAGE_KEY)
    }

    //-------------------------------------------------------------------------
    // Helper Methods
    //-------------------------------------------------------------------------

    CommandHandler createCommandHandler() {
        new NlstCommandHandler()
    }

    Command createValidCommand() {
        return new Command(CommandNames.NLST, [DIR])
    }

    void setUp() {
        super.setUp()
        createDirectory(DIR)
    }

}