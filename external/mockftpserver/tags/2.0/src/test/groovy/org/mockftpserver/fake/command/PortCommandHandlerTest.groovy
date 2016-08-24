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
 * Tests for PortCommandHandler
 *
 * @version $Revision$ - $Date$
 *
 * @author Chris Mair
 */
class PortCommandHandlerTest extends AbstractFakeCommandHandlerTest {

    static final PARAMETERS = ["11", "22", "33", "44", "1", "206"]
    static final PARAMETERS_INSUFFICIENT = ["7", "29", "99", "11", "77"]
    static final PORT = (1 << 8) + 206
    static final HOST = InetAddress.getByName("11.22.33.44")

    boolean testNotLoggedIn = false

    void testHandleCommand() {
        handleCommand(PARAMETERS)
        assertSessionReply(ReplyCodes.PORT_OK, 'port')
        assert session.clientDataPort == PORT
        assert session.clientDataHost == HOST
    }

    void testHandleCommand_MissingRequiredParameter() {
        testHandleCommand_MissingRequiredParameter(PARAMETERS_INSUFFICIENT)
    }

    //-------------------------------------------------------------------------
    // Helper Methods
    //-------------------------------------------------------------------------

    void setUp() {
        super.setUp()
    }

    CommandHandler createCommandHandler() {
        new PortCommandHandler()
    }

    Command createValidCommand() {
        return new Command(CommandNames.PORT, PARAMETERS)
    }

}