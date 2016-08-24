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
import org.mockftpserver.stub.command.TypeCommandHandler;

/**
 * Tests for the TypeCommandHandler class
 * 
 * @version $Revision$ - $Date$
 * 
 * @author Chris Mair
 */
public final class TypeCommandHandlerTest extends AbstractCommandHandlerTest {

    private TypeCommandHandler commandHandler;

    /**
     * Test the handleCommand() method
     */
    public void testHandleCommand() throws Exception {
        final Command COMMAND1 = new Command("TYPE", array("A"));
        final Command COMMAND2 = new Command("TYPE", array("B"));
        final Command COMMAND3 = new Command("TYPE", array("L", "8"));

        session.sendReply(ReplyCodes.TYPE_OK, replyTextFor(ReplyCodes.TYPE_OK));
        session.sendReply(ReplyCodes.TYPE_OK, replyTextFor(ReplyCodes.TYPE_OK));
        session.sendReply(ReplyCodes.TYPE_OK, replyTextFor(ReplyCodes.TYPE_OK));
        replay(session);
        
        commandHandler.handleCommand(COMMAND1, session);
        commandHandler.handleCommand(COMMAND2, session);
        commandHandler.handleCommand(COMMAND3, session);
        verify(session);

        verifyNumberOfInvocations(commandHandler, 3);
        verifyOneDataElement(commandHandler.getInvocation(0), TypeCommandHandler.TYPE_INFO_KEY, new String[] {"A", null});
        verifyOneDataElement(commandHandler.getInvocation(1), TypeCommandHandler.TYPE_INFO_KEY, new String[] {"B", null});
        verifyOneDataElement(commandHandler.getInvocation(2), TypeCommandHandler.TYPE_INFO_KEY, new String[] {"L", "8"});
    }
    
    /**
     * Test the handleCommand() method, when no type parameter has been specified
     */
    public void testHandleCommand_MissingTypeParameter() throws Exception {
        testHandleCommand_InvalidParameters(commandHandler, CommandNames.TYPE, EMPTY);
    }

    /**
     * Perform initialization before each test
     * @see org.mockftpserver.stub.command.AbstractCommandHandlerTest#setUp()
     */
    protected void setUp() throws Exception {
        super.setUp();
        commandHandler = new TypeCommandHandler();
        commandHandler.setReplyTextBundle(replyTextBundle);
    }

}
