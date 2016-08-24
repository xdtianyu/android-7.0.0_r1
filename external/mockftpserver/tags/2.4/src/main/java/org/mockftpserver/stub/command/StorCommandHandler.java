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
import org.mockftpserver.core.command.InvocationRecord;
import org.mockftpserver.core.session.Session;

/**
 * CommandHandler for the STOR (Store) command. Send back two replies on the control connection: a
 * reply code of 150 and another of 226.
 * <p/>
 * Each invocation record stored by this CommandHandler includes the following data element key/values:
 * <ul>
 * <li>{@link #PATHNAME_KEY} ("pathname") - the pathname of the directory submitted on the invocation (the first command parameter)
 * <li>{@link #FILE_CONTENTS_KEY} ("fileContents") - the file contents (<code>byte[]</code>) sent on the data connection
 * </ul>
 *
 * @author Chris Mair
 * @version $Revision$ - $Date$
 */
public class StorCommandHandler extends AbstractStorCommandHandler {

    /**
     * @see org.mockftpserver.stub.command.AbstractStubDataCommandHandler#beforeProcessData(org.mockftpserver.core.command.Command, org.mockftpserver.core.session.Session, org.mockftpserver.core.command.InvocationRecord)
     */
    protected void beforeProcessData(Command command, Session session, InvocationRecord invocationRecord) throws Exception {
        String filename = command.getRequiredParameter(0);
        invocationRecord.set(PATHNAME_KEY, filename);
    }

}
