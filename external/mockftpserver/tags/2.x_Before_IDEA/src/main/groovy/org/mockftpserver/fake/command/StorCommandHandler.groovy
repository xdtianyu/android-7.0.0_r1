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
import org.mockftpserver.core.session.SessionKeys
import org.mockftpserver.fake.filesystem.FileSystemException
/**
 * CommandHandler for the STOR command. Handler logic:
 * <ol>
 *  <li>If the user has not logged in, then reply with 530 and terminate</li>
 *  <li>If the required pathname parameter is missing, then reply with 501 and terminate</li>
 *  <li>Send an initial reply of 150</li>
 *  <li>If the pathname parameter does not specify a valid filename, then reply with 553 and terminate</li>
 *  <li>If file write/store fails, then reply with 553 and terminate</li>
 *  <li>Send a final reply with 226</li>
 * </ol>
 * 
 * @version $Revision: $ - $Date: $
 *
 * @author Chris Mair
 */
class StorCommandHandler extends AbstractFakeCommandHandler {

    protected void handle(Command command, Session session) {
        verifyLoggedIn(session)
        def path = getRealPath(session, getRequiredParameter(command))
        verifyForNewFile(!fileSystem.isDirectory(path), path)
        def parent = fileSystem.getParent(path)
        verifyForNewFile(fileSystem.isDirectory(parent), parent)
        
        sendReply(session, ReplyCodes.SEND_DATA_INITIAL_OK)
        def contents = session.readData()
        try {
            def out = fileSystem.createOutputStream(path, false)
            out.withStream { it.write(contents) }
            sendReply(session, ReplyCodes.SEND_DATA_FINAL_OK)
        }
        catch(FileSystemException e) {
            LOG.warn("Error handling command: $command; ${e}; path: ${path}")
            sendReply(session, ReplyCodes.NEW_FILE_ERROR, [path])
        }
    }

}