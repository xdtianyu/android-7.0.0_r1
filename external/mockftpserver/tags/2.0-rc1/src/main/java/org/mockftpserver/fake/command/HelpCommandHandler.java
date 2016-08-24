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
import org.mockftpserver.core.util.StringUtil;

import java.util.Arrays;
import java.util.List;

/**
 * CommandHandler for the HELP command. Handler logic:
 * <ol>
 * <li>If the optional command-name parameter is specified, then reply with 214 along with the
 * help text configured for that command (or empty if none)</li>
 * <li>Otherwise, reply with 214 along with the configured default help text that has been configured
 * (or empty if none)</li>
 * </ol>
 *
 * @author Chris Mair
 * @version $Revision: 89 $ - $Date: 2008-08-02 08:07:44 -0400 (Sat, 02 Aug 2008) $
 * @see org.mockftpserver.fake.ServerConfiguration
 * @see org.mockftpserver.fake.FakeFtpServer
 */
public class HelpCommandHandler extends AbstractFakeCommandHandler {

    protected void handle(Command command, Session session) {
        List parameters = Arrays.asList(command.getParameters());
        String key = StringUtil.join(parameters, " ");
        String help = getServerConfiguration().getHelpText(key);
        if (help == null) {
            sendReply(session, ReplyCodes.HELP_OK, "help.noHelpTextDefined", list(key));
            ;
        } else {
            sendReply(session, ReplyCodes.HELP_OK, "help", list(help));
        }
    }

}