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

import java.util.List;

/**
 * Interface for a file system for managing files and directories.
 *
 * @author Chris Mair
 * @version $Revision: 129 $ - $Date: 2008-10-19 06:27:17 -0400 (Sun, 19 Oct 2008) $
 */
public interface FileSystem {

    /**
     * Add the specified file system entry (file or directory) to this file system
     *
     * @param entry - the FileSystemEntry to add
     */
    public void add(FileSystemEntry entry);

    /**
     * Return the List of FileSystemEntry objects for the files in the specified directory path. If the
     * path does not refer to a valid directory, then an empty List is returned.
     *
     * @param path - the path of the directory whose contents should be returned
     * @return the List of FileSystemEntry objects for all files in the specified directory may be empty
     */
    public List listFiles(String path);

    /**
     * Return the List of filenames in the specified directory path. The returned filenames do not
     * include a path. If the path does not refer to a valid directory, then an empty List is
     * returned.
     *
     * @param path - the path of the directory whose contents should be returned
     * @return the List of filenames (not including paths) for all files in the specified directory
     *         may be empty
     * @throws AssertionError - if path is null
     */
    public List listNames(String path);

    /**
     * Delete the file or directory specified by the path. Return true if the file is successfully
     * deleted, false otherwise. If the path refers to a directory, it must be empty. Return false
     * if the path does not refer to a valid file or directory or if it is a non-empty directory.
     *
     * @param path - the path of the file or directory to delete
     * @return true if the file or directory is successfully deleted
     * @throws AssertionError - if path is null
     */
    public boolean delete(String path);

    /**
     * Rename the file or directory. Specify the FROM path and the TO path. Throw an exception if the FROM path or
     * the parent directory of the TO path do not exist; or if the rename fails for another reason.
     *
     * @param fromPath - the source (old) path + filename
     * @param toPath   - the target (new) path + filename
     * @throws AssertionError      - if fromPath or toPath is null
     * @throws FileSystemException - if the rename fails.
     */
    public void rename(String fromPath, String toPath);

    /**
     * Return the formatted directory listing entry for the file represented by the specified FileSystemEntry
     *
     * @param fileSystemEntry - the FileSystemEntry representing the file or directory entry to be formatted
     * @return the the formatted directory listing entry
     */
    public String formatDirectoryListing(FileSystemEntry fileSystemEntry);

    //-------------------------------------------------------------------------
    // Path-related Methods
    //-------------------------------------------------------------------------

    /**
     * Return true if there exists a file or directory at the specified path
     *
     * @param path - the path
     * @return true if the file/directory exists
     * @throws AssertionError - if path is null
     */
    public boolean exists(String path);

    /**
     * Return true if the specified path designates an existing directory, false otherwise
     *
     * @param path - the path
     * @return true if path is a directory, false otherwise
     * @throws AssertionError - if path is null
     */
    public boolean isDirectory(String path);

    /**
     * Return true if the specified path designates an existing file, false otherwise
     *
     * @param path - the path
     * @return true if path is a file, false otherwise
     * @throws AssertionError - if path is null
     */
    public boolean isFile(String path);

    /**
     * Return true if the specified path designates an absolute file path. What
     * constitutes an absolute path is dependent on the file system implementation.
     *
     * @param path - the path
     * @return true if path is absolute, false otherwise
     * @throws AssertionError - if path is null
     */
    public boolean isAbsolute(String path);

    /**
     * Build a path from the two path components. Concatenate path1 and path2. Insert the file system-dependent
     * separator character in between if necessary (i.e., if both are non-empty and path1 does not already
     * end with a separator character AND path2 does not begin with one).
     *
     * @param path1 - the first path component may be null or empty
     * @param path2 - the second path component may be null or empty
     * @return the path resulting from concatenating path1 to path2
     */
    public String path(String path1, String path2);

    /**
     * Returns the FileSystemEntry object representing the file system entry at the specified path, or null
     * if the path does not specify an existing file or directory within this file system.
     *
     * @param path - the path of the file or directory within this file system
     * @return the FileSystemEntry containing the information for the file or directory, or else null
     */
    public FileSystemEntry getEntry(String path);

    /**
     * Return the parent path of the specified path. If <code>path</code> specifies a filename,
     * then this method returns the path of the directory containing that file. If <code>path</code>
     * specifies a directory, the this method returns its parent directory. If <code>path</code> is
     * empty or does not have a parent component, then return an empty string.
     * <p/>
     * All path separators in the returned path are converted to the system-dependent separator character.
     *
     * @param path - the path
     * @return the parent of the specified path, or null if <code>path</code> has no parent
     * @throws AssertionError - if path is null
     */
    public String getParent(String path);

}