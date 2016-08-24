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
package org.mockftpserver.stub.command;

import org.mockftpserver.core.command.Command;
import org.mockftpserver.core.command.CommandHandler;
import org.mockftpserver.core.command.InvocationRecord;
import org.mockftpserver.core.command.ReplyCodes;
import org.mockftpserver.core.session.Session;
import org.mockftpserver.core.util.PortParser;
import org.mockftpserver.core.util.HostAndPort;

import java.net.InetAddress;
import java.net.UnknownHostException;

/**
 * CommandHandler for the EPRT command. Send back a reply code of 200.
 * <p/>
 * Each invocation record stored by this CommandHandler includes the following data element key/values:
 * <ul>
 * <li>{@link #HOST_KEY} ("host") - the client data host (InetAddress) submitted on the invocation (from parameters 1-4)
 * <li>{@link #PORT_KEY} ("port") - the port number (Integer) submitted on the invocation (from parameter 5-6)
 * </ul>
 * See RFC2428 for more information.
 *
 * @author Chris Mair
 * @version $Revision$ - $Date$
 */
public class EprtCommandHandler extends AbstractStubCommandHandler implements CommandHandler {

    public static final String HOST_KEY = "host";
    public static final String PORT_KEY = "port";

    /**
     * Constructor. Initialize the replyCode.
     */
    public EprtCommandHandler() {
        setReplyCode(ReplyCodes.EPRT_OK);
    }

    /**
     * Handle the command
     *
     * @throws java.net.UnknownHostException
     * @see org.mockftpserver.core.command.CommandHandler#handleCommand(org.mockftpserver.core.command.Command, org.mockftpserver.core.session.Session)
     */
    public void handleCommand(Command command, Session session, InvocationRecord invocationRecord) throws UnknownHostException {
        String parameter = command.getRequiredParameter(0);

        HostAndPort client = PortParser.parseExtendedAddressHostAndPort(parameter);
        LOG.debug("host=" + client.host + " port=" + client.port);
        session.setClientDataHost(client.host);
        session.setClientDataPort(client.port);
        invocationRecord.set(HOST_KEY, client.host);
        invocationRecord.set(PORT_KEY, new Integer(client.port));
        sendReply(session);
    }

}