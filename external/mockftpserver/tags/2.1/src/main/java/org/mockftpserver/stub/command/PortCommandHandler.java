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
package org.mockftpserver.stub.command;

import org.mockftpserver.core.command.Command;
import org.mockftpserver.core.command.CommandHandler;
import org.mockftpserver.core.command.InvocationRecord;
import org.mockftpserver.core.command.ReplyCodes;
import org.mockftpserver.core.session.Session;
import org.mockftpserver.core.util.HostAndPort;
import org.mockftpserver.core.util.PortParser;

import java.net.UnknownHostException;

/**
 * CommandHandler for the PORT command. Send back a reply code of 200.
 * <p/>
 * Each invocation record stored by this CommandHandler includes the following data element key/values:
 * <ul>
 * <li>{@link #HOST_KEY} ("host") - the client data host (InetAddress) submitted on the invocation (from parameters 1-4)
 * <li>{@link #PORT_KEY} ("port") - the port number (Integer) submitted on the invocation (from parameter 5-6)
 * </ul>
 *
 * @author Chris Mair
 * @version $Revision$ - $Date$
 */
public class PortCommandHandler extends AbstractStubCommandHandler implements CommandHandler {

    public static final String HOST_KEY = "host";
    public static final String PORT_KEY = "port";

    /**
     * Constructor. Initialize the replyCode.
     */
    public PortCommandHandler() {
        setReplyCode(ReplyCodes.PORT_OK);
    }

    /**
     * Handle the command
     *
     * @throws UnknownHostException
     * @see org.mockftpserver.core.command.CommandHandler#handleCommand(org.mockftpserver.core.command.Command, org.mockftpserver.core.session.Session)
     */
    public void handleCommand(Command command, Session session, InvocationRecord invocationRecord) throws UnknownHostException {
        HostAndPort client = PortParser.parseHostAndPort(command.getParameters());
        LOG.debug("host=" + client.host + " port=" + client.port);
        session.setClientDataHost(client.host);
        session.setClientDataPort(client.port);
        invocationRecord.set(HOST_KEY, client.host);
        invocationRecord.set(PORT_KEY, new Integer(client.port));
        sendReply(session);
    }

}
