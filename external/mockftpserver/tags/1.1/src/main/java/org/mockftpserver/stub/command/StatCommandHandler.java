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
 * CommandHandler for the STAT (Status) command. By default, return empty status information,
 * along with a reply code of 211 if no pathname parameter is specified or 213 if a 
 * pathname is specified. You can customize the returned status information by setting 
 * the <code>status</code> property. 
 * <p>
 * Each invocation record stored by this CommandHandler includes the following data element key/values:
 * <ul>
 *    <li>{@link #PATHNAME_KEY} ("pathname") - the pathname of the directory (or file) submitted on the 
 *          invocation (the first command parameter); this parameter is optional, so the value may be null. 
 * </ul>
 * 
 * @see SystCommandHandler
 * 
 * @version $Revision$ - $Date$
 * 
 * @author Chris Mair
 */
public final class StatCommandHandler extends AbstractStubCommandHandler implements CommandHandler {

    public static final String PATHNAME_KEY = "pathname";

    private String status = "";

    /**
     * Constructor.  
     */
    public StatCommandHandler() {
        // Do not initialize replyCode -- will be set dynamically
    }
    
    /**
     * @see org.mockftpserver.core.command.CommandHandler#handleCommand(Command, Session, InvocationRecord)
     */
    public void handleCommand(Command command, Session session, InvocationRecord invocationRecord) {
        String pathname = command.getOptionalString(0);
        invocationRecord.set(PATHNAME_KEY, pathname);

        // Only use dynamic reply code if the replyCode property was NOT explicitly set
        if (replyCode == 0) {
            int code = (pathname == null) ? ReplyCodes.STAT_SYSTEM_OK : ReplyCodes.STAT_FILE_OK;
            sendReply(session, code, replyMessageKey, replyText, new String[] { status });
        }
        else {
            sendReply(session, status);
        }
    }

    /**
     * Set the contents of the status to send back as the reply text for this command
     * @param status - the status
     */
    public void setStatus(String status) {
        this.status = status;
    }
    
}
