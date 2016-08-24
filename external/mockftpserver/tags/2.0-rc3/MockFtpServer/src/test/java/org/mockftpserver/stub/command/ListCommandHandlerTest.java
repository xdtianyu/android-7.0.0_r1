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

import org.easymock.MockControl;
import org.mockftpserver.core.command.AbstractCommandHandlerTest;
import org.mockftpserver.core.command.Command;
import org.mockftpserver.core.command.CommandNames;
import org.mockftpserver.core.command.ReplyCodes;

/**
 * Tests for the ListCommandHandler class
 *
 * @author Chris Mair
 * @version $Revision$ - $Date$
 */
public final class ListCommandHandlerTest extends AbstractCommandHandlerTest {

    private ListCommandHandler commandHandler;

    /**
     * Test the handleCommand() method
     *
     * @throws Exception
     */
    public void testHandleCommand() throws Exception {
        final String DIR_LISTING = " directory listing\nabc.txt\ndef.log\n";
        final String DIR_LISTING_TRIMMED = DIR_LISTING.trim();
        ((ListCommandHandler) commandHandler).setDirectoryListing(DIR_LISTING);

        for (int i = 0; i < 2; i++) {
            session.sendReply(ReplyCodes.TRANSFER_DATA_INITIAL_OK, replyTextFor(ReplyCodes.TRANSFER_DATA_INITIAL_OK));
            session.openDataConnection();
            byte[] bytes = DIR_LISTING_TRIMMED.getBytes();
            session.sendData(bytes, bytes.length);
            control(session).setMatcher(MockControl.ARRAY_MATCHER);
            session.closeDataConnection();
            session.sendReply(ReplyCodes.TRANSFER_DATA_FINAL_OK, replyTextFor(ReplyCodes.TRANSFER_DATA_FINAL_OK));
        }
        replay(session);

        Command command1 = new Command(CommandNames.LIST, array(DIR1));
        Command command2 = new Command(CommandNames.LIST, EMPTY);
        commandHandler.handleCommand(command1, session);
        commandHandler.handleCommand(command2, session);
        verify(session);

        verifyNumberOfInvocations(commandHandler, 2);
        verifyOneDataElement(commandHandler.getInvocation(0), ListCommandHandler.PATHNAME_KEY, DIR1);
        verifyOneDataElement(commandHandler.getInvocation(1), ListCommandHandler.PATHNAME_KEY, null);
    }

    /**
     * Perform initialization before each test
     *
     * @see org.mockftpserver.core.command.AbstractCommandHandlerTest#setUp()
     */
    protected void setUp() throws Exception {
        super.setUp();
        commandHandler = new ListCommandHandler();
        commandHandler.setReplyTextBundle(replyTextBundle);
    }

}
