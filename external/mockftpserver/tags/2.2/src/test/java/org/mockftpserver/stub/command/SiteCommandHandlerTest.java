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

import org.mockftpserver.core.command.*;
import org.mockftpserver.core.command.AbstractCommandHandlerTestCase;

/**
 * Tests for the SiteCommandHandler class
 * 
 * @version $Revision$ - $Date$
 * 
 * @author Chris Mair
 */
public final class SiteCommandHandlerTest extends AbstractCommandHandlerTestCase {

    private static final String PARAMETERS1 = "abc def";
    private static final String PARAMETERS2 = "abc,23,def";
    
    private SiteCommandHandler commandHandler;
    private Command command1;
    private Command command2;
    
    /**
     * Test the handleCommand(Command,Session) method
     * @throws Exception
     */
    public void testHandleCommand() throws Exception {
        session.sendReply(ReplyCodes.SITE_OK, replyTextFor(ReplyCodes.SITE_OK));
        session.sendReply(ReplyCodes.SITE_OK, replyTextFor(ReplyCodes.SITE_OK));
        replay(session);
        
        commandHandler.handleCommand(command1, session);
        commandHandler.handleCommand(command2, session);
        verify(session);

        verifyNumberOfInvocations(commandHandler, 2);
        verifyOneDataElement(commandHandler.getInvocation(0), SiteCommandHandler.PARAMETERS_KEY, PARAMETERS1);
        verifyOneDataElement(commandHandler.getInvocation(1), SiteCommandHandler.PARAMETERS_KEY, PARAMETERS2);
    }

    /**
     * Test the handleCommand() method, when no "parameters" parameter has been specified
     */
    public void testHandleCommand_MissingParameters() throws Exception {
        testHandleCommand_InvalidParameters(commandHandler, CommandNames.SITE, EMPTY);
    }

    /**
     * Perform initialization before each test
     * @see org.mockftpserver.core.command.AbstractCommandHandlerTestCase#setUp()
     */
    protected void setUp() throws Exception {
        super.setUp();
        commandHandler = new SiteCommandHandler();
        commandHandler.setReplyTextBundle(replyTextBundle);
        command1 = new Command(CommandNames.SITE, array(PARAMETERS1));
        command2 = new Command(CommandNames.SITE, array(PARAMETERS2));
    }
}
