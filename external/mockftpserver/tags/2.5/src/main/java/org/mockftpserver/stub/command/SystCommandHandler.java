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
import org.mockftpserver.core.util.Assert;

/**
 * CommandHandler for the SYST (System) command. Send back a reply code of 215. By default,
 * return "WINDOWS" as the system name. You can customize the returned name by
 * setting the <code>systemName</code> property.
 * <p/>
 * See the available system names listed in the Assigned Numbers document
 * (<a href="http://www.ietf.org/rfc/rfc943">RFC 943</a>).
 * <p/>
 * Each invocation record stored by this CommandHandler contains no data elements.
 *
 * @author Chris Mair
 * @version $Revision$ - $Date$
 */
public class SystCommandHandler extends AbstractStubCommandHandler implements CommandHandler {

    private String systemName = "WINDOWS";

    /**
     * Constructor. Initialize the replyCode.
     */
    public SystCommandHandler() {
        setReplyCode(ReplyCodes.SYST_OK);
    }

    public void handleCommand(Command command, Session session, InvocationRecord invocationRecord) {
        sendReply(session, quotes(systemName));
    }

    /**
     * Set the systemName String to be returned by this command
     *
     * @param systemName - the systemName
     */
    public void setSystemName(String systemName) {
        Assert.notNull(systemName, "systemName");
        this.systemName = systemName;
    }

}
