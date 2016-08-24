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
import org.apache.log4j.Loggerimport org.mockftpserver.core.command.ReplyCodes
import org.mockftpserver.fake.user.UserAccount
import org.mockftpserver.fake.filesystem.FakeUnixFileSystem

/**
 * Abstract superclass for CommandHandler tests
 * 
 * @version $Revision: $ - $Date: $
 *
 * @author Chris Mair
 */
abstract class AbstractFakeCommandHandlerTest extends AbstractGroovyTest {

    protected session
    protected serverConfiguration
    protected commandHandler
    protected fileSystem
    
    //-------------------------------------------------------------------------
    // Tests (common to all subclasses)
    //-------------------------------------------------------------------------
    
    void testHandleCommand_ServerConfigurationIsNull() {
        commandHandler.serverConfiguration = null
        def command = createValidCommand()
        shouldFailWithMessageContaining("serverConfiguration") { commandHandler.handleCommand(command, session) }
    }
    
    void testHandleCommand_CommandIsNull() {
        shouldFailWithMessageContaining("command") { commandHandler.handleCommand(null, session) }
    }
    
    void testHandleCommand_SessionIsNull() {
        def command = createValidCommand()
        shouldFailWithMessageContaining("session") { commandHandler.handleCommand(command, null) }
    }
    
    //-------------------------------------------------------------------------
    // Abstract Method Declarations (must be implemented by all subclasses)
    //-------------------------------------------------------------------------
    
    /**
     * Create and return a new instance of the CommandHandler class under test. Concrete subclasses must implement.
     */
    abstract CommandHandler createCommandHandler()
    
    /**
     * Create and return a valid instance of the Command for the CommandHandler class 
     * under test. Concrete subclasses must implement.
     */
    abstract Command createValidCommand()
    
    //-------------------------------------------------------------------------
    // Test Setup
    //-------------------------------------------------------------------------
    
	void setUp() {
	    super.setUp()
	    session = new StubSession()
	    serverConfiguration = new StubServerConfiguration()
	    fileSystem = new FakeUnixFileSystem()
	    fileSystem.createParentDirectoriesAutomatically = true
	    serverConfiguration.setFileSystem(fileSystem)
	    
	    commandHandler = createCommandHandler()
	    commandHandler.serverConfiguration = serverConfiguration
	}

    //-------------------------------------------------------------------------
    // Helper Methods
    //-------------------------------------------------------------------------

    /**
     * Perform a test of the handleCommand() method on the specified command
     * parameters, which are missing a required parameter for this CommandHandler.
     */
    protected void testHandleCommand_MissingRequiredParameter(List commandParameters) {
		commandHandler.handleCommand(createCommand(commandParameters), session)
        assertSessionReply(ReplyCodes.COMMAND_SYNTAX_ERROR)
    }
    
    /**
     * Perform a test of the handleCommand() method on the specified command
     * parameters, which are missing a required parameter for this CommandHandler.
     */
    protected testHandleCommand_MissingRequiredSessionAttribute() {
        def command = createValidCommand()
		commandHandler.handleCommand(command, session)
        assertSessionReply(ReplyCodes.ILLEGAL_STATE)
    }

    /**
     * @return a new Command with the specified parameters for this CommandHandler
     */
    protected Command createCommand(List commandParameters) {
        new Command(createValidCommand().name, commandParameters)
    }
    
    /**
     * Invoke the handleCommand() method for the current CommandHandler, passing in
     * the specified parameters
     * @param parameters - the List of command parameters; may be empty, but not null
     */
    protected void handleCommand(List parameters) {
        commandHandler.handleCommand(createCommand(parameters), session)
    }
    
    /**
     * Assert that the specified reply code and message containing text was sent through the session.
     * @param expectedReplyCode - the expected reply code
     * @param text - the text expected within the reply message; defaults to the reply code as a String
     */
    protected assertSessionReply(int expectedReplyCode, String text=expectedReplyCode as String) {
        assertSessionReply(0, expectedReplyCode, text)
    }
     
    /**
     * Assert that the specified reply code and message containing text was sent through the session.
     * @param replyIndex - the index of the reply to compare
     * @param expectedReplyCode - the expected reply code
     * @param text - the text expected within the reply message; defaults to the reply code as a String
     */
    protected assertSessionReply(int replyIndex, int expectedReplyCode, String text=expectedReplyCode as String) {
		LOG.info(session)
		def actual = session.sentReplies[replyIndex]
		assert actual, "No reply for index [$replyIndex] sent for $session"
		def actualReplyCode = actual[0]
		def actualMessage = actual[1]
		assert actualReplyCode == expectedReplyCode
		assert actualMessage.contains(text), "[$actualMessage] does not contain[$text]"
    }
     
    /**
     * Assert that the specified data was sent through the session.
     * @param expectedData - the expected data
     */
    protected assertSessionData(String expectedData) {
		def actual = session.sentData[0]
		assert actual != null, "No data for index [0] sent for $session"
        assert actual == expectedData
    }
     
    /**
     * Execute the handleCommand() method with the specified parameters and 
     * assert that the standard SEND DATA replies were sent through the session.
     * @param parameters - the command parameters to use; defaults to []
     * @param finalReplyCode - the expected final reply code; defaults to ReplyCodes.SEND_DATA_FINAL_OK
     */
    protected handleCommandAndVerifySendDataReplies(parameters=[], int finalReplyCode=ReplyCodes.SEND_DATA_FINAL_OK) {
        commandHandler.handleCommand(createCommand(parameters), session)        
        assertSessionReply(0, ReplyCodes.SEND_DATA_INITIAL_OK)
        assertSessionReply(1, finalReplyCode)
    }
    
     /**
      * Set the current directory within the session
      * @param path - the new path value for the current directory 
      */
     protected void setCurrentDirectory(String path) {
         session.setAttribute(SessionKeys.CURRENT_DIRECTORY, path)
     }
     
    /**
     * Convenience method to return the end-of-line character(s) for the current CommandHandler.
     */
    protected endOfLine() {
        commandHandler.endOfLine()
    }
    
    /**
     * Return the specified paths concatenated with the path separator in between
     * @param paths - the varargs list of path components to concatenate
     * @return p[0] + '/' + p[1] + '/' + p[2] + ...
     */
    protected String p(String[] paths) {
        return paths.join("/").replace("//", "/")
    }
    
}