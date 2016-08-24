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

import java.net.InetAddress;
import java.net.UnknownHostException;

import org.apache.log4j.Logger;
import org.mockftpserver.core.command.Command;
import org.mockftpserver.core.command.CommandNames;
import org.mockftpserver.core.command.ReplyCodes;
import org.mockftpserver.core.util.AssertFailedException;
import org.mockftpserver.stub.command.PortCommandHandler;

/**
 * Tests for the PortCommandHandler class
 * 
 * @version $Revision$ - $Date$
 * 
 * @author Chris Mair
 */
public final class PortCommandHandlerTest extends AbstractCommandHandlerTest {

    private static final Logger LOG = Logger.getLogger(PortCommandHandlerTest.class);
    private static final String[] PARAMETERS = new String[] { "192", "22", "250", "44", "1", "206" };
    private static final String[] PARAMETERS_INSUFFICIENT = new String[] {"7", "29", "99", "11", "77"};
    private static final int PORT = (1 << 8) + 206;
    private static final InetAddress HOST = inetAddress("192.22.250.44");

    private PortCommandHandler commandHandler;

    /**
     * Test the handleCommand() method
     */
    public void testHandleCommand() throws Exception {
        final Command COMMAND = new Command(CommandNames.PORT, PARAMETERS);

        session.setClientDataPort(PORT);
        session.setClientDataHost(HOST);
        session.sendReply(ReplyCodes.PORT_OK, replyTextFor(ReplyCodes.PORT_OK));
        replay(session);

        commandHandler.handleCommand(COMMAND, session);
        verify(session);
        
        verifyNumberOfInvocations(commandHandler, 1);
        verifyTwoDataElements(commandHandler.getInvocation(0), 
                PortCommandHandler.HOST_KEY, HOST,
                PortCommandHandler.PORT_KEY, new Integer(PORT));
    }

    /**
     * Test the handleCommand() method, when not enough parameters have been specified
     */
    public void testHandleCommand_InsufficientParameters() throws Exception {
        testHandleCommand_InvalidParameters(commandHandler, CommandNames.PORT, PARAMETERS_INSUFFICIENT);
    }

    /**
     * Test the parseHost() method
     * @throws UnknownHostException 
     */
    public void testParseHost() throws UnknownHostException {
        InetAddress host = PortCommandHandler.parseHost(PARAMETERS);
        assertEquals("InetAddress", HOST, host);
    }
    
    /**
     * Test the parsePortNumber() method
     */
    public void testParsePortNumber() {
        int portNumber = PortCommandHandler.parsePortNumber(PARAMETERS);
        assertEquals("portNumber", PORT, portNumber);
    }
    
    /**
     * Test the parseHost() method, passing in null
     * @throws UnknownHostException 
     */
    public void testParseHost_Null() throws UnknownHostException {
        try {
            PortCommandHandler.parseHost(null);
            fail("Expected AssertFailedException");
        }
        catch (AssertFailedException expected) {
            LOG.info("Expected: " + expected);
        }
    }
        
    /**
     * Test the parseHost() method, passing in a String[] with not enough parameters
     * @throws UnknownHostException 
     */
    public void testParseHost_InsufficientParameters() throws UnknownHostException {
        try {
            PortCommandHandler.parseHost(PARAMETERS_INSUFFICIENT);
            fail("Expected AssertFailedException");
        }
        catch (AssertFailedException expected) {
            LOG.info("Expected: " + expected);
        }
    }
        
    /**
     * Test the parsePortNumber() method, passing in null
     * @throws UnknownHostException 
     */
    public void testParsePortNumber_Null() throws UnknownHostException {
        try {
            PortCommandHandler.parsePortNumber(null);
            fail("Expected AssertFailedException");
        }
        catch (AssertFailedException expected) {
            LOG.info("Expected: " + expected);
        }
    }
        
    /**
     * Test the parsePortNumber() method, passing in a String[] with not enough parameters
     * @throws UnknownHostException 
     */
    public void testParsePortNumber_InsufficientParameters() throws UnknownHostException {
        try {
            PortCommandHandler.parsePortNumber(PARAMETERS_INSUFFICIENT);
            fail("Expected AssertFailedException");
        }
        catch (AssertFailedException expected) {
            LOG.info("Expected: " + expected);
        }
    }
        
    /**
     * Perform initialization before each test
     * 
     * @see org.mockftpserver.stub.command.AbstractCommandHandlerTest#setUp()
     */
    protected void setUp() throws Exception {
        super.setUp();
        commandHandler = new PortCommandHandler();
        commandHandler.setReplyTextBundle(replyTextBundle);
   }

}
