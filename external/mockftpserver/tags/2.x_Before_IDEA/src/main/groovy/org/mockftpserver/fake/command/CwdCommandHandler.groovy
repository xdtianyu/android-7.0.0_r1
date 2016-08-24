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
 * CommandHandler for the CWD command. Handler logic:
 * <ol>
 *  <li>If the user has not logged in, then reply with 530</li>
 *  <li>If the required pathname parameter is missing, then reply with 501</li>
 *  <li>If the pathname parameter does not specify an existing directory, then reply with 550</li>
 *  <li>Otherwise, reply with 250 and change the current directory stored in the session</li>
 * </ol>
 * The supplied pathname may be absolute or relative to the current directory.
 * 
 * @version $Revision: $ - $Date: $
 *
 * @author Chris Mair
 */
class CwdCommandHandler extends AbstractFakeCommandHandler {

    protected void handle(Command command, Session session) {
        verifyLoggedIn(session)
        def path = getRealPath(session, getRequiredParameter(command))

        verifyForExistingFile(fileSystem.exists(path), path)
        verifyForExistingFile(fileSystem.isDirectory(path), path)

        session.setAttribute(SessionKeys.CURRENT_DIRECTORY, path)
        sendReply(session, ReplyCodes.CWD_OK, [path])
    }

}