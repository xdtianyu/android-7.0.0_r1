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
import org.mockftpserver.core.command.ReplyCodes
import org.mockftpserver.core.session.SessionKeys
import org.mockftpserver.core.session.StubSession
import org.mockftpserver.fake.StubServerConfiguration
import org.mockftpserver.fake.UserAccount
import org.mockftpserver.fake.filesystem.DirectoryEntry
import org.mockftpserver.fake.filesystem.FileEntry
import org.mockftpserver.fake.filesystem.FileSystemException
import org.mockftpserver.fake.filesystem.TestUnixFakeFileSystem
import org.mockftpserver.test.AbstractGroovyTestCase
import org.mockftpserver.test.StubResourceBundle

/**
 * Abstract superclass for CommandHandler tests
 *
 * @version $Revision$ - $Date$
 *
 * @author Chris Mair
 */
abstract class AbstractFakeCommandHandlerTestCase extends AbstractGroovyTestCase {

    protected static final ERROR_MESSAGE_KEY = 'msgkey'

    protected session
    protected serverConfiguration
    protected replyTextBundle
    protected commandHandler
    protected fileSystem
    protected userAccount

    /** Set this to false to skip the test that verifies that the CommandHandler requires a logged in user              */
    boolean testNotLoggedIn = true

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

    void testHandleCommand_NotLoggedIn() {
        if (getProperty('testNotLoggedIn')) {
            def command = createValidCommand()
            session.removeAttribute(SessionKeys.USER_ACCOUNT)
            commandHandler.handleCommand(command, session)
            assertSessionReply(ReplyCodes.NOT_LOGGED_IN)
        }
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
        replyTextBundle = new StubResourceBundle()
        fileSystem = new TestUnixFakeFileSystem()
        fileSystem.createParentDirectoriesAutomatically = true
        serverConfiguration.setFileSystem(fileSystem)

        userAccount = new UserAccount()
        session.setAttribute(SessionKeys.USER_ACCOUNT, userAccount)

        commandHandler = createCommandHandler()
        commandHandler.serverConfiguration = serverConfiguration
        commandHandler.replyTextBundle = replyTextBundle
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
     * @deprecated
     */
    protected assertSessionReply(int expectedReplyCode, text = expectedReplyCode as String) {
        assertSessionReply(0, expectedReplyCode, text)
    }

    /**
     * Assert that the specified reply code and message containing text was sent through the session.
     * @param replyIndex - the index of the reply to compare
     * @param expectedReplyCode - the expected reply code
     * @param text - the text expected within the reply message; defaults to the reply code as a String
     */
    protected assertSessionReply(int replyIndex, int expectedReplyCode, text = expectedReplyCode as String) {
        LOG.info(session)
        String actualMessage = session.getReplyMessage(replyIndex)
        def actualReplyCode = session.getReplyCode(replyIndex)
        assert actualReplyCode == expectedReplyCode
        if (text instanceof List) {
            text.each { assert actualMessage.contains(it), "[$actualMessage] does not contain [$it]" }
        }
        else {
            assert actualMessage.contains(text), "[$actualMessage] does not contain [$text]"
        }
    }

    /**
     * Assert that the specified reply codes were sent through the session.
     * @param replyCodes - the List of expected sent reply codes
     */
    protected assertSessionReplies(List replyCodes) {
        LOG.info(session)
        replyCodes.eachWithIndex {replyCode, replyIndex ->
            assertSessionReply(replyIndex, replyCode)
        }
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
     * Assert that the specified data was sent through the session, terminated by an end-of-line.
     * @param expectedData - the expected data
     */
    protected assertSessionDataWithEndOfLine(String expectedData) {
        assertSessionData(expectedData + endOfLine())
    }

    /**
     * Assert that the data sent through the session terminated with an end-of-line.
     */
    protected assertSessionDataEndsWithEndOfLine() {
        assert session.sentData[0].endsWith(endOfLine())
    }

    /**
     * Execute the handleCommand() method with the specified parameters and 
     * assert that the standard SEND DATA replies were sent through the session.
     * @param parameters - the command parameters to use; defaults to []
     * @param finalReplyCode - the expected final reply code; defaults to ReplyCodes.TRANSFER_DATA_FINAL_OK
     */
    protected handleCommandAndVerifySendDataReplies(parameters = [], int finalReplyCode = ReplyCodes.TRANSFER_DATA_FINAL_OK) {
        handleCommand(parameters)
        assertSessionReplies([ReplyCodes.TRANSFER_DATA_INITIAL_OK, finalReplyCode])
    }

    /**
     * Execute the handleCommand() method with the specified parameters and
     * assert that the standard SEND DATA replies were sent through the session.
     * @param parameters - the command parameters to use
     * @param initialReplyMessageKey - the expected reply message key for the initial reply
     * @param finalReplyMessageKey -  the expected reply message key for the final reply
     * @param finalReplyCode - the expected final reply code; defaults to ReplyCodes.TRANSFER_DATA_FINAL_OK
     */
    protected handleCommandAndVerifySendDataReplies(parameters, String initialReplyMessageKey, String finalReplyMessageKey, int finalReplyCode = ReplyCodes.TRANSFER_DATA_FINAL_OK) {
        handleCommand(parameters)
        assertSessionReply(0, ReplyCodes.TRANSFER_DATA_INITIAL_OK, initialReplyMessageKey)
        assertSessionReply(1, finalReplyCode, finalReplyMessageKey)
    }

    /**
     * Override the named method for the specified object instance
     * @param object - the object instance
     * @param methodName - the name of the method to override
     * @param newMethod - the Closure representing the new method for this single instance
     */
    protected void overrideMethod(object, String methodName, Closure newMethod) {
        LOG.info("Overriding method [$methodName] for class [${object.class}]")
        def emc = new ExpandoMetaClass(object.class, false)
        emc."$methodName" = newMethod
        emc.initialize()
        object.metaClass = emc
    }

    /**
     * Override the named method (that takes a single String arg) of the fileSystem object to throw a (generic) FileSystemException
     * @param methodName - the name of the fileSystem method to override
     */
    protected void overrideMethodToThrowFileSystemException(String methodName) {
        def newMethod = {String path -> throw new FileSystemException("Error thrown by method [$methodName]", ERROR_MESSAGE_KEY) }
        overrideMethod(fileSystem, methodName, newMethod)
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
     * Create a new directory entry with the specified path in the file system
     * @param path - the path of the new directory entry
     * @return the newly created DirectoryEntry
     */
    protected DirectoryEntry createDirectory(String path) {
        DirectoryEntry entry = new DirectoryEntry(path)
        fileSystem.add(entry)
        return entry
    }

    /**
     * Create a new file entry with the specified path in the file system
     * @param path - the path of the new file entry
     * @param contents - the contents for the file; defaults to null
     * @return the newly created FileEntry
     */
    protected FileEntry createFile(String path, contents = null) {
        FileEntry entry = new FileEntry(path: path, contents: contents)
        fileSystem.add(entry)
        return entry
    }

}