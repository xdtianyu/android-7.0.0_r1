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
import org.mockftpserver.core.command.Command;
import org.mockftpserver.core.command.CommandNames;
import org.mockftpserver.core.command.ReplyCodes;
import org.mockftpserver.core.util.AssertFailedException;
import org.mockftpserver.stub.command.SystCommandHandler;

/**
 * Tests for the SystCommandHandler class
 * 
 * @version $Revision$ - $Date$
 * 
 * @author Chris Mair
 */
public final class SystCommandHandlerTest extends AbstractCommandHandlerTest {

    private static final Logger LOG = Logger.getLogger(SystCommandHandlerTest.class);
    
    private SystCommandHandler commandHandler;

    /**
     * Test the handleCommand() method
     */
    public void testHandleCommand() throws Exception {

        final String SYSTEM_NAME = "UNIX";
        commandHandler.setSystemName(SYSTEM_NAME);

        session.sendReply(ReplyCodes.SYST_OK, formattedReplyTextFor(ReplyCodes.SYST_OK, "\"" + SYSTEM_NAME + "\""));
        replay(session);
        
        final Command COMMAND = new Command(CommandNames.SYST, EMPTY);

        commandHandler.handleCommand(COMMAND, session);
        verify(session);
        
        verifyNumberOfInvocations(commandHandler, 1);
        verifyNoDataElements(commandHandler.getInvocation(0));
    }
    
    /**
     * Test the SetSystemName method, passing in a null
     */
    public void testSetSystemName_Null() {
        try {
            commandHandler.setSystemName(null);
            fail("Expected AssertFailedException");
        }
        catch (AssertFailedException expected) {
            LOG.info("Expected: " + expected);
        }
    }
    
    /**
     * Perform initialization before each test
     * @see org.mockftpserver.stub.command.AbstractCommandHandlerTest#setUp()
     */
    protected void setUp() throws Exception {
        super.setUp();
        commandHandler = new SystCommandHandler();
        commandHandler.setReplyTextBundle(replyTextBundle);
    }

}
