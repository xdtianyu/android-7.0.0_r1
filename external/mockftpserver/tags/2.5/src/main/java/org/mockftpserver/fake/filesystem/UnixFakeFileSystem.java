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

import org.mockftpserver.core.util.Assert;

/**
 * Implementation of the {@link FileSystem} interface that simulates a Unix
 * file system. The rules for file and directory names include:
 * <ul>
 * <li>Filenames are case-sensitive</li>
 * <li>Forward slashes (/) are the only valid path separators</li>
 * </ul>
 * <p/>
 * The <code>directoryListingFormatter</code> property is automatically initialized to an instance
 * of {@link UnixDirectoryListingFormatter}.
 *
 * @author Chris Mair
 * @version $Revision$ - $Date$
 */
public class UnixFakeFileSystem extends AbstractFakeFileSystem {

    public static final char SEPARATOR = '/';

    /**
     * Construct a new instance and initialize the directoryListingFormatter to a UnixDirectoryListingFormatter.
     */
    public UnixFakeFileSystem() {
        this.setDirectoryListingFormatter(new UnixDirectoryListingFormatter());
    }

    //-------------------------------------------------------------------------
    // Abstract Method Implementations
    //-------------------------------------------------------------------------

    protected char getSeparatorChar() {
        return SEPARATOR;
    }

    /**
     * Return true if the specified path designates a valid (absolute) file path. For Unix,
     * a path is valid if it starts with the '/' character, followed by zero or more names
     * (a sequence of any characters except '/'), delimited by '/'. The path may optionally
     * contain a terminating '/'.
     *
     * @param path - the path
     * @return true if path is valid, false otherwise
     * @throws AssertionError - if path is null
     */
    protected boolean isValidName(String path) {
        Assert.notNull(path, "path");
        // Any character but '/'
        return path.matches("\\/|(\\/[^\\/]+\\/?)+");

    }

    /**
     * Return true if the specified char is a separator character ('\' or '/')
     *
     * @param c - the character to test
     * @return true if the specified char is a separator character ('\' or '/')
     */
    protected boolean isSeparator(char c) {
        return c == SEPARATOR;
    }

    /**
     * @return true if the specified path component is a root for this filesystem
     */
    protected boolean isRoot(String pathComponent) {
        return pathComponent.indexOf(":") != -1;
    }

}