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

import org.mockftpserver.core.command.Command;
import org.mockftpserver.core.command.CommandHandler;
import org.mockftpserver.core.command.InvocationRecord;
import org.mockftpserver.core.command.ReplyCodes;
import org.mockftpserver.core.session.Session;

import java.io.IOException;
import java.net.InetAddress;

/**
 * CommandHandler for the EPSV (Extended Address Passive Mode) command. Request the Session to switch
 * to passive data connection mode. Return a reply code of 229, along with response text of the form:
 * "<i>Entering Extended Passive Mode (|||PORT|)</i>", where <i>PORT</i> is the 16-bit TCP port
 * address of the data connection on the server to which the client must connect.
 * See RFC2428 for more information.
 * <p/>
 * Each invocation record stored by this CommandHandler contains no data elements.
 *
 * @author Chris Mair
 * @version $Revision$ - $Date$
 */
public class EpsvCommandHandler extends AbstractStubCommandHandler implements CommandHandler {

    /**
     * Constructor. Initialize the replyCode.
     */
    public EpsvCommandHandler() {
        setReplyCode(ReplyCodes.EPSV_OK);
    }

    /**
     * @throws java.io.IOException
     * @see org.mockftpserver.core.command.CommandHandler#handleCommand(org.mockftpserver.core.command.Command, org.mockftpserver.core.session.Session)
     */
    public void handleCommand(Command command, Session session, InvocationRecord invocationRecord)
            throws IOException {

        int port = session.switchToPassiveMode();
        InetAddress server = session.getServerHost();
        LOG.debug("server=" + server + " port=" + port);
        sendReply(session, Integer.toString(port));
    }

}