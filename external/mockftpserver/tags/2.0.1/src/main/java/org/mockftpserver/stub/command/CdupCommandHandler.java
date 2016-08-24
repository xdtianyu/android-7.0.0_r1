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
 * CommandHandler for the CDUP (Change To Parent Directory) command. Send back a reply code of 250.
 * <p>
 * Each invocation record stored by this CommandHandler contains no data elements.
 *
 * @author Chris Mair
 * @version $Revision$ - $Date$
 */
public class CdupCommandHandler extends AbstractStubCommandHandler implements CommandHandler {

    /**
     * Constructor. Initialize the replyCode.
     */
    public CdupCommandHandler() {
        setReplyCode(ReplyCodes.CDUP_OK);
    }

    public void handleCommand(Command command, Session session, InvocationRecord invocationRecord) {
        sendReply(session);
    }

}
