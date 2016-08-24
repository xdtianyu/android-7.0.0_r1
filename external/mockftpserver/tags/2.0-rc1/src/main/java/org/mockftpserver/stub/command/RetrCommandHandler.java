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

import org.apache.log4j.Logger;
import org.mockftpserver.core.command.Command;
import org.mockftpserver.core.command.CommandHandler;
import org.mockftpserver.core.command.InvocationRecord;
import org.mockftpserver.core.session.Session;
import org.mockftpserver.core.util.Assert;

/**
 * CommandHandler for the RETR (Retrieve) command. Return the configured file contents on the data
 * connection, along with two replies on the control connection: a reply code of 150 and
 * another of 226. By default, return an empty file (i.e., a zero-length byte[]). You can
 * customize the returned file contents by setting the <code>fileContents</code> property,
 * specified either as a String or as a byte array.
 * <p/>
 * Each invocation record stored by this CommandHandler includes the following data element key/values:
 * <ul>
 * <li>{@link #PATHNAME_KEY} ("pathname") - the pathname of the file submitted on the invocation (the first command parameter)
 * </ul>
 *
 * @author Chris Mair
 * @version $Revision$ - $Date$
 */
public final class RetrCommandHandler extends AbstractStubDataCommandHandler implements CommandHandler {

    private static final Logger LOG = Logger.getLogger(RetrCommandHandler.class);
    public static final String PATHNAME_KEY = "pathname";

    private byte[] fileContents = new byte[0];

    /**
     * Create new uninitialized instance
     */
    public RetrCommandHandler() {
    }

    /**
     * Create new instance using the specified fileContents
     *
     * @param fileContents - the file contents
     * @throws org.mockftpserver.core.util.AssertFailedException
     *          - if the fileContents is null
     */
    public RetrCommandHandler(String fileContents) {
        setFileContents(fileContents);
    }

    /**
     * Create new instance using the specified fileContents
     *
     * @param fileContents - the file contents
     * @throws org.mockftpserver.core.util.AssertFailedException
     *          - if the fileContents is null
     */
    public RetrCommandHandler(byte[] fileContents) {
        setFileContents(fileContents);
    }

    /**
     * @see org.mockftpserver.stub.command.AbstractStubDataCommandHandler#beforeProcessData(org.mockftpserver.core.command.Command, org.mockftpserver.core.session.Session, org.mockftpserver.core.command.InvocationRecord)
     */
    protected void beforeProcessData(Command command, Session session, InvocationRecord invocationRecord) throws Exception {
        String filename = command.getRequiredParameter(0);
        invocationRecord.set(PATHNAME_KEY, filename);
    }

    /**
     * @see org.mockftpserver.stub.command.AbstractStubDataCommandHandler#processData(org.mockftpserver.core.command.Command, org.mockftpserver.core.session.Session, org.mockftpserver.core.command.InvocationRecord)
     */
    protected void processData(Command command, Session session, InvocationRecord invocationRecord) {
        LOG.info("Sending " + fileContents.length + " bytes");
        session.sendData(fileContents, fileContents.length);
    }

    /**
     * Set the file contents to return from subsequent command invocations
     *
     * @param fileContents - the fileContents to set
     * @throws org.mockftpserver.core.util.AssertFailedException
     *          - if the fileContents is null
     */
    public void setFileContents(String fileContents) {
        Assert.notNull(fileContents, "fileContents");
        setFileContents(fileContents.getBytes());
    }

    /**
     * Set the file contents to return from subsequent command invocations
     *
     * @param fileContents - the file contents
     * @throws org.mockftpserver.core.util.AssertFailedException
     *          - if the fileContents is null
     */
    public void setFileContents(byte[] fileContents) {
        Assert.notNull(fileContents, "fileContents");
        this.fileContents = fileContents;
    }

}
