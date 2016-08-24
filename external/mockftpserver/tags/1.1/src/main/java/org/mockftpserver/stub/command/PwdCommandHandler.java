/*
 * Copyright 2007 the original author or authors.
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
package org.mockftpserver.stub.command;

import org.mockftpserver.core.command.Command;
import org.mockftpserver.core.command.CommandHandler;
import org.mockftpserver.core.command.InvocationRecord;
import org.mockftpserver.core.command.ReplyCodes;
import org.mockftpserver.core.session.Session;

/**
 * CommandHandler for the PWD (Print Working Directory) and XPWD commands. By default, return 
 * an empty directory name, along with a reply code of 257. You can customize the returned 
 * directory name by setting the <code>directory</code> property.
 * <p>
 * Each invocation record stored by this CommandHandler contains no data elements.
 * 
 * @version $Revision$ - $Date$
 * 
 * @author Chris Mair
 */
public final class PwdCommandHandler extends AbstractStubCommandHandler implements CommandHandler {

    private String directory = "";
    
    /**
     * Constructor. Initialize the replyCode. 
     */
    public PwdCommandHandler() {
        setReplyCode(ReplyCodes.PWD_OK);
    }
    
    /**
     * @see org.mockftpserver.core.command.CommandHandler#handleCommand(Command, Session, InvocationRecord)
     */
    public void handleCommand(Command command, Session session, InvocationRecord invocationRecord) {
        sendReply(session, quotes(directory));
    }

    /**
     * Set the directory String to be returned by this command
     * @param directory - the directory
     */
    public void setDirectory(String response) {
        this.directory = response;
    }
    
}
