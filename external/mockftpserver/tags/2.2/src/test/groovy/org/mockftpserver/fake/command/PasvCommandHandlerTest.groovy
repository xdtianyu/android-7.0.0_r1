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
import org.mockftpserver.core.command.CommandNames
import org.mockftpserver.core.command.ReplyCodes

/**
 * Tests for PasvCommandHandler
 *
 * @version $Revision$ - $Date$
 *
 * @author Chris Mair
 */
class PasvCommandHandlerTest extends AbstractFakeCommandHandlerTestCase {

    static final PORT = (23 << 8) + 77
    static final InetAddress SERVER = inetAddress("192.168.0.2")

    void testHandleCommand() {
        final HOST_AND_PORT = "192,168,0,2,23,77"
        session.switchToPassiveModeReturnValue = PORT
        session.serverHost = SERVER
        handleCommand([])

        assertSessionReply(ReplyCodes.PASV_OK, HOST_AND_PORT)
        assert session.switchedToPassiveMode
    }

    //-------------------------------------------------------------------------
    // Helper Methods
    //-------------------------------------------------------------------------

    CommandHandler createCommandHandler() {
        new PasvCommandHandler()
    }

    Command createValidCommand() {
        return new Command(CommandNames.PASV, [])
    }

    void setUp() {
        super.setUp()
    }

}