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

import org.apache.log4j.Logger;
import org.mockftpserver.core.command.AbstractCommandHandlerTestCase;
import org.mockftpserver.core.command.Command;
import org.mockftpserver.core.command.CommandNames;
import org.mockftpserver.core.command.ReplyCodes;

import java.net.InetAddress;

/**
 * Tests for the PasvCommandHandler class
 *
 * @author Chris Mair
 * @version $Revision$ - $Date$
 */
public final class PasvCommandHandlerTest extends AbstractCommandHandlerTestCase {

    private static final Logger LOG = Logger.getLogger(PasvCommandHandlerTest.class);
    private static final int PORT = (23 << 8) + 77;

    private PasvCommandHandler commandHandler;

    /**
     * Test the handleCommand() method
     */
    public void testHandleCommand() throws Exception {

        final InetAddress SERVER = inetAddress("192.168.0.2");
        session.switchToPassiveMode();
        control(session).setReturnValue(PORT);
        session.getServerHost();
        control(session).setReturnValue(SERVER);
        session.sendReply(ReplyCodes.PASV_OK, formattedReplyTextFor(227, "(192,168,0,2,23,77)"));
        replay(session);

        final Command COMMAND = new Command(CommandNames.PASV, EMPTY);

        commandHandler.handleCommand(COMMAND, session);
        verify(session);

        verifyNumberOfInvocations(commandHandler, 1);
        verifyNoDataElements(commandHandler.getInvocation(0));
    }

    /**
     * Perform initialization before each test
     *
     * @see org.mockftpserver.core.command.AbstractCommandHandlerTestCase#setUp()
     */
    protected void setUp() throws Exception {
        super.setUp();
        commandHandler = new PasvCommandHandler();
        commandHandler.setReplyTextBundle(replyTextBundle);
    }

}
