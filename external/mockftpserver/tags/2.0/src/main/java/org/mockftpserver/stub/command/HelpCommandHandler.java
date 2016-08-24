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
 * CommandHandler for the HELP command. By default, return an empty help message,
 * along with a reply code of 214. You can customize the returned help message by
 * setting the <code>helpMessage</code> property.
 * <p>
 * Each invocation record stored by this CommandHandler includes the following data element key/values:
 * <ul>
 * <li>{@link #COMMAND_NAME_KEY} ("commandName") - the command name optionally submitted on
 * the invocation (the first command parameter). May be null.
 * </ul>
 *
 * @author Chris Mair
 * @version $Revision$ - $Date$
 */
public final class HelpCommandHandler extends AbstractStubCommandHandler implements CommandHandler {

    public static final String COMMAND_NAME_KEY = "commandName";

    private String helpMessage = "";

    /**
     * Constructor. Initialize the replyCode.
     */
    public HelpCommandHandler() {
        setReplyCode(ReplyCodes.HELP_OK);
    }

    /**
     * @see org.mockftpserver.core.command.AbstractTrackingCommandHandler#handleCommand(org.mockftpserver.core.command.Command, org.mockftpserver.core.session.Session, org.mockftpserver.core.command.InvocationRecord)
     */
    public void handleCommand(Command command, Session session, InvocationRecord invocationRecord) {
        invocationRecord.set(COMMAND_NAME_KEY, command.getOptionalString(0));
        sendReply(session, helpMessage);
    }

    /**
     * Set the help message String to be returned by this command
     *
     * @param helpMessage - the help message
     */
    public void setHelpMessage(String helpMessage) {
        this.helpMessage = helpMessage;
    }

}
