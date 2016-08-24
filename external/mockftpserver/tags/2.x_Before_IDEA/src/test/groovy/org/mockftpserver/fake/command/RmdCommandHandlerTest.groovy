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
import org.mockftpserver.core.command.ReplyCodes
/**
 * Tests for RmdCommandHandler
 * 
 * @version $Revision: $ - $Date: $
 *
 * @author Chris Mair
 */
class RmdCommandHandlerTest extends AbstractLoginRequiredCommandHandlerTest {

    def DIR = "/usr"
    
    void testHandleCommand() {
        assert fileSystem.createDirectory(DIR)
        commandHandler.handleCommand(createCommand([DIR]), session)        
        assertSessionReply(ReplyCodes.RMD_OK)
        assert fileSystem.exists(DIR) == false
	}
    
    void testHandleCommand_PathIsRelative() {
        def SUB = "sub"
        assert fileSystem.createDirectory(p(DIR,SUB))
        session.setAttribute(SessionKeys.CURRENT_DIRECTORY, DIR)
        commandHandler.handleCommand(createCommand([SUB]), session)        
        assertSessionReply(ReplyCodes.RMD_OK)
        assert fileSystem.exists(p(DIR,SUB)) == false
	}

    void testHandleCommand_PathDoesNotExistInFileSystem() {
        commandHandler.handleCommand(createCommand([DIR]), session)        
        assertSessionReply(ReplyCodes.EXISTING_FILE_ERROR, DIR)
	}
    
    void testHandleCommand_PathSpecifiesAFile() {
        assert fileSystem.createFile(DIR)
        commandHandler.handleCommand(createCommand([DIR]), session)        
        assertSessionReply(ReplyCodes.EXISTING_FILE_ERROR, DIR)
        assert fileSystem.exists(DIR)
	}
    
    void testHandleCommand_DirectoryIsNotEmpty() {
        final FILE = DIR + "/file.txt"
        assert fileSystem.createFile(FILE)
        commandHandler.handleCommand(createCommand([DIR]), session)        
        assertSessionReply(ReplyCodes.EXISTING_FILE_ERROR, DIR)
        assert fileSystem.exists(DIR)
        assert fileSystem.exists(FILE)
	}
    
    void testHandleCommand_MissingPathParameter() {
        testHandleCommand_MissingRequiredParameter([])
    }
    
    //-------------------------------------------------------------------------
    // Helper Methods
    //-------------------------------------------------------------------------
    
	CommandHandler createCommandHandler() {
	    new RmdCommandHandler()
	}
	
    Command createValidCommand() {
        return new Command(CommandNames.RMD, [DIR])
    }

}