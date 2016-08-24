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
import org.mockftpserver.fake.filesystem.FileEntry;
import org.mockftpserver.fake.filesystem.FileSystemException;

import java.io.IOException;
import java.io.OutputStream;

/**
 * Abstract superclass for CommandHandlers that that store a file (STOR, STOU, APPE). Handler logic:
 * <ol>
 * <li>If the user has not logged in, then reply with 530 and terminate</li>
 * <li>If the pathname parameter is required but missing, then reply with 501 and terminate</li>
 * <li>If the required pathname parameter does not specify a valid filename, then reply with 553 and terminate</li>
 * <li>If the current user does not have write access to the named file, if it already exists, or else to its
 * parent directory, then reply with 553 and terminate</li>
 * <li>If the current user does not have execute access to the parent directory, then reply with 553 and terminate</li>
 * <li>Send an initial reply of 150</li>
 * <li>Read all available bytes from the data connection and store/append to the named file in the server file system</li>
 * <li>If file write/store fails, then reply with 553 and terminate</li>
 * <li>Send a final reply with 226</li>
 * </ol>
 *
 * @author Chris Mair
 * @version $Revision$ - $Date$
 */
public abstract class AbstractStoreFileCommandHandler extends AbstractFakeCommandHandler {

    protected void handle(Command command, Session session) {
        verifyLoggedIn(session);
        this.replyCodeForFileSystemException = ReplyCodes.WRITE_FILE_ERROR;

        String filename = getOutputFile(command);
        String path = getRealPath(session, filename);
        verifyFileSystemCondition(!getFileSystem().isDirectory(path), path, "filesystem.isDirectory");
        String parentPath = getFileSystem().getParent(path);
        verifyFileSystemCondition(getFileSystem().isDirectory(parentPath), parentPath, "filesystem.isNotADirectory");

        // User must have write permission to the file, if an existing file, or else to the directory if a new file
        String pathMustBeWritable = getFileSystem().exists(path) ? path : parentPath;
        verifyWritePermission(session, pathMustBeWritable);

        // User must have execute permission to the parent directory
        verifyExecutePermission(session, parentPath);

        sendReply(session, ReplyCodes.TRANSFER_DATA_INITIAL_OK);

        session.openDataConnection();
        byte[] contents = session.readData();
        session.closeDataConnection();

        FileEntry file = (FileEntry) getFileSystem().getEntry(path);
        if (file == null) {
            file = new FileEntry(path);
            getFileSystem().add(file);
        }
        file.setPermissions(getUserAccount(session).getDefaultPermissionsForNewFile());

        if (contents != null && contents.length > 0) {
            OutputStream out = file.createOutputStream(appendToOutputFile());
            try {
                out.write(contents);
            }
            catch (IOException e) {
                LOG.error("Error writing to file [" + file.getPath() + "]", e);
                throw new FileSystemException(file.getPath(), null, e);
            }
            finally {
                try {
                    out.close();
                } catch (IOException e) {
                    LOG.error("Error closing OutputStream for file [" + file.getPath() + "]", e);
                }
            }
        }
        sendReply(session, ReplyCodes.TRANSFER_DATA_FINAL_OK, getMessageKey(), list(filename));
    }

    /**
     * Return the path (absolute or relative) for the output file. The default behavior is to return
     * the required first parameter for the specified Command. Subclasses may override the default behavior.
     *
     * @param command - the Command
     * @return the output file name
     */
    protected String getOutputFile(Command command) {
        return command.getRequiredParameter(0);
    }

    /**
     * @return true if this command should append the transferred contents to the output file; false means
     *         overwrite an existing file. This default implentation returns false.
     */
    protected boolean appendToOutputFile() {
        return false;
    }

    /**
     * @return the message key for the reply message sent with the final (226) reply
     */
    protected abstract String getMessageKey();

}