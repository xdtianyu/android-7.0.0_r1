/*
 * Copyright 2007 the original author or authors.
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
package org.mockftpserver.stub.command;

import org.mockftpserver.core.command.AbstractCommandHandlerTest;
import org.mockftpserver.core.command.Command;
import org.mockftpserver.core.command.CommandNames;
import org.mockftpserver.core.command.ReplyCodes;

/**
 * Tests for the AppeCommandHandler class
 *
 * @author Chris Mair
 * @version $Revision$ - $Date$
 */
public final class AppeCommandHandlerTest extends AbstractCommandHandlerTest {

    private AppeCommandHandler commandHandler;

    /**
     * Perform initialization before each test
     *
     * @see org.mockftpserver.core.command.AbstractCommandHandlerTest#setUp()
     */
    protected void setUp() throws Exception {
        super.setUp();
        commandHandler = new AppeCommandHandler();
        commandHandler.setReplyTextBundle(replyTextBundle);
    }

    /**
     * Test the handleCommand() method, as well as the getFileContents() and clearFileContents() methods
     */
    public void testHandleCommand() throws Exception {
        final String DATA = "ABC";

        session.sendReply(ReplyCodes.TRANSFER_DATA_INITIAL_OK, replyTextFor(ReplyCodes.TRANSFER_DATA_INITIAL_OK));
        session.openDataConnection();
        session.readData();
        control(session).setReturnValue(DATA.getBytes());
        session.closeDataConnection();
        session.sendReply(ReplyCodes.TRANSFER_DATA_FINAL_OK, replyTextFor(ReplyCodes.TRANSFER_DATA_FINAL_OK));
        replay(session);

        Command command = new Command(CommandNames.APPE, array(FILENAME1));
        commandHandler.handleCommand(command, session);
        verify(session);

        verifyNumberOfInvocations(commandHandler, 1);
        verifyTwoDataElements(commandHandler.getInvocation(0), AppeCommandHandler.PATHNAME_KEY, FILENAME1,
                AppeCommandHandler.FILE_CONTENTS_KEY, DATA.getBytes());
    }

    /**
     * Test the handleCommand() method, when no pathname parameter has been specified
     */
    public void testHandleCommand_MissingPathnameParameter() throws Exception {
        testHandleCommand_InvalidParameters(commandHandler, CommandNames.APPE, EMPTY);
    }

}
