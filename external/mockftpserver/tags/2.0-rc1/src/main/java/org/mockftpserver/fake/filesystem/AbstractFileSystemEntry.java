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

import java.util.Date;

/**
 * The abstract superclass for concrete file system entry classes representing files and directories.
 *
 * @author Chris Mair
 * @version $Revision: 147 $ - $Date: 2008-11-02 19:10:37 -0500 (Sun, 02 Nov 2008) $
 */
public abstract class AbstractFileSystemEntry implements FileSystemEntry {

    private String path;
    private boolean pathLocked = false;

    private Date lastModified;
    private String owner;
    private String group;

    public Date getLastModified() {
        return lastModified;
    }

    public void setLastModified(Date lastModified) {
        this.lastModified = lastModified;
    }

    public String getOwner() {
        return owner;
    }

    public void setOwner(String owner) {
        this.owner = owner;
    }

    public String getGroup() {
        return group;
    }

    public void setGroup(String group) {
        this.group = group;
    }

    public Permissions getPermissions() {
        return permissions;
    }

    public void setPermissions(Permissions permissions) {
        this.permissions = permissions;
    }

    private Permissions permissions;

    /**
     * Construct a new instance without setting its path
     */
    public AbstractFileSystemEntry() {
    }

    /**
     * Construct a new instance with the specified value for its path
     *
     * @param path - the value for path
     */
    public AbstractFileSystemEntry(String path) {
        this.path = path;
    }

    /**
     * @return the path for this entry
     */
    public String getPath() {
        return path;
    }

    /**
     * @return the file name or directory name (no path) for this entry
     */
    public String getName() {
        int separatorIndex1 = path.lastIndexOf('/');
        int separatorIndex2 = path.lastIndexOf('\\');
//        int separatorIndex = [separatorIndex1, separatorIndex2].max();
        int separatorIndex = separatorIndex1 > separatorIndex2 ? separatorIndex1 : separatorIndex2;
        return (separatorIndex == -1) ? path : path.substring(separatorIndex + 1);
    }

    /**
     * Set the path for this entry. Throw an exception if pathLocked is true.
     *
     * @param path - the new path value
     */
    public void setPath(String path) {
        Assert.isFalse(pathLocked, "path is locked");
        this.path = path;
    }

    public void lockPath() {
        this.pathLocked = true;
    }

    public void setPermissionsFromString(String permissionsString) {
        this.permissions = new Permissions(permissionsString);
    }

    /**
     * Abstract method -- must be implemented within concrete subclasses
     *
     * @return true if this file system entry represents a directory
     */
    public abstract boolean isDirectory();

}
