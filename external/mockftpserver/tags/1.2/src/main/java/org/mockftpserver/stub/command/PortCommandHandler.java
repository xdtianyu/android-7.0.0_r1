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
import java.util.Arrays;

import org.apache.log4j.Logger;
import org.mockftpserver.core.command.Command;
import org.mockftpserver.core.command.CommandHandler;
import org.mockftpserver.core.command.InvocationRecord;
import org.mockftpserver.core.command.ReplyCodes;
import org.mockftpserver.core.session.Session;
import org.mockftpserver.core.util.Assert;
import org.mockftpserver.core.util.AssertFailedException;

/**
 * CommandHandler for the PORT command. Send back a reply code of 200.
 * <p>
 * Each invocation record stored by this CommandHandler includes the following data element key/values:
 * <ul>
 *    <li>{@link #HOST_KEY} ("host") - the client data host (InetAddress) submitted on the invocation (from parameters 1-4)
 *    <li>{@link #PORT_KEY} ("port") - the port number (Integer) submitted on the invocation (from parameter 5-6)
 * </ul>
 * 
 * @version $Revision$ - $Date$
 * 
 * @author Chris Mair
 */
public final class PortCommandHandler extends AbstractStubCommandHandler implements CommandHandler {

    public static final String HOST_KEY = "host";
    public static final String PORT_KEY = "port";
    private static final Logger LOG = Logger.getLogger(PortCommandHandler.class);
    
    /**
     * Constructor. Initialize the replyCode. 
     */
    public PortCommandHandler() {
        setReplyCode(ReplyCodes.PORT_OK);
    }
    
    /**
     * @throws UnknownHostException 
     * @throws UnknownHostException 
     * @see org.mockftpserver.core.command.CommandHandler#handleCommand(Command, Session, InvocationRecord)
     */
    public void handleCommand(Command command, Session session, InvocationRecord invocationRecord) throws UnknownHostException {
        InetAddress host = parseHost(command.getParameters());
        int port = parsePortNumber(command.getParameters());
        LOG.debug("host=" + host + " port=" + port);
        session.setClientDataHost(host);
        session.setClientDataPort(port);
        invocationRecord.set(HOST_KEY, host);
        invocationRecord.set(PORT_KEY, new Integer(port));
        sendReply(session);
    }
    
    /**
     * Parse a 32-bit IP address from the String[] of FTP command parameters.
     * 
     * @param parameters - the String[] of command parameters. It is the concatenation 
     *      of a 32-bit internet host address and a 16-bit TCP port address. This address 
     *      information is broken into 8-bit fields and the value of each field is encoded 
     *      as a separate parameter whose value is a decimal number (in character string 
     *      representation).  Thus, the six parameters for the port command would be:
     *              h1,h2,h3,h4,p1,p2
     *      where h1 is the high order 8 bits of the internet host address, and p1 is the
     *      high order 8 bits of the port number.
     * 
     * @return the InetAddres representing the host parsed from the parameters
     *
     * @throws AssertFailedException - if parameters is null or contains an insufficient number of elements
     * @throws NumberFormatException - if one of the parameters does not contain a parsable integer 
     * @throws UnknownHostException 
     */
    static InetAddress parseHost(String[] parameters) throws UnknownHostException {
        verifySufficientParameters(parameters);

        byte host1 = Byte.parseByte(parameters[0]);
        byte host2 = Byte.parseByte(parameters[1]);
        byte host3 = Byte.parseByte(parameters[2]);
        byte host4 = Byte.parseByte(parameters[3]);

        byte[] address = { host1, host2, host3, host4 };
        InetAddress inetAddress = InetAddress.getByAddress(address);

        return inetAddress;
    }

    /**
     * Parse a 16-bit port number from the String[] of FTP command parameters.
     * 
     * @param parameters - the String[] of command parameters. It is the concatenation 
     *      of a 32-bit internet host address and a 16-bit TCP port address. This address 
     *      information is broken into 8-bit fields and the value of each field is encoded 
     *      as a separate parameter whose value is a decimal number (in character string 
     *      representation).  Thus, the six parameters for the port command would be:
     *              h1,h2,h3,h4,p1,p2
     *      where h1 is the high order 8 bits of the internet host address, and p1 is the
     *      high order 8 bits of the port number.
     * 
     * @return the port number parsed from the parameters
     * 
     * @throws AssertFailedException - if parameters is null or contains an insufficient number of elements
     * @throws NumberFormatException - if one of the parameters does not contain a parsable integer 
     */
    static int parsePortNumber(String[] parameters) {
        verifySufficientParameters(parameters);

        int port1 = Integer.parseInt(parameters[4]);
        int port2 = Integer.parseInt(parameters[5]);
        int port = (port1 << 8) + port2;

        return port;
    }
    
    /**
     * Verify that the parameters is not null and contains the required number of elements
     * @param parameters - the String[] of command parameters
     * @throws AssertFailedException - if parameters is null or contains an insufficient number of elements
     */
    private static void verifySufficientParameters(String[] parameters) {
        Assert.notNull(parameters, "parameters");
        Assert.isTrue(parameters.length >= 6, "The PORT command must contain least be 6 parameters: " + Arrays.asList(parameters));
    }
    
}
