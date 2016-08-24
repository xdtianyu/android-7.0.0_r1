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
package org.mockftpserver.fake.command;

import org.mockftpserver.core.command.Command;
import org.mockftpserver.core.command.ReplyCodes;
import org.mockftpserver.core.session.Session;
import org.mockftpserver.core.session.SessionKeys;
import org.mockftpserver.fake.UserAccount;

/**
 * CommandHandler for the USER command. Handler logic:
 * <ol>
 * <li>If the required pathname parameter is missing, then reply with 501</li>
 * <li>If the user account configured for the named user is not valid, then reply with 530</li>
 * <li>If the named user does not need a password for login, then reply with 230</li>
 * <li>Otherwise, reply with 331</li>
 * </ol>
 *
 * @author Chris Mair
 * @version $Revision: 89 $ - $Date: 2008-08-02 08:07:44 -0400 (Sat, 02 Aug 2008) $
 */
public class UserCommandHandler extends AbstractFakeCommandHandler {

    protected void handle(Command command, Session session) {
        String username = command.getRequiredParameter(0);
        UserAccount userAccount = getServerConfiguration().getUserAccount(username);

        if (userAccount != null) {
            if (!validateUserAccount(username, session)) {
                return;
            }

            // If the UserAccount is configured to not require password for login
            if (!userAccount.isPasswordRequiredForLogin()) {
                login(userAccount, session, ReplyCodes.USER_LOGGED_IN_OK, "user.loggedIn");
                return;
            }
        }
        session.setAttribute(SessionKeys.USERNAME, username);
        sendReply(session, ReplyCodes.USER_NEED_PASSWORD_OK, "user.needPassword");
    }

}