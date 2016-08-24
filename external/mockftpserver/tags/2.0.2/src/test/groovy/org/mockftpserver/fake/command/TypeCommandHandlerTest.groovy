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
import org.mockftpserver.core.session.SessionKeys

/**
 * Tests for TestCommandHandler
 *
 * @version $Revision$ - $Date$
 *
 * @author Chris Mair
 */
class TypeCommandHandlerTest extends AbstractFakeCommandHandlerTest {

    void testHandleCommand_Ascii() {
        handleCommand(['A'])
        assertSessionReply(ReplyCodes.TYPE_OK, 'type')
        assert session.getAttribute(SessionKeys.ASCII_TYPE) == true
    }

    void testHandleCommand_NonAscii() {
        handleCommand(['I'])
        assertSessionReply(ReplyCodes.TYPE_OK, 'type')
        assert session.getAttribute(SessionKeys.ASCII_TYPE) == false
    }

    void testHandleCommand_MissingRequiredParameter() {
        testHandleCommand_MissingRequiredParameter([])
        assert session.getAttribute(SessionKeys.ASCII_TYPE) == null
    }

    //-------------------------------------------------------------------------
    // Helper Methods
    //-------------------------------------------------------------------------

    CommandHandler createCommandHandler() {
        new TypeCommandHandler()
    }

    Command createValidCommand() {
        return new Command(CommandNames.TYPE, ['A'])
    }

    void setUp() {
        super.setUp()
    }

}