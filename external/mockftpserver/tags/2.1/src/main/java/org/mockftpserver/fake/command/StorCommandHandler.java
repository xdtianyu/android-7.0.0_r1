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

/**
 * CommandHandler for the STOR command. Handler logic:
 * <ol>
 * <li>If the user has not logged in, then reply with 530 and terminate</li>
 * <li>If the required pathname parameter is missing, then reply with 501 and terminate</li>
 * <li>If the pathname parameter does not specify a valid filename, then reply with 553 and terminate</li>
 * <li>If the current user does not have write access to the named file, if it already exists, or else to its
 * parent directory, then reply with 553 and terminate</li>
 * <li>If the current user does not have execute access to the parent directory, then reply with 553 and terminate</li>
 * <li>Send an initial reply of 150</li>
 * <li>Read all available bytes from the data connection and write out to the named file in the server file system</li>
 * <li>If file write/store fails, then reply with 553 and terminate</li>
 * <li>Send a final reply with 226</li>
 * </ol>
 *
 * @author Chris Mair
 * @version $Revision$ - $Date$
 */
public class StorCommandHandler extends AbstractStoreFileCommandHandler {

    /**
     * @return the message key for the reply message sent with the final (226) reply
     */
    protected String getMessageKey() {
        return "stor";
    }

}