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

import java.util.ArrayList;
import java.util.List;

/**
 * CommandHandler for the USER command. The <code>passwordRequired</code> property defaults to true,
 * indicating that a password is required following the user name. If true, this command handler
 * returns a reply of 331. If false, return a reply of 230.
 * <p/>
 * Each invocation record stored by this CommandHandler includes the following data element key/values:
 * <ul>
 * <li>{@link #USERNAME_KEY} ("username") - the user name submitted on the invocation (the first command parameter)
 * </ul>
 *
 * @author Chris Mair
 * @version $Revision$ - $Date$
 */
public final class UserCommandHandler extends AbstractStubCommandHandler implements CommandHandler {

    public static final String USERNAME_KEY = "username";

    private boolean passwordRequired = true;
    private List usernames = new ArrayList();

    /**
     * Constructor.
     */
    public UserCommandHandler() {
        // Do not initialize replyCode -- will be set dynamically
    }

    /**
     * @see org.mockftpserver.core.command.CommandHandler#handleCommand(org.mockftpserver.core.command.Command, org.mockftpserver.core.session.Session)
     */
    public void handleCommand(Command command, Session session, InvocationRecord invocationRecord) {
        usernames.add(command.getRequiredParameter(0));
        invocationRecord.set(USERNAME_KEY, command.getRequiredParameter(0));

        // Only use dynamic reply code if the replyCode property was NOT explicitly set
        if (replyCode == 0) {
            int code = (passwordRequired) ? ReplyCodes.USER_NEED_PASSWORD_OK : ReplyCodes.USER_LOGGED_IN_OK;
            sendReply(session, code, replyMessageKey, replyText, null);
        } else {
            sendReply(session);
        }
    }

    /**
     * Return true if a password is required at login. See {@link #setPasswordRequired(boolean)}.
     *
     * @return the passwordRequired flag
     */
    public boolean isPasswordRequired() {
        return passwordRequired;
    }

    /**
     * Set true to indicate that a password is required. If true, this command handler returns a reply
     * of 331. If false, return a reply of 230.
     *
     * @param passwordRequired - is a password required for login
     */
    public void setPasswordRequired(boolean passwordRequired) {
        this.passwordRequired = passwordRequired;
    }

}
