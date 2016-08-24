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
 * CommandHandler for the PWD command. Handler logic:
 * <ol>
 * <li>If the required "current directory" property is missing from the session, then reply with 550</li>
 * <li>Otherwise, reply with 257 and the current directory</li>
 * </ol>
 *
 * @author Chris Mair
 * @version $Revision: 164 $ - $Date: 2008-11-18 21:00:39 -0500 (Tue, 18 Nov 2008) $
 */
public class PwdCommandHandler extends AbstractFakeCommandHandler {

    protected void handle(Command command, Session session) {
        String currentDirectory = (String) session.getAttribute(SessionKeys.CURRENT_DIRECTORY);
        this.replyCodeForFileSystemException = ReplyCodes.READ_FILE_ERROR;
        verifyFileSystemCondition(notNullOrEmpty(currentDirectory), currentDirectory, "filesystem.currentDirectoryNotSet");
        sendReply(session, ReplyCodes.PWD_OK, "pwd", list(currentDirectory));
    }

}