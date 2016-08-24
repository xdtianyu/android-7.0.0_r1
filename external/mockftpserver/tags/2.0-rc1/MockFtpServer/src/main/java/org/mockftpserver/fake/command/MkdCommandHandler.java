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
import org.mockftpserver.fake.filesystem.DirectoryEntry;

/**
 * CommandHandler for the MKD command. Handler logic:
 * <ol>
 * <li>If the user has not logged in, then reply with 530</li>
 * <li>If the required pathname parameter is missing, then reply with 501</li>
 * <li>If the parent directory of the specified pathname does not exist, then reply with 550</li>
 * <li>If the pathname parameter specifies an existing file or directory, or if the create directory fails, then reply with 550</li>
 * <li>If the current user does not have write and execute access to the parent directory, then reply with 550</li>
 * <li>Otherwise, reply with 257</li>
 * </ol>
 * The supplied pathname may be absolute or relative to the current directory.
 *
 * @author Chris Mair
 * @version $Revision$ - $Date$
 */
public class MkdCommandHandler extends AbstractFakeCommandHandler {

    protected void handle(Command command, Session session) {
        verifyLoggedIn(session);
        String path = getRealPath(session, command.getRequiredParameter(0));
        String parent = getFileSystem().getParent(path);

        this.replyCodeForFileSystemException = ReplyCodes.READ_FILE_ERROR;
        verifyFileSystemCondition(getFileSystem().exists(parent), parent, "filesystem.doesNotExist");
        verifyFileSystemCondition(!getFileSystem().exists(path), path, "filesystem.alreadyExists");

        // User must have write permission to the parent directory
        verifyWritePermission(session, parent);

        // User must have execute permission to the parent directory
        verifyExecutePermission(session, parent);

        DirectoryEntry dirEntry = new DirectoryEntry(path);
        getFileSystem().add(dirEntry);
        dirEntry.setPermissions(getUserAccount(session).getDefaultPermissionsForNewDirectory());

        sendReply(session, ReplyCodes.MKD_OK, "mkd", list(path));
    }

}