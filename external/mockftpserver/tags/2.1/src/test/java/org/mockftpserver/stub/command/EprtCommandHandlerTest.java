/*
 * Copyright 2009 the original author or authors.
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

import java.net.InetAddress;

/**
 * Tests for the EprtCommandHandler class
 *
 * @author Chris Mair
 * @version $Revision$ - $Date$
 */
public final class EprtCommandHandlerTest extends AbstractCommandHandlerTest {

    private static final String[] PARAMETERS_INSUFFICIENT = EMPTY;
    private static final String[] PARAMETERS_IPV4 = {"|1|132.235.1.2|6275|"};
    private static final InetAddress HOST_IPV4 = inetAddress("132.235.1.2");
    private static final String[] PARAMETERS_IPV6 = {"|2|1080::8:800:200C:417A|6275|"};
    private static final InetAddress HOST_IPV6 = inetAddress("1080::8:800:200C:417A");
    private static final int PORT = 6275;

    private EprtCommandHandler commandHandler;

    public void testHandleCommand_IPv4() throws Exception {
        final Command COMMAND = new Command(CommandNames.EPRT, PARAMETERS_IPV4);

        session.setClientDataPort(PORT);
        session.setClientDataHost(HOST_IPV4);
        session.sendReply(ReplyCodes.EPRT_OK, replyTextFor(ReplyCodes.EPRT_OK));
        replay(session);

        commandHandler.handleCommand(COMMAND, session);
        verify(session);

        verifyNumberOfInvocations(commandHandler, 1);
        verifyTwoDataElements(commandHandler.getInvocation(0),
                PortCommandHandler.HOST_KEY, HOST_IPV4,
                PortCommandHandler.PORT_KEY, new Integer(PORT));
    }

    public void testHandleCommand_IPv6() throws Exception {
        final Command COMMAND = new Command(CommandNames.EPRT, PARAMETERS_IPV6);

        session.setClientDataPort(PORT);
        session.setClientDataHost(HOST_IPV6);
        session.sendReply(ReplyCodes.EPRT_OK, replyTextFor(ReplyCodes.EPRT_OK));
        replay(session);

        commandHandler.handleCommand(COMMAND, session);
        verify(session);

        verifyNumberOfInvocations(commandHandler, 1);
        verifyTwoDataElements(commandHandler.getInvocation(0),
                PortCommandHandler.HOST_KEY, HOST_IPV6,
                PortCommandHandler.PORT_KEY, new Integer(PORT));
    }

    public void testHandleCommand_MissingRequiredParameter() throws Exception {
        testHandleCommand_InvalidParameters(commandHandler, CommandNames.EPRT, PARAMETERS_INSUFFICIENT);
    }

    /**
     * Perform initialization before each test
     *
     * @see org.mockftpserver.core.command.AbstractCommandHandlerTest#setUp()
     */
    protected void setUp() throws Exception {
        super.setUp();
        commandHandler = new EprtCommandHandler();
        commandHandler.setReplyTextBundle(replyTextBundle);
    }

}