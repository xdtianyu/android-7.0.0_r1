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

import java.util.Date;

/**
 * Interface for an entry within a fake file system, representing a single file or directory.
 *
 * @author Chris Mair
 * @version $Revision: 160 $ - $Date: 2008-11-15 08:46:23 -0500 (Sat, 15 Nov 2008) $
 */
public interface FileSystemEntry {

    /**
     * Return true if this entry represents a directory, false otherwise
     *
     * @return true if this file system entry is a directory, false otherwise
     */
    public boolean isDirectory();

    /**
     * Return the path for this file system entry
     *
     * @return the path for this file system entry
     */
    public String getPath();

    /**
     * Return the file name or directory name (no path) for this entry
     *
     * @return the file name or directory name (no path) for this entry
     */
    public String getName();

    /**
     * Return the size of this file system entry
     *
     * @return the file size in bytes
     */
    public long getSize();

    /**
     * Return the timestamp Date for the last modification of this file system entry
     *
     * @return the last modified timestamp Date for this file system entry
     */
    public Date getLastModified();

    /**
     * Set the timestamp Date for the last modification of this file system entry
     *
     * @param lastModified - the lastModified value, as a Date
     */
    public void setLastModified(Date lastModified);

    /**
     * @return the username of the owner of this file system entry
     */
    public String getOwner();

    /**
     * @return the name of the owning group for this file system entry
     */
    public String getGroup();

    /**
     * @return the Permissions for this file system entry
     */
    public Permissions getPermissions();

    /**
     * Return a new FileSystemEntry that is a clone of this object, except having the specified path
     *
     * @param path - the new path value for the cloned file system entry
     * @return a new FileSystemEntry that has all the same values as this object except for its path
     */
    public FileSystemEntry cloneWithNewPath(String path);

    /**
     * Lock down the path so it cannot be changed
     */
    public void lockPath();

}