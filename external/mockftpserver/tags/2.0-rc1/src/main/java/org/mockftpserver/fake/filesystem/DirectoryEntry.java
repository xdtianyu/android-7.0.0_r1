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
 * File system entry representing a directory
 *
 * @author Chris Mair
 * @version $Revision: 123 $ - $Date: 2008-09-24 22:23:19 -0400 (Wed, 24 Sep 2008) $
 */
public class DirectoryEntry extends AbstractFileSystemEntry {

    /**
     * Construct a new instance without setting its path
     */
    public DirectoryEntry() {
    }

    /**
     * Construct a new instance with the specified value for its path
     *
     * @param path - the value for path
     */
    public DirectoryEntry(String path) {
        super(path);
    }

    /**
     * Abstract method -- must be implemented within concrete subclasses
     *
     * @return true if this file system entry represents a directory
     */
    public boolean isDirectory() {
        return true;
    }

    /**
     * Return the size of this directory. This method returns zero.
     *
     * @return the file size in bytes
     */
    public long getSize() {
        return 0;
    }

    /**
     * @see java.lang.Object#toString()
     */
    public String toString() {
        return "Directory['" + getPath() + "' lastModified=" + getLastModified() + "  owner=" + getOwner() +
                "  group=" + getGroup() + "  permissions=" + getPermissions() + "]";
    }

    /**
     * Return a new FileSystemEntry that is a clone of this object, except having the specified path
     *
     * @param path - the new path value for the cloned file system entry
     * @return a new FileSystemEntry that has all the same values as this object except for its path
     */
    public FileSystemEntry cloneWithNewPath(String path) {
        DirectoryEntry clone = new DirectoryEntry(path);
        clone.setLastModified(getLastModified());
        clone.setOwner(getOwner());
        clone.setGroup(getGroup());
        clone.setPermissions(getPermissions());
        return clone;
    }

}
