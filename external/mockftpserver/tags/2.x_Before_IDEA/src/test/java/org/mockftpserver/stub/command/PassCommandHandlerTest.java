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

import org.mockftpserver.core.command.Command;
import org.mockftpserver.core.command.CommandNames;
import org.mockftpserver.core.command.ReplyCodes;
import org.mockftpserver.stub.command.PassCommandHandler;

/**
 * Tests for the PassCommandHandler class
 * 
 * @version $Revision$ - $Date$
 * 
 * @author Chris Mair
 */
public final class PassCommandHandlerTest extends AbstractCommandHandlerTest {

    private static final String PASSWORD1 = "password1";
    private static final String PASSWORD2 = "password2";

    private PassCommandHandler commandHandler;
    private Command command1;
    private Command command2;

    /**
     * Test the handleCommand() method
     */
    public void testHandleCommand() throws Exception {

        session.sendReply(ReplyCodes.PASS_OK, replyTextFor(ReplyCodes.PASS_OK));
        session.sendReply(ReplyCodes.PASS_OK, replyTextFor(ReplyCodes.PASS_OK));
        replay(session);
        
        commandHandler.handleCommand(command1, session);
        commandHandler.handleCommand(command2, session);
        verify(session);
        
        verifyNumberOfInvocations(commandHandler, 2);
        verifyOneDataElement(commandHandler.getInvocation(0), PassCommandHandler.PASSWORD_KEY, PASSWORD1);
        verifyOneDataElement(commandHandler.getInvocation(1), PassCommandHandler.PASSWORD_KEY, PASSWORD2);
    }
    
    /**
     * Test the handleCommand() method, when no password parameter has been specified
     */
    public void testHandleCommand_MissingPasswordParameter() throws Exception {
        testHandleCommand_InvalidParameters(commandHandler, CommandNames.PASS, EMPTY);
    }

    /**
     * Perform initialization before each test
     * @see org.mockftpserver.stub.command.AbstractCommandHandlerTest#setUp()
     */
    protected void setUp() throws Exception {
        super.setUp();
        commandHandler = new PassCommandHandler();
        commandHandler.setReplyTextBundle(replyTextBundle);
        command1 = new Command(CommandNames.PASS, array(PASSWORD1));
        command2 = new Command(CommandNames.PASS, array(PASSWORD2));
    }

}
