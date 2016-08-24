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
package org.mockftpserver.stub.command;

import org.mockftpserver.core.command.AbstractCommandHandlerTestCase;
import org.mockftpserver.core.command.Command;
import org.mockftpserver.core.command.CommandNames;
import org.mockftpserver.core.command.ReplyCodes;

import java.net.InetAddress;

/**
 * Tests for the PortCommandHandler class
 *
 * @author Chris Mair
 * @version $Revision$ - $Date$
 */
public final class PortCommandHandlerTest extends AbstractCommandHandlerTestCase {

    private static final String[] PARAMETERS = new String[]{"11", "22", "33", "44", "1", "206"};
    private static final String[] PARAMETERS_INSUFFICIENT = new String[]{"7", "29", "99", "11", "77"};
    private static final int PORT = (1 << 8) + 206;
    private static final InetAddress HOST = inetAddress("11.22.33.44");

    private PortCommandHandler commandHandler;

    /**
     * Test the handleCommand() method
     */
    public void testHandleCommand() throws Exception {
        final Command COMMAND = new Command(CommandNames.PORT, PARAMETERS);

        session.setClientDataPort(PORT);
        session.setClientDataHost(HOST);
        session.sendReply(ReplyCodes.PORT_OK, replyTextFor(ReplyCodes.PORT_OK));
        replay(session);

        commandHandler.handleCommand(COMMAND, session);
        verify(session);

        verifyNumberOfInvocations(commandHandler, 1);
        verifyTwoDataElements(commandHandler.getInvocation(0),
                PortCommandHandler.HOST_KEY, HOST,
                PortCommandHandler.PORT_KEY, new Integer(PORT));
    }

    /**
     * Test the handleCommand() method, when not enough parameters have been specified
     */
    public void testHandleCommand_InsufficientParameters() throws Exception {
        testHandleCommand_InvalidParameters(commandHandler, CommandNames.PORT, PARAMETERS_INSUFFICIENT);
    }

    /**
     * Perform initialization before each test
     *
     * @see org.mockftpserver.core.command.AbstractCommandHandlerTestCase#setUp()
     */
    protected void setUp() throws Exception {
        super.setUp();
        commandHandler = new PortCommandHandler();
        commandHandler.setReplyTextBundle(replyTextBundle);
    }

}
