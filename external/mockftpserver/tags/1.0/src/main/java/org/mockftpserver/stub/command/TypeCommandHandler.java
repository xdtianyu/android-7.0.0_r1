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

import org.apache.log4j.Logger;
import org.mockftpserver.core.command.Command;
import org.mockftpserver.core.command.CommandHandler;
import org.mockftpserver.core.command.InvocationRecord;
import org.mockftpserver.core.command.ReplyCodes;
import org.mockftpserver.core.session.Session;

/**
 * CommandHandler for the TYPE command. Send back a reply code of 200.
 * <p>
 * Each invocation record stored by this CommandHandler includes the following data element key/values:
 * <ul>
 *    <li>{@link #TYPE_INFO_KEY} ("typeInfo") - the type information submitted on the 
 *          invocation, which is a String[2] containing the first two command parameter values.
 * </ul>
 * 
 * @version $Revision$ - $Date$
 * 
 * @author Chris Mair
 */
public final class TypeCommandHandler extends AbstractStubCommandHandler implements CommandHandler {

    public static final String TYPE_INFO_KEY = "typeInfo";

    private static final Logger LOG = Logger.getLogger(TypeCommandHandler.class);
    
    /**
     * Constructor. Initialize the replyCode. 
     */
    public TypeCommandHandler() {
        setReplyCode(ReplyCodes.TYPE_OK);
    }
    
    /**
     * @see org.mockftpserver.core.command.CommandHandler#handleCommand(Command, Session, InvocationRecord)
     */
    public void handleCommand(Command command, Session session, InvocationRecord invocationRecord) {
        LOG.debug("Processing TYPE: " + command);
        String type = command.getRequiredString(0);
        String format = command.getOptionalString(1);
        invocationRecord.set(TYPE_INFO_KEY, new String[] {type, format});
        sendReply(session);
    }
    
}
