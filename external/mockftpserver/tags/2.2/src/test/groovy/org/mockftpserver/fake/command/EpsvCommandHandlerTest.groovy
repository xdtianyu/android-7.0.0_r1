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
package org.mockftpserver.fake.command

import org.mockftpserver.core.command.Command
import org.mockftpserver.core.command.CommandHandler
import org.mockftpserver.core.command.CommandNames
import org.mockftpserver.core.command.ReplyCodes

/**
 * Tests for EpsvCommandHandler
 *
 * @version $Revision$ - $Date$
 *
 * @author Chris Mair
 */
class EpsvCommandHandlerTest extends AbstractFakeCommandHandlerTestCase {

    static final SERVER = InetAddress.getByName("1080::8:800:200C:417A")
    static final PORT = 6275

    void testHandleCommand() {
        session.switchToPassiveModeReturnValue = PORT
        session.serverHost = SERVER
        handleCommand([])

        assertSessionReply(ReplyCodes.EPSV_OK, PORT as String)
        assert session.switchedToPassiveMode
    }

    //-------------------------------------------------------------------------
    // Helper Methods
    //-------------------------------------------------------------------------

    CommandHandler createCommandHandler() {
        new EpsvCommandHandler()
    }

    Command createValidCommand() {
        return new Command(CommandNames.EPSV, [])
    }

}