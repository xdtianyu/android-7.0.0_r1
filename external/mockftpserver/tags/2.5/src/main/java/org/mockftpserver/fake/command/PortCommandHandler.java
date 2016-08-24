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
package org.mockftpserver.fake.command;

import org.mockftpserver.core.command.Command;
import org.mockftpserver.core.command.ReplyCodes;
import org.mockftpserver.core.session.Session;
import org.mockftpserver.core.util.HostAndPort;
import org.mockftpserver.core.util.PortParser;

/**
 * CommandHandler for the PORT command. Handler logic:
 * <ol>
 * <li>Parse the client data host (InetAddress) submitted from parameters 1-4
 * <li>Parse the port number submitted on the invocation from parameter 5-6
 * <li>Send backa a reply of 200</li>
 * </ol>
 *
 * @author Chris Mair
 * @version $Revision$ - $Date$
 * @see org.mockftpserver.core.util.PortParser
 */
public class PortCommandHandler extends AbstractFakeCommandHandler {

    protected void handle(Command command, Session session) {
        HostAndPort client = PortParser.parseHostAndPort(command.getParameters());
        LOG.debug("host=" + client.host + " port=" + client.port);
        session.setClientDataHost(client.host);
        session.setClientDataPort(client.port);
        sendReply(session, ReplyCodes.PORT_OK, "port");
    }

}