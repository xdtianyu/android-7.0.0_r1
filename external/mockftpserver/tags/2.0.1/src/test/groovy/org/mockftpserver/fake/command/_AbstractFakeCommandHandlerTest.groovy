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

import org.mockftpserver.core.CommandSyntaxException
import org.mockftpserver.core.IllegalStateException
import org.mockftpserver.core.NotLoggedInException
import org.mockftpserver.core.command.Command
import org.mockftpserver.core.command.ReplyCodes
import org.mockftpserver.core.session.Session
import org.mockftpserver.core.session.SessionKeys
import org.mockftpserver.core.session.StubSession
import org.mockftpserver.fake.StubServerConfiguration
import org.mockftpserver.fake.UserAccount
import org.mockftpserver.fake.filesystem.FileSystemException
import org.mockftpserver.fake.filesystem.InvalidFilenameException
import org.mockftpserver.fake.filesystem.UnixFakeFileSystem
import org.mockftpserver.test.AbstractGroovyTest
import org.mockftpserver.test.StubResourceBundle

/**
 * Tests for AbstractFakeCommandHandler
 *
 * @version $Revision$ - $Date$
 *
 * @author Chris Mair
 */
class AbstractFakeCommandHandlerClassTest extends AbstractGroovyTest {

    static PATH = "some/path"
    static REPLY_CODE = 99
    static MESSAGE_KEY = "99.WithFilename"
    static ARG = "ABC"
    static MSG = "text {0}"
    static MSG_WITH_ARG = "text ABC"
    static MSG_FOR_KEY = "some other message"
    static INTERNAL_ERROR = AbstractFakeCommandHandler.INTERNAL_ERROR_KEY
    static MSG_INTERNAL_ERROR = "internal error message {0}"
    private AbstractFakeCommandHandler commandHandler
    private session
    private serverConfiguration
    private replyTextBundle
    private fileSystem
    private userAccount

    //-------------------------------------------------------------------------
    // Tests
    //-------------------------------------------------------------------------

    void testHandleCommand() {
        def command = new Command("C1", ["abc"])
        commandHandler.handleCommand(command, session)
        assert commandHandler.handled

        assertHandleCommandReplyCode(new CommandSyntaxException(""), ReplyCodes.COMMAND_SYNTAX_ERROR)
        assertHandleCommandReplyCode(new IllegalStateException(""), ReplyCodes.ILLEGAL_STATE)
        assertHandleCommandReplyCode(new NotLoggedInException(""), ReplyCodes.NOT_LOGGED_IN)
        assertHandleCommandReplyCode(new InvalidFilenameException(""), ReplyCodes.FILENAME_NOT_VALID)

        shouldFail { commandHandler.handleCommand(null, session) }
        shouldFail { commandHandler.handleCommand(command, null) }
    }

    void testHandleCommand_FileSystemException() {
        assertHandleCommandReplyCode(new FileSystemException(PATH, ''), ReplyCodes.READ_FILE_ERROR, PATH)
        commandHandler.replyCodeForFileSystemException = ReplyCodes.WRITE_FILE_ERROR
        assertHandleCommandReplyCode(new FileSystemException(PATH, ''), ReplyCodes.WRITE_FILE_ERROR, PATH)
    }

    void testSendReply() {
        commandHandler.sendReply(session, REPLY_CODE)
        assert session.sentReplies[0] == [REPLY_CODE, MSG], session.sentReplies[0]

        commandHandler.sendReply(session, REPLY_CODE, [ARG])
        assert session.sentReplies[1] == [REPLY_CODE, MSG_WITH_ARG], session.sentReplies[0]

        shouldFailWithMessageContaining('session') { commandHandler.sendReply(null, REPLY_CODE) }
        shouldFailWithMessageContaining('reply code') { commandHandler.sendReply(session, 0) }
    }

    void testSendReply_MessageKey() {
        commandHandler.sendReply(session, REPLY_CODE, MESSAGE_KEY)
        assert session.sentReplies[0] == [REPLY_CODE, MSG_FOR_KEY], session.sentReplies[0]

        shouldFailWithMessageContaining('session') { commandHandler.sendReply(null, REPLY_CODE, MESSAGE_KEY) }
        shouldFailWithMessageContaining('reply code') { commandHandler.sendReply(session, 0, MESSAGE_KEY) }
    }

    void testSendReply_NullMessageKey() {
        commandHandler.sendReply(session, REPLY_CODE, null, null)
        assert session.sentReplies[0] == [REPLY_CODE, MSG_INTERNAL_ERROR], session.sentReplies[0]
    }

    void testAssertValidReplyCode() {
        commandHandler.assertValidReplyCode(1)        // no exception expected
        shouldFail { commandHandler.assertValidReplyCode(0) }
    }

    void testGetRequiredSessionAttribute() {
        shouldFail(IllegalStateException) { commandHandler.getRequiredSessionAttribute(session, "undefined") }

        session.setAttribute("abc", "not empty")
        commandHandler.getRequiredSessionAttribute(session, "abc") // no exception

        session.setAttribute("abc", "")
        commandHandler.getRequiredSessionAttribute(session, "abc") // no exception
    }

    void testVerifyLoggedIn() {
        shouldFail(NotLoggedInException) { commandHandler.verifyLoggedIn(session) }
        session.setAttribute(SessionKeys.USER_ACCOUNT, userAccount)
        commandHandler.verifyLoggedIn(session)        // no exception expected
    }

    void testGetUserAccount() {
        assert commandHandler.getUserAccount(session) == null
        session.setAttribute(SessionKeys.USER_ACCOUNT, userAccount)
        assert commandHandler.getUserAccount(session)
    }

    void testVerifyFileSystemCondition() {
        commandHandler.verifyFileSystemCondition(true, PATH, '')    // no exception expected
        shouldFail(FileSystemException) { commandHandler.verifyFileSystemCondition(false, PATH, '') }
    }

    void testGetRealPath() {
        assert commandHandler.getRealPath(session, "/xxx") == "/xxx"

        session.setAttribute(SessionKeys.CURRENT_DIRECTORY, "/usr/me")
        assert commandHandler.getRealPath(session, null) == "/usr/me"
        assert commandHandler.getRealPath(session, "/xxx") == "/xxx"
        assert commandHandler.getRealPath(session, "xxx") == "/usr/me/xxx"
        assert commandHandler.getRealPath(session, "../xxx") == "/usr/xxx"
        assert commandHandler.getRealPath(session, "./xxx") == "/usr/me/xxx"
    }

    //-------------------------------------------------------------------------
    // Test Setup
    //-------------------------------------------------------------------------

    void setUp() {
        super.setUp()
        commandHandler = new TestFakeCommandHandler()
        session = new StubSession()
        serverConfiguration = new StubServerConfiguration()
        replyTextBundle = new StubResourceBundle()
        userAccount = new UserAccount()
        fileSystem = new UnixFakeFileSystem()
        serverConfiguration.setFileSystem(fileSystem)

        replyTextBundle.put(REPLY_CODE as String, MSG)
        replyTextBundle.put(MESSAGE_KEY as String, MSG_FOR_KEY)
        replyTextBundle.put(INTERNAL_ERROR as String, MSG_INTERNAL_ERROR)

        commandHandler.serverConfiguration = serverConfiguration
        commandHandler.replyTextBundle = replyTextBundle
    }

    //-------------------------------------------------------------------------
    // Helper Methods
    //-------------------------------------------------------------------------

    /**
     * Assert that when the CommandHandler handleCommand() method throws the
     * specified exception, that the expected reply is sent through the session.
     */
    private void assertHandleCommandReplyCode(Throwable exception, int expected, text = null) {
        commandHandler.exception = exception
        def command = new Command("C1", ["abc"])
        session.sentReplies.clear()
        commandHandler.handleCommand(command, session)
        def sentReply = session.sentReplies[0][0]
        assert sentReply == expected
        if (text) {
            def sentMessage = session.sentReplies[0][1]
            assert sentMessage.contains(text), "sentMessage=[$sentMessage] text=[$text]"
        }
    }

}

/**
 * Concrete subclass of AbstractFakeCommandHandler for testing
 */
class TestFakeCommandHandler extends AbstractFakeCommandHandler {
    boolean handled = false
    def exception

    protected void handle(Command command, Session session) {
        if (exception) {
            throw exception
        }
        this.handled = true
    }
}