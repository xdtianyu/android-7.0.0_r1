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
 * CommandHandler for the CDUP command. Handler logic:
 * <ol>
 * <li>If the user has not logged in, then reply with 530</li>
 * <li>If the current directory has no parent or if the current directory cannot be changed, then reply with 550 and terminate</li>
 * <li>If the current user does not have execute access to the parent directory, then reply with 550 and terminate</li>
 * <li>Otherwise, reply with 200 and change the current directory stored in the session to the parent directory</li>
 * </ol>
 *
 * @author Chris Mair
 * @version $Revision$ - $Date$
 */
public class CdupCommandHandler extends AbstractFakeCommandHandler {

    protected void handle(Command command, Session session) {
        verifyLoggedIn(session);
        String currentDirectory = (String) getRequiredSessionAttribute(session, SessionKeys.CURRENT_DIRECTORY);
        String path = getFileSystem().getParent(currentDirectory);

        this.replyCodeForFileSystemException = ReplyCodes.READ_FILE_ERROR;
        verifyFileSystemCondition(notNullOrEmpty(path), currentDirectory, "filesystem.parentDirectoryDoesNotExist");
        verifyFileSystemCondition(getFileSystem().isDirectory(path), path, "filesystem.isNotADirectory");

        // User must have execute permission to the parent directory
        verifyExecutePermission(session, path);

        session.setAttribute(SessionKeys.CURRENT_DIRECTORY, path);
        sendReply(session, ReplyCodes.CDUP_OK, "cdup", list(path));
    }

}