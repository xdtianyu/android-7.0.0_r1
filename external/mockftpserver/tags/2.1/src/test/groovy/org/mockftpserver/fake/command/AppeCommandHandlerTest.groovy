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
import org.mockftpserver.fake.filesystem.FileEntry
import org.mockftpserver.fake.filesystem.Permissions

/**
 * Tests for AppeCommandHandler
 *
 * @version $Revision$ - $Date$
 *
 * @author Chris Mair
 */
class AppeCommandHandlerTest extends AbstractStoreFileCommandHandlerTest {

    void testHandleCommand_MissingPathParameter() {
        testHandleCommand_MissingRequiredParameter([])
    }

    void testHandleCommand_AbsolutePath() {
        userAccount.defaultPermissionsForNewFile = Permissions.NONE
        testHandleCommand([FILE], 'appe', CONTENTS)
    }

    void testHandleCommand_AbsolutePath_FileAlreadyExists() {
        def ORIGINAL_CONTENTS = '123 456 789'
        fileSystem.add(new FileEntry(path: FILE, contents: ORIGINAL_CONTENTS))
        testHandleCommand([FILE], 'appe', ORIGINAL_CONTENTS + CONTENTS)
    }

    void testHandleCommand_RelativePath() {
        setCurrentDirectory(DIR)
        testHandleCommand([FILENAME], 'appe', CONTENTS)
    }

    void testHandleCommand_PathSpecifiesAnExistingDirectory() {
        createDirectory(FILE)
        handleCommand([FILE])
        assertSessionReply(ReplyCodes.FILENAME_NOT_VALID, FILE)
    }

    void testHandleCommand_ParentDirectoryDoesNotExist() {
        def NO_SUCH_DIR = "/path/DoesNotExist"
        handleCommand([p(NO_SUCH_DIR, FILENAME)])
        assertSessionReply(ReplyCodes.FILENAME_NOT_VALID, NO_SUCH_DIR)
    }

    //-------------------------------------------------------------------------
    // Helper Methods
    //-------------------------------------------------------------------------

    CommandHandler createCommandHandler() {
        new AppeCommandHandler()
    }

    Command createValidCommand() {
        return new Command(CommandNames.APPE, [FILE])
    }

    void setUp() {
        super.setUp()
    }

    protected String verifyOutputFile() {
        assert fileSystem.isFile(FILE)
        assert session.getReplyMessage(1).contains(FILENAME)
        return FILE
    }

}