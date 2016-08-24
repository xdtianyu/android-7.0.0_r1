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

import org.mockftpserver.test.AbstractGroovyTest
import org.mockftpserver.core.command.Command
import org.mockftpserver.core.command.CommandHandler
import org.mockftpserver.core.command.CommandNamesimport org.mockftpserver.core.session.StubSession
import org.mockftpserver.core.session.SessionKeys
import org.mockftpserver.fake.StubServerConfiguration
import org.mockftpserver.fake.user.UserAccount
import org.apache.log4j.Loggerimport org.mockftpserver.core.command.ReplyCodesimport org.mockftpserver.fake.filesystem.FileSystemimport org.mockftpserver.fake.filesystem.FileSystemExceptionimport org.mockftpserver.fake.filesystem.FakeUnixFileSystemimport org.mockftpserver.fake.filesystem.FakeUnixFileSystem
/**
 * Tests for StorCommandHandler
 * 
 * @version $Revision: $ - $Date: $
 *
 * @author Chris Mair
 */
class StorCommandHandlerTest extends AbstractLoginRequiredCommandHandlerTest {

    def DIR = "/"
    def FILENAME = "file.txt"
    def FILE = p(DIR,FILENAME)
    def CONTENTS = "abc"

    void testHandleCommand_MissingPathParameter() {
        testHandleCommand_MissingRequiredParameter([])
    }
    
    void testHandleCommand_AbsolutePath() {
        session.dataToRead = CONTENTS.bytes
        handleCommandAndVerifySendDataReplies([FILE])
        assert fileSystem.isFile(FILE)
        
        def contents = fileSystem.createInputStream(FILE).text
        assert contents == CONTENTS
	}

    void testHandleCommand_RelativePath() {
        setCurrentDirectory(DIR)
        session.dataToRead = CONTENTS.bytes
        handleCommandAndVerifySendDataReplies([FILENAME])
        assert fileSystem.isFile(FILE)
        
        def contents = fileSystem.createInputStream(FILE).text
        assert contents == CONTENTS
	}

    void testHandleCommand_PathSpecifiesAnExistingDirectory() {
        assert fileSystem.createDirectory(FILE)
        commandHandler.handleCommand(createCommand([FILE]), session)        
        assertSessionReply(ReplyCodes.FILENAME_NOT_VALID, FILE)
	}
    
    void testHandleCommand_ParentDirectoryDoesNotExist() {
        def NO_SUCH_DIR = "/path/DoesNotExist"
        handleCommand([p(NO_SUCH_DIR,FILENAME)])
        assertSessionReply(ReplyCodes.FILENAME_NOT_VALID, NO_SUCH_DIR)
	}
    
    void testHandleCommand_CreateOutputStreamThrowsException() {
        // Override createOutputStream() method to throw exception
        def emc = new ExpandoMetaClass(fileSystem.class, false)
        emc.createOutputStream = { String path, boolean append -> 
            println "Calling createOutputStream() - throwing exception"
            throw new FileSystemException("bad") 
        }
        emc.initialize()
        fileSystem.metaClass = emc

        handleCommand([FILE])
        assertSessionReply(0, ReplyCodes.SEND_DATA_INITIAL_OK)
        assertSessionReply(1, ReplyCodes.NEW_FILE_ERROR)
	}
    
    //-------------------------------------------------------------------------
    // Helper Methods
    //-------------------------------------------------------------------------
    
	CommandHandler createCommandHandler() {
	    new StorCommandHandler()
	}
	
    Command createValidCommand() {
        return new Command(CommandNames.STOR, [FILE])
    }

    void setUp() {
        super.setUp()
        assert fileSystem.createDirectory(DIR)
    }
   
}