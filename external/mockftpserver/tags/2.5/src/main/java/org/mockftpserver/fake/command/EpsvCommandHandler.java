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

import java.net.InetAddress;

/**
 * CommandHandler for the EPSV command. Handler logic:
 * <ol>
 * <li>If the user has not logged in, then reply with 530</li>
 * <li>Otherwise, request the Session to switch to passive data connection mode. Return a reply code
 * of 229, along with response text including: "<i>(|||PORT|)</i>", where <i>PORT</i> is the 16-bit
 * TCP port address of the data connection on the server to which the client must connect.</li>
 * </ol>
 * See RFC2428 for more information.
 *
 * @author Chris Mair
 * @version $Revision$ - $Date$
 */
public class EpsvCommandHandler extends AbstractFakeCommandHandler {

    protected void handle(Command command, Session session) {
        verifyLoggedIn(session);
        int port = session.switchToPassiveMode();
        InetAddress server = session.getServerHost();
        LOG.debug("server=" + server + " port=" + port);
        sendReply(session, ReplyCodes.EPSV_OK, "epsv", list(Integer.toString(port)));
    }

}