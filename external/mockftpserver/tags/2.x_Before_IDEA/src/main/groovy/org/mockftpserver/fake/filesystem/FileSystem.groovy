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

package org.mockftpserver.fake.filesystem

import java.io.File
import java.io.InputStream
import java.io.OutputStream
import java.util.List

/**
 * Interface for a file system for managing files and directories.
 * 
 * @version $Revision: $ - $Date: $
 *
 * @author Chris Mair
 */
public interface FileSystem {

    /**
     * Creates an empty file with the specified pathname.
     * 
     * @param path - the path of the filename to create
     * @return true if and only if the file was created false otherwise
     * 
     * @throws AssertionError - if path is null
     * @throws FileSystemException - if an I/O error occurs
     */
    public boolean createFile(String path)

    /**
     * Creates the directory named by the specified pathname.
     * 
     * @param path - the path of the directory to create
     * @return true if and only if the directory was created false otherwise
     * 
     * @throws AssertionError - if path is null
     */
    public boolean createDirectory(String path)

    /**
     * Create and return a new OutputStream for writing to the file at the specified path
     * @param path - the path of the file
     * @param append - true if the OutputStream should append to the end of the file if the file already exists
     * 
     * @throws AssertionError - if path is null
     * @throws FileSystemException - wraps a FileNotFoundException if thrown
     */
    public OutputStream createOutputStream(String path, boolean append)

    /**
     * Create and return a new InputStream for reading from the file at the specified path
     * @param path - the path of the file
     * 
     * @throws AssertionError - if path is null
     * @throws FileSystemException - wraps a FileNotFoundException if thrown
     */
    public InputStream createInputStream(String path)

    /**
     * Return the List of FileInfo objects for the files in the specified directory path. If the
     * path does not refer to a valid directory, then an empty List is returned.
     * 
     * @param path - the path of the directory whose contents should be returned
     * @return the List of FileInfo objects for all files in the specified directory may be empty
     */
    public List listFiles(String path)

    /**
     * Return the List of filenames in the specified directory path. The returned filenames do not
     * include a path. If the path does not refer to a valid directory, then an empty List is
     * returned.
     * 
     * @param path - the path of the directory whose contents should be returned
     * @return the List of filenames (not including paths) for all files in the specified directory
     *         may be empty
     * 
     * @throws AssertionError - if path is null
     */
    public List listNames(String path)

    /**
     * Delete the file or directory specified by the path. Return true if the file is successfully
     * deleted, false otherwise. If the path refers to a directory, it must be empty. Return false
     * if the path does not refer to a valid file or directory or if it is a non-empty directory.
     * 
     * @param path - the path of the file or directory to delete
     * @return true if the file or directory is successfully deleted
     * 
     * @throws AssertionError - if path is null
     */
    public boolean delete(String path)

    /**
     * Rename the file or directory. Specify the FROM path and the TO path. Return true if the file
     * is successfully renamed, false otherwise. Return false if the path does not refer to a valid
     * file or directory.
     * 
     * @param path - the path of the file or directory to delete
     * @param fromPath - the source (old) path + filename
     * @param toPath - the target (new) path + filename
     * @return true if the file or directory is successfully renamed
     * 
     * @throws AssertionError - if fromPath or toPath is null
     */
    public boolean rename(String fromPath, String toPath)

    //-------------------------------------------------------------------------
    // Path-related Methods
    //-------------------------------------------------------------------------

    /**
     * Return true if there exists a file or directory at the specified path
     * 
     * @param path - the path
     * @return true if the file/directory exists
     * 
     * @throws AssertionError - if path is null
     */
    public boolean exists(String path)

    /**
     * Return true if the specified path designates an existing directory, false otherwise
     * 
     * @param path - the path
     * @return true if path is a directory, false otherwise
     * 
     * @throws AssertionError - if path is null
     */
    public boolean isDirectory(String path)

    /**
     * Return true if the specified path designates an existing file, false otherwise
     * 
     * @param path - the path
     * @return true if path is a file, false otherwise
     * 
     * @throws AssertionError - if path is null
     */
    public boolean isFile(String path)

     /**
      * Return true if the specified path designates an absolute file path. What
      * constitutes an absolute path is dependent on the file system implementation.
      * 
      * @param path - the path
      * @return true if path is absolute, false otherwise
      * 
      * @throws AssertionError - if path is null
      */
     public boolean isAbsolute(String path)

    /**
     * Build a path from the two path components. Concatenate path1 and path2. Insert the file system-dependent
     * separator character in between if necessary (i.e., if both are non-empty and path1 does not already
     * end with a separator character AND path2 does not begin with one).
     * 
     * @param path1 - the first path component may be null or empty
     * @param path2 - the second path component may be null or empty
     * @return the path resulting from concatenating path1 to path2
     */
    public String path(String path1, String path2)

    /**
     * Returns the name of the file or directory denoted by this abstract
     * pathname.  This is just the last name in the pathname's name
     * sequence.  If the pathname's name sequence is empty, then the empty
     * string is returned.
     *
     * @return  The name of the file or directory denoted by this abstract pathname, or the 
     *          empty string if this pathname's name sequence is empty
     *          
     * @see File#getName()         
     */
    public String getName(String path)

    /**
     * Return the parent path of the specified path. If <code>path</code> specifies a filename,
     * then this method returns the path of the directory containing that file. If <code>path</code>
     * specifies a directory, the this method returns its parent directory. If <code>path</code> is
     * empty or does not have a parent component, then return an empty string.
     * <p>
     * All path separators in the returned path are converted to the system-dependent separator character.
     * @param path - the path
     * @return the parent of the specified path, or null if <code>path</code> has no parent
     * 
     * @throws AssertionError - if path is null
     */
    public String getParent(String path)

    /**
     * Return the standard, normalized form of the path. 
     * @param path
     * @return
     * 
     * @throws AssertionError - if path is null
     * @throws FileSystemException - if an IOException occurs while determining the canonical path from the real" file system.
     */
    public String normalize(String path)

}