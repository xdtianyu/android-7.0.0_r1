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
 * CommandHandler for the PASS (Password) command. Send back a reply code of 230. 
 * <p>
 * Each invocation record stored by this CommandHandler includes the following data element key/values:
 * <ul>
 *    <li>"password" - the password submitted on the invocation (the first command parameter)
 * </ul>
 * 
 * @version $Revision$ - $Date$
 * 
 * @author Chris Mair
 */
public final class PassCommandHandler extends AbstractStubCommandHandler implements CommandHandler {

    public static final String PASSWORD_KEY = "password";

    /**
     * Constructor. Initialize the replyCode. 
     */
    public PassCommandHandler() {
        setReplyCode(ReplyCodes.PASS_OK);
    }
    
    /**
     * @see org.mockftpserver.core.command.CommandHandler#handleCommand(Command, Session, InvocationRecord)
     */
    public void handleCommand(Command command, Session session, InvocationRecord invocationRecord) {
        invocationRecord.set(PASSWORD_KEY, command.getRequiredString(0));
        sendReply(session);
    }
    
}
