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
import org.mockftpserver.core.session.Session;

/**
 * CommandHandler for the NLST command. Return the configured directory listing on the data
 * connection, along with two replies on the control connection: a reply code of 150 and
 * another of 226. By default, return an empty directory listing. You can customize the
 * returned directory listing by setting the <code>directoryListing</code> property.
 * <p>
 * Each invocation record stored by this CommandHandler includes the following data element key/values:
 * <ul>
 * <li>{@link #PATHNAME_KEY} ("pathname") - the pathname of the directory (or file) submitted on the
 * invocation (the first command parameter); this parameter is optional, so the value may be null.
 * </ul>
 *
 * @author Chris Mair
 * @version $Revision$ - $Date$
 */
public class NlstCommandHandler extends AbstractStubDataCommandHandler implements CommandHandler {

    public static final String PATHNAME_KEY = "pathname";

    private String directoryListing = "";

    /**
     * @see org.mockftpserver.stub.command.AbstractStubDataCommandHandler#beforeProcessData(org.mockftpserver.core.command.Command, org.mockftpserver.core.session.Session, org.mockftpserver.core.command.InvocationRecord)
     */
    protected void beforeProcessData(Command command, Session session, InvocationRecord invocationRecord) throws Exception {
        invocationRecord.set(PATHNAME_KEY, command.getOptionalString(0));
    }

    /**
     * @see org.mockftpserver.stub.command.AbstractStubDataCommandHandler#processData(org.mockftpserver.core.command.Command, org.mockftpserver.core.session.Session, org.mockftpserver.core.command.InvocationRecord)
     */
    protected void processData(Command command, Session session, InvocationRecord invocationRecord) {
        session.sendData(directoryListing.getBytes(), directoryListing.length());
    }

    /**
     * Set the contents of the directoryListing to send back on the data connection for this command.
     * The passed-in value is trimmed automatically.
     *
     * @param directoryListing - the directoryListing to set
     */
    public void setDirectoryListing(String directoryListing) {
        this.directoryListing = directoryListing.trim();
    }

}
