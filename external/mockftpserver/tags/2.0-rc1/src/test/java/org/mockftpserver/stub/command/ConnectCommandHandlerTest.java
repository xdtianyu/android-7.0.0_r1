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
import org.mockftpserver.stub.command.ConnectCommandHandler;

/**
 * Tests for the ConnectCommandHandler class
 * 
 * @version $Revision$ - $Date$
 * 
 * @author Chris Mair
 */
public final class ConnectCommandHandlerTest extends AbstractCommandHandlerTest {

    private ConnectCommandHandler commandHandler;
    private Command command1;

    /**
     * Test the handleCommand() method
     * @throws Exception
     */
    public void testHandleCommand() throws Exception {

        session.sendReply(ReplyCodes.CONNECT_OK, replyTextFor(ReplyCodes.CONNECT_OK));
        replay(session);
        
        commandHandler.handleCommand(command1, session);
        verify(session);
        
        verifyNumberOfInvocations(commandHandler, 1);
        verifyNoDataElements(commandHandler.getInvocation(0));
    }
    
    /**
     * Perform initialization before each test
     * @see org.mockftpserver.stub.command.AbstractCommandHandlerTest#setUp()
     */
    protected void setUp() throws Exception {
        super.setUp();
        commandHandler = new ConnectCommandHandler();
        commandHandler.setReplyTextBundle(replyTextBundle);
        command1 = new Command(CommandNames.CONNECT, EMPTY);
    }

}
