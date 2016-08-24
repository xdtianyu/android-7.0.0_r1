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
import org.mockftpserver.core.session.SessionKeys;

/**
 * CommandHandler for the RNFR command. Handler logic:
 * <ol>
 * <li>If the user has not logged in, then reply with 530</li>
 * <li>If the required FROM pathname parameter is missing, then reply with 501</li>
 * <li>If the FROM pathname parameter does not specify a valid file or directory, then reply with 550</li>
 * <li>If the current user does not have read access to the path, then reply with 550</li>
 * <li>Otherwise, reply with 350 and store the FROM path in the session</li>
 * </ol>
 * The supplied pathname may be absolute or relative to the current directory.
 *
 * @author Chris Mair
 * @version $Revision$ - $Date$
 */
public class RnfrCommandHandler extends AbstractFakeCommandHandler {

    protected void handle(Command command, Session session) {
        verifyLoggedIn(session);
        String fromPath = getRealPath(session, command.getRequiredParameter(0));

        this.replyCodeForFileSystemException = ReplyCodes.READ_FILE_ERROR;
        verifyFileSystemCondition(getFileSystem().exists(fromPath), fromPath, "filesystem.doesNotExist");

        // User must have read permission to the file
        verifyReadPermission(session, fromPath);

        session.setAttribute(SessionKeys.RENAME_FROM, fromPath);
        sendReply(session, ReplyCodes.RNFR_OK, "rnfr");
    }

}