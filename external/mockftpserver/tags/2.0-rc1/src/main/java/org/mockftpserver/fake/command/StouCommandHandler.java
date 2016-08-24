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

/**
 * CommandHandler for the STOU command. Handler logic:
 * <ol>
 * <li>If the user has not logged in, then reply with 530 and terminate</li>
 * <li>Create a new file within the current directory with a unique name</li>
 * <li>Send an initial reply of 150</li>
 * <li>Read all available bytes from the data connection and write out to the unique file in the server file system</li>
 * <li>If file write/store fails, then reply with 553 and terminate</li>
 * <li>Send a final reply with 226, along with the new unique filename</li>
 * </ol>
 *
 * @author Chris Mair
 * @version $Revision: 81 $ - $Date: 2008-07-11 15:28:15 -0400 (Fri, 11 Jul 2008) $
 */
public class StouCommandHandler extends AbstractStoreFileCommandHandler {

    /**
     * @return the message key for the reply message sent with the final (226) reply
     */
    protected String getMessageKey() {
        return "stou";
    }

    /**
     * Return the path (absolute or relative) for the output file.
     */
    protected String getOutputFile(Command command) {
        String baseName = defaultIfNullOrEmpty(command.getOptionalString(0), "Temp");
        String suffix = Long.toString(System.currentTimeMillis());
        return baseName + suffix;
    }

}