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

/**
 * Exception thrown when a path/filename is not valid. Causes include:
 * <ul>
 * <li>The filename contains invalid characters</li>
 * <li>The path specifies a new filename, but its parent directory does not exist</li>
 * <li>The path is expected to be a file, but actually specifies an existing directory</li>
 * </ul>
 */
public class InvalidFilenameException extends FileSystemException {

    private static final String MESSAGE_KEY = "filesystem.pathIsNotValid";

    /**
     * @param path - the path involved in the file system operation that caused the exception
     */
    public InvalidFilenameException(String path) {
        super(path, MESSAGE_KEY);
    }

    /**
     * @param path  - the path involved in the file system operation that caused the exception
     * @param cause - the exception cause, wrapped by this exception
     */
    public InvalidFilenameException(String path, Throwable cause) {
        super(path, MESSAGE_KEY, cause);
    }

}