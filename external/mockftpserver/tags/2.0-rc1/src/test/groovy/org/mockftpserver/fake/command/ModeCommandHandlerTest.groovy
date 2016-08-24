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
 * Tests for ModeCommandHandler
 *
 * @version $Revision: 67 $ - $Date: 2008-06-11 21:38:50 -0400 (Wed, 11 Jun 2008) $
 *
 * @author Chris Mair
 */
class ModeCommandHandlerTest extends AbstractFakeCommandHandlerTest {

    void testHandleCommand() {
        handleCommand([])
        assertSessionReply(ReplyCodes.MODE_OK, 'mode')
    }

    //-------------------------------------------------------------------------
    // Helper Methods
    //-------------------------------------------------------------------------

    CommandHandler createCommandHandler() {
        new ModeCommandHandler()
    }

    Command createValidCommand() {
        return new Command(CommandNames.MODE, [])
    }

    void setUp() {
        super.setUp()
    }

}