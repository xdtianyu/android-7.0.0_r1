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
package org.mockftpserver.core.command;

import org.mockftpserver.core.session.Session;

/**
 * Interface for classes that can handle an FTP command.
 * 
 * @version $Revision$ - $Date$
 * 
 * @author Chris Mair
 */
public interface CommandHandler {

    /**
     * Handle the specified command for the session. This method is declared to throw 
     * Exception, allowing CommandHandler implementations to avoid unnecessary
     * exception-handling. All checked exceptions are expected to be wrapped and handled 
     * by the caller.
     * 
     * @param command - the Command to be handled
     * @param session - the session on which the Command was submitted
     * 
     * @throws Exception
     */
    public void handleCommand(Command command, Session session) throws Exception;

}