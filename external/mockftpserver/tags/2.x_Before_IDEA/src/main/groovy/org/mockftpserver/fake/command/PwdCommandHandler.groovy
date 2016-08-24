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
package org.mockftpserver.fake.command

import org.mockftpserver.fake.command.AbstractFakeCommandHandlerimport org.mockftpserver.core.command.Commandimport org.mockftpserver.core.session.Sessionimport org.mockftpserver.core.session.SessionKeys
import org.mockftpserver.core.command.ReplyCodes

/**
 * CommandHandler for the PWD command. Handler logic:
 * <ol>
 *  <li>If the required "current directory" property is missing from the session, then reply with 550</li>
 *  <li>Otherwise, reply with 257 and the current directory</li>
 * </ol>
 * 
 * @version $Revision: $ - $Date: $
 *
 * @author Chris Mair
 */
class PwdCommandHandler extends AbstractFakeCommandHandler {

    protected void handle(Command command, Session session) {
        def currentDirectory = session.getAttribute(SessionKeys.CURRENT_DIRECTORY)
        verifyForExistingFile(currentDirectory, currentDirectory)
        sendReply(session, ReplyCodes.PWD_OK, [currentDirectory])
    }

}