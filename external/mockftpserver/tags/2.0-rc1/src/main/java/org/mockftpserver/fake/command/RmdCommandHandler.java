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
 * CommandHandler for the RMD command. Handler logic:
 * <ol>
 * <li>If the user has not logged in, then reply with 530</li>
 * <li>If the required pathname parameter is missing, then reply with 501</li>
 * <li>If the pathname parameter does not specify an existing, empty directory, then reply with 550</li>
 * <li>Otherwise, reply with 250</li>
 * </ol>
 * The supplied pathname may be absolute or relative to the current directory.
 *
 * @author Chris Mair
 * @version $Revision: 136 $ - $Date: 2008-10-23 22:17:29 -0400 (Thu, 23 Oct 2008) $
 */
public class RmdCommandHandler extends AbstractFakeCommandHandler {

    protected void handle(Command command, Session session) {
        verifyLoggedIn(session);
        String path = getRealPath(session, command.getRequiredParameter(0));

        this.replyCodeForFileSystemException = ReplyCodes.READ_FILE_ERROR;
        verifyFileSystemCondition(getFileSystem().exists(path), path, "filesystem.doesNotExist");
        verifyFileSystemCondition(getFileSystem().isDirectory(path), path, "filesystem.isNotADirectory");
        verifyFileSystemCondition(getFileSystem().listNames(path).size() == 0, path, "filesystem.directoryIsNotEmpty");

        // User must have write permission to the parent directory
        verifyWritePermission(session, getFileSystem().getParent(path));

        getFileSystem().delete(path);
        sendReply(session, ReplyCodes.RMD_OK, "rmd", list(path));
    }

}