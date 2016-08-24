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

/**
 * CommandHandler for the STAT command. Handler logic:
 * <ol>
 * <li>Reply with 211 along with the system status text that has been configured on the
 * {@link org.mockftpserver.fake.FakeFtpServer}.</li>
 * </ol>
 *
 * @author Chris Mair
 * @version $Revision$ - $Date$
 * @see org.mockftpserver.fake.ServerConfiguration
 * @see org.mockftpserver.fake.FakeFtpServer
 */
public class StatCommandHandler extends AbstractFakeCommandHandler {

    protected void handle(Command command, Session session) {
        String systemStatus = getServerConfiguration().getSystemStatus();
        sendReply(session, ReplyCodes.STAT_SYSTEM_OK, "stat", list(systemStatus));
    }

}