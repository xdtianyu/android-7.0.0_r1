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
 * CommandHandler for the RNTO command. Handler logic:
 * <ol>
 *  <li>If the user has not logged in, then reply with 530</li>
 *  <li>If this command was not preceded by a valid RNFR command, then reply with 503</li>
 *  <li>If the required TO pathname parameter is missing, then reply with 501</li>
 *  <li>If the TO pathname parameter does not specify a valid filename, then reply with 553</li>
 *  <li>If the TO pathname parameter specifies an existing directory, then reply with 553</li>
 *  <li>If the rename operation fails, then reply with 553</li>
 *  <li>Otherwise, reply with 250 and remove the FROM path stored in the session by the RNFR command</li>
 * </ol>
 * The supplied pathname may be absolute or relative to the current directory.
 * 
 * @version $Revision: $ - $Date: $
 *
 * @author Chris Mair
 */
class RntoCommandHandler extends AbstractFakeCommandHandler {

    protected void handle(Command command, Session session) {
        verifyLoggedIn(session)
        def toPath = getRealPath(session, getRequiredParameter(command))
        def fromPath = getRequiredSessionAttribute(session, SessionKeys.RENAME_FROM)

        verifyForNewFile(!fileSystem.isDirectory(toPath), toPath)
        verifyForNewFile(fileSystem.rename(fromPath, toPath), toPath)

        // TODO use custom message, including FROM and TO path?
        session.removeAttribute(SessionKeys.RENAME_FROM)
        sendReply(session, ReplyCodes.RNTO_OK)
    }

}