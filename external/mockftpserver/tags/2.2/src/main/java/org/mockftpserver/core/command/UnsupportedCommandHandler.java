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
package org.mockftpserver.core.command;

import org.mockftpserver.core.session.Session;

/**
 * CommandHandler that encapsulates the sending of the reply when a requested command is not
 * recognized/supported. Send back a reply code of 502, indicating command not implemented.
 * <p>
 * Note that this is a "special" CommandHandler, in that it handles any unrecognized command,
 * rather than an explicit FTP command.
 * <p>
 * Each invocation record stored by this CommandHandler contains no data elements.
 *
 * @author Chris Mair
 * @version $Revision$ - $Date$
 */
public final class UnsupportedCommandHandler extends AbstractStaticReplyCommandHandler implements CommandHandler {

    /**
     * Constructor. Initiate the replyCode.
     */
    public UnsupportedCommandHandler() {
        setReplyCode(ReplyCodes.COMMAND_NOT_SUPPORTED);
    }

    /**
     * @see org.mockftpserver.core.command.AbstractTrackingCommandHandler#handleCommand(org.mockftpserver.core.command.Command, org.mockftpserver.core.session.Session, org.mockftpserver.core.command.InvocationRecord)
     */
    public void handleCommand(Command command, Session session, InvocationRecord invocationRecord) {
        LOG.warn("No CommandHandler is defined for command [" + command.getName() + "]");
        sendReply(session, command.getName());
    }

}