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
 * Tests for PortCommandHandler
 *
 * @version $Revision$ - $Date$
 *
 * @author Chris Mair
 */
class EprtCommandHandlerTest extends AbstractFakeCommandHandlerTestCase {

    static final PARAMETERS_IPV4 = ["|1|132.235.1.2|6275|"]
    static final HOST_IPV4 = InetAddress.getByName("132.235.1.2")
    static final PARAMETERS_IPV6 = ["|2|1080::8:800:200C:417A|6275|"]
    static final HOST_IPV6 = InetAddress.getByName("1080::8:800:200C:417A")
    static final PORT = 6275

    boolean testNotLoggedIn = false

    void testHandleCommand_IPv4() {
        handleCommand(PARAMETERS_IPV4)
        assertSessionReply(ReplyCodes.EPRT_OK, 'eprt')
        assert session.clientDataHost == HOST_IPV4
        assert session.clientDataPort == PORT
    }

    void testHandleCommand_IPv6() {
        handleCommand(PARAMETERS_IPV6)
        assertSessionReply(ReplyCodes.EPRT_OK, 'eprt')
        assert session.clientDataHost == HOST_IPV6
        assert session.clientDataPort == PORT
    }

    void testHandleCommand_IPv6_CustomDelimiter() {
        handleCommand(["@2@1080::8:800:200C:417A@6275@"])
        assertSessionReply(ReplyCodes.EPRT_OK, 'eprt')
        assert session.clientDataHost == HOST_IPV6
        assert session.clientDataPort == PORT
    }

    void testHandleCommand_IllegalParameterFormat() {
        handleCommand(['abcdef'])
        assertSessionReply(ReplyCodes.COMMAND_SYNTAX_ERROR)
    }

    void testHandleCommand_PortMissing() {
        handleCommand(['|1|132.235.1.2|'])
        assertSessionReply(ReplyCodes.COMMAND_SYNTAX_ERROR)
    }

    void testHandleCommand_IllegalHostName() {
        handleCommand(['|1|132.xxx.rrr|6275|'])
        assertSessionReply(ReplyCodes.COMMAND_SYNTAX_ERROR)
    }

    void testHandleCommand_MissingRequiredParameter() {
        testHandleCommand_MissingRequiredParameter([])
    }

    //-------------------------------------------------------------------------
    // Helper Methods
    //-------------------------------------------------------------------------

    void setUp() {
        super.setUp()
    }

    CommandHandler createCommandHandler() {
        new EprtCommandHandler()
    }

    Command createValidCommand() {
        return new Command(CommandNames.EPRT, PARAMETERS_IPV4)
    }

}