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
package org.mockftpserver.fake.command;

import org.mockftpserver.core.command.Command;
import org.mockftpserver.core.command.ReplyCodes;
import org.mockftpserver.core.session.Session;
import org.mockftpserver.core.util.HostAndPort;
import org.mockftpserver.core.util.PortParser;

/**
 * CommandHandler for the EPRT command. Handler logic:
 * <ol>
 * <li>Parse the client network address (InetAddress) and port number from the (single)
 *     parameter string of the form: "EPRT<space><d><net-prt><d><net-addr><d><tcp-port><d>".
 *     The client network address can be in IPv4 format (e.g., "132.235.1.2") or
 *     IPv6 format (e.g., "1080::8:800:200C:417A")     
 * <li>Send back a reply of 200</li>
 * </ol>
 * See RFC2428 for more information.
 *
 * @author Chris Mair
 * @version $Revision$ - $Date$
 */
public class EprtCommandHandler extends AbstractFakeCommandHandler {

    protected void handle(Command command, Session session) {
        String parameter = command.getRequiredParameter(0);
        HostAndPort client = PortParser.parseExtendedAddressHostAndPort(parameter);
        LOG.debug("host=" + client.host + " port=" + client.port);
        session.setClientDataHost(client.host);
        session.setClientDataPort(client.port);
        sendReply(session, ReplyCodes.EPRT_OK, "eprt");
    }

}