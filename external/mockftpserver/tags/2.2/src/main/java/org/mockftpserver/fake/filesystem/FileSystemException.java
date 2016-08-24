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
package org.mockftpserver.fake.filesystem;

import org.mockftpserver.core.MockFtpServerException;

/**
 * Represents an error that occurs while performing a FileSystem operation.
 *
 * @author Chris Mair
 * @version $Revision$ - $Date$
 */
public class FileSystemException extends MockFtpServerException {

    /**
     * The path involved in the file system operation that caused the exception
     */
    private String path;

    /**
     * The message key for the exception message
     */
    private String messageKey;

    /**
     * Construct a new instance for the specified path and message key
     *
     * @param path       - the path involved in the file system operation that caused the exception
     * @param messageKey - the exception message key
     */
    public FileSystemException(String path, String messageKey) {
        super(path);
        this.path = path;
        this.messageKey = messageKey;
    }

    /**
     * @param path       - the path involved in the file system operation that caused the exception
     * @param messageKey - the exception message key
     * @param cause      - the exception cause, wrapped by this exception
     */
    public FileSystemException(String path, String messageKey, Throwable cause) {
        super(path, cause);
        this.path = path;
        this.messageKey = messageKey;
    }

    public String getPath() {
        return path;
    }

    public void setPath(String path) {
        this.path = path;
    }

    public String getMessageKey() {
        return messageKey;
    }

    public void setMessageKey(String messageKey) {
        this.messageKey = messageKey;
    }

}
