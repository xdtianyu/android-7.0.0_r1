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
 * CommandHandler for the QUIT command. Return a reply code of 221 and close the current session.
 *
 * @author Chris Mair
 * @version $Revision$ - $Date$
 */
public class QuitCommandHandler extends AbstractFakeCommandHandler {

    protected void handle(Command command, Session session) {
        sendReply(session, ReplyCodes.QUIT_OK, "quit");
        session.close();
    }

}