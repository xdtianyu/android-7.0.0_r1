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
 * CommandHandler for the ALLO command. Handler logic:
 * <ol>
 * <li>If the user has not logged in, then reply with 530</li>
 * <li>Otherwise, reply with 200</li>
 * </ol>
 *
 * @author Chris Mair
 * @version $Revision: 80 $ - $Date: 2008-07-07 22:15:50 -0400 (Mon, 07 Jul 2008) $
 */
public class AlloCommandHandler extends AbstractFakeCommandHandler {

    protected void handle(Command command, Session session) {
        verifyLoggedIn(session);
        sendReply(session, ReplyCodes.ALLO_OK, "allo");
    }

}