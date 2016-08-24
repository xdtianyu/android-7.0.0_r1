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

/**
 * CommandHandler that encapsulates the sending of the reply for the initial connection from the
 * FTP client to the server. Send back a reply code of 220, indicating a successful connection.
 * <p/>
 * Note that this is a "special" CommandHandler, in that it handles the initial
 * connection from the client, rather than an explicit FTP command.
 *
 * @author Chris Mair
 * @version $Revision: 58 $ - $Date: 2008-05-26 21:40:29 -0400 (Mon, 26 May 2008) $
 */
public class ConnectCommandHandler extends AbstractFakeCommandHandler {

    protected void handle(Command command, Session session) {
        sendReply(session, ReplyCodes.CONNECT_OK);
    }

}