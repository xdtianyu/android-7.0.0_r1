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

import org.mockftpserver.core.MockFtpServerException;
import org.mockftpserver.core.command.Command;
import org.mockftpserver.core.command.CommandHandler;
import org.mockftpserver.core.command.InvocationRecord;
import org.mockftpserver.core.session.Session;
import org.mockftpserver.core.util.Assert;
import org.mockftpserver.core.util.AssertFailedException;

import java.io.IOException;
import java.io.InputStream;

/**
 * CommandHandler for the RETR command. Returns the contents of the specified file on the
 * data connection, along with two replies on the control connection: a reply code of 150 and
 * another of 226.
 * <p/>
 * The <code>file</code> property specifies the pathname for the file whose contents should
 * be returned from this command. The file path is relative to the CLASSPATH (using the
 * ClassLoader for this class).
 * <p/>
 * An exception is thrown if the <code>file</code> property has not been set or if the specified
 * file does not exist or cannot be read.
 * <p/>
 * Each invocation record stored by this CommandHandler includes the following data element key/values:
 * <ul>
 * <li>{@link #PATHNAME_KEY} ("pathname") - the pathname of the file submitted on the invocation (the first command parameter)
 * </ul>
 *
 * @author Chris Mair
 * @version $Revision$ - $Date$
 */
public class FileRetrCommandHandler extends AbstractStubDataCommandHandler implements CommandHandler {

    public static final String PATHNAME_KEY = "pathname";
    static final int BUFFER_SIZE = 512;     // package-private for testing

    private String file;

    /**
     * Create new uninitialized instance
     */
    public FileRetrCommandHandler() {
    }

    /**
     * Create new instance using the specified file pathname
     *
     * @param file - the path to the file
     * @throws AssertFailedException - if the file is null
     */
    public FileRetrCommandHandler(String file) {
        setFile(file);
    }

    /**
     * @see org.mockftpserver.stub.command.AbstractStubDataCommandHandler#beforeProcessData(org.mockftpserver.core.command.Command, org.mockftpserver.core.session.Session, org.mockftpserver.core.command.InvocationRecord)
     */
    protected void beforeProcessData(Command command, Session session, InvocationRecord invocationRecord) throws Exception {
        Assert.notNull(file, "file");
        invocationRecord.set(PATHNAME_KEY, command.getRequiredParameter(0));
    }

    /**
     * @see org.mockftpserver.stub.command.AbstractStubDataCommandHandler#processData(org.mockftpserver.core.command.Command, org.mockftpserver.core.session.Session, org.mockftpserver.core.command.InvocationRecord)
     */
    protected void processData(Command command, Session session, InvocationRecord invocationRecord) {
        InputStream inputStream = getClass().getClassLoader().getResourceAsStream(file);
        Assert.notNull(inputStream, "InputStream for [" + file + "]");
        byte[] buffer = new byte[BUFFER_SIZE];
        try {
            int numBytes;
            while ((numBytes = inputStream.read(buffer)) != -1) {
                LOG.trace("Sending " + numBytes + " bytes...");
                session.sendData(buffer, numBytes);
            }
        }
        catch (IOException e) {
            throw new MockFtpServerException(e);
        }
    }

    /**
     * Set the path of the file whose contents should be returned when this command is
     * invoked. The path is relative to the CLASSPATH.
     *
     * @param file - the path to the file
     * @throws AssertFailedException - if the file is null
     */
    public void setFile(String file) {
        Assert.notNull(file, "file");
        this.file = file;
    }

}
