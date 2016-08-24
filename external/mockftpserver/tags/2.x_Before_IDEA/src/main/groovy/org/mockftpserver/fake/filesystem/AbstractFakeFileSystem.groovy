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

import java.io.InputStream
import java.io.OutputStream
import java.util.ArrayList
import java.util.Collections
import java.util.HashMap
import java.util.Iterator
import java.util.List
import java.util.Map
import org.apache.log4j.Logger

/**
 * Abstract superclass for implementation of the FileSystem interface that manage the files 
 * and directories in memory, simulating a real file system.
 * 
 * @version $Revision: $ - $Date: $
 * 
 * @author Chris Mair
 */
abstract class AbstractFakeFileSystem implements FileSystem {

     private static final LOG = Logger.getLogger(AbstractFakeFileSystem)

     /**
      * If <code>true</code>, creating a directory or file will automatically create 
      * any parent directories (recursively) that do not already exist. If <code>false</code>, 
      * then creating a directory or file throws an exception if its parent directory 
      * does not exist. This value defaults to <code>false</code>.
      */
     boolean createParentDirectoriesAutomatically = false
     
     private Map entries = new HashMap()

     /**
      * Add the specified file system entry (file or directory) to this file system
      * 
      * @param entry - the subclass of AbstractFileSystemEntry to add
      */
     public void addEntry(AbstractFileSystemEntry entry) {
         def normalized = normalize(entry.path)
         if (entries.get(normalized) != null) {
             throw new FileSystemException("The path [" + normalized + "] already exists")
         }

         // Make sure parent directory exists, if there is a parent
         String parent = getParent(normalized)
         if (parent != null) {
             verifyPathExists(parent)
         }

         entries.put(normalized, entry)
     }

     /**
      * Creates an empty file with the specified pathname.
      * 
      * @param path - the path of the filename to create
      * @return true if and only if the file was created false otherwise
      * 
      * @throws AssertionError - if path is null
      * @throws FileSystemException - if an I/O error occurs
      */
     public boolean createFile(String path) {
         assert path != null
         checkForInvalidFilename(path)

         // TODO Consider refactoring into adEntry()
         if (!parentDirectoryExists(path)) {
             if (createParentDirectoriesAutomatically) {
                 String parent = getParent(path)
                 if (!createDirectory(parent)) {
                     return false
                 }
             }
             else {
                 throw new FileSystemException("Parent directory does not exist: " + getParent(path))
             }
         }
         
         if (exists(path)) {
             return false
         }
         String normalizedPath = normalize(path)
         addEntry(new FileEntry(normalizedPath))
         return true
     }

     /**
      * Creates the directory named by the specified pathname.
      * 
      * @param path - the path of the directory to create
      * @return true if and only if the directory was created false otherwise
      * 
      * @throws AssertionError - if path is null
      * 
      * @see org.mockftpserver.fake.filesystem.FileSystem#createDirectory(java.lang.String)
      */
     public boolean createDirectory(String path) {
         assert path != null
         String normalizedPath = normalize(path)
         
         if (!parentDirectoryExists(path)) {
             String parent = getParent(path)
             if (createParentDirectoriesAutomatically) {
                 if (!createDirectory(parent)) {
                     return false
                 }
             }
             else {
                 return false
             }
         }
         try {
             addEntry(new DirectoryEntry(normalizedPath))
             return true
         }
         catch (FileSystemException e) {
             return false
         }
     }

     /**
      * Create and return a new InputStream for reading from the file at the specified path
      * @param path - the path of the file
      * 
      * @throws AssertionError - if path is null
      * @throws FileSystemException - wraps a FileNotFoundException if thrown
      * 
      * @see org.mockftpserver.fake.filesystem.FileSystem#createInputStream(java.lang.String)
      */
     public InputStream createInputStream(String path) {
         verifyPathExists(path)
         verifyIsDirectory(path, false)
         FileEntry fileEntry = (FileEntry) getEntry(path)
         return fileEntry.createInputStream()
     }

     /**
      * Create and return a new OutputStream for writing to the file at the specified path
      * @param path - the path of the file
      * @param append - true if the OutputStream should append to the end of the file if the file already exists
      * 
      * @throws AssertionError - if path is null
      * @throws FileSystemException - wraps a FileNotFoundException if thrown
      * 
      * @see org.mockftpserver.fake.filesystem.FileSystem#createOutputStream(java.lang.String,boolean)
      */
     public OutputStream createOutputStream(String path, boolean append) {
         checkForInvalidFilename(path)
         verifyParentDirectoryExists(path)
         if (exists(path)) {
             verifyIsDirectory(path, false)
         }
         else {
             addEntry(new FileEntry(path))
         }
         FileEntry fileEntry = (FileEntry) getEntry(path)
         return fileEntry.createOutputStream(append)
     }

     /**
      * Delete the file or directory specified by the path. Return true if the file is successfully
      * deleted, false otherwise. If the path refers to a directory, it must be empty. Return false
      * if the path does not refer to a valid file or directory or if it is a non-empty directory.
      * 
      * @param path - the path of the file or directory to delete
      * @return true if the file or directory is successfully deleted
      * 
      * @throws AssertionError - if path is null
      * 
      * @see org.mockftpserver.fake.filesystem.FileSystem#delete(java.lang.String)
      */
     public boolean delete(String path) {
         assert path != null
         String key = normalize(path)
         AbstractFileSystemEntry entry = getEntry(key)

         if (entry != null && !hasChildren(path)) {
             entries.remove(key)
             return true
         }
         return false
     }

     /**
      * Return true if there exists a file or directory at the specified path
      * 
      * @param path - the path
      * @return true if the file/directory exists
      * 
      * @throws AssertionError - if path is null
      * 
      * @see org.mockftpserver.fake.filesystem.FileSystem#exists(java.lang.String)
      */
     public boolean exists(String path) {
         assert path != null
         return getEntry(path) != null
     }

     /**
      * Return true if the specified path designates an existing directory, false otherwise
      * 
      * @param path - the path
      * @return true if path is a directory, false otherwise
      * 
      * @throws AssertionError - if path is null
      * 
      * @see org.mockftpserver.fake.filesystem.FileSystem#isDirectory(java.lang.String)
      */
     public boolean isDirectory(String path) {
         assert path != null
         AbstractFileSystemEntry entry = getEntry(path)
         return entry != null && entry.isDirectory()
     }

     /**
      * Return true if the specified path designates an existing file, false otherwise
      * 
      * @param path - the path
      * @return true if path is a file, false otherwise
      * 
      * @throws AssertionError - if path is null
      * 
      * @see org.mockftpserver.fake.filesystem.FileSystem#isFile(java.lang.String)
      */
     public boolean isFile(String path) {
         assert path != null
         AbstractFileSystemEntry entry = getEntry(path)
         return entry != null && !entry.isDirectory()
     }

     /**
      * Return the List of FileInfo objects for the files in the specified directory 
      * or file path. If the path specifies a single file, then return a list with
      * a single FileInfo object representing that file. If the path does not refer 
      * to a valid or existing directory or file, then an empty List is returned.
      * 
      * @param path - the path of the directory or file whose information should be returned
      * @return the List of FileInfo objects for the specified directory or file; may be empty
      * 
      * @see org.mockftpserver.fake.filesystem.FileSystem#listFiles(java.lang.String)
      */
     public List listFiles(String path) {
         if (isFile(path)) {
             return [buildFileInfoForPath(path)]
         }
          
         String normalizedPath = normalize(path)
         List fileInfoList = []
         List children = children(path)
         if (children != null) {
             children.each { childPath ->
                 if (normalizedPath.equals(getParent(childPath))) {
                     def fileInfo = buildFileInfoForPath(childPath)
                     fileInfoList.add(fileInfo)
                 }
             }
         }
         return fileInfoList
     }
      
     /**
      * Build a FileInfo based on the file or directory specified by the path
      * @param path - the path for the file or directory
      */ 
     protected FileInfo buildFileInfoForPath(String path) {
         AbstractFileSystemEntry entry = getRequiredEntry(path)
         def name = getName(entry.getPath())
         def lastModified = entry.lastModified
         entry.isDirectory()  \
             ? FileInfo.forDirectory(name, entry.lastModified)  \
             : FileInfo.forFile(name, ((FileEntry)entry).getSize(), entry.lastModified)
     }

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
      * 
      * @see org.mockftpserver.fake.filesystem.FileSystem#listNames(java.lang.String)
      */
     public List listNames(String path) {
         List filenames = new ArrayList()
         List children = children(path)
         children.each { childPath ->
             filenames.add(getName(childPath))
         }
         return filenames
     }

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
     public boolean rename(String fromPath, String toPath) {
         assert toPath != null
         assert fromPath != null

         AbstractFileSystemEntry entry = getEntry(fromPath)
         
         if (entry != null) {
             String normalizedFromPath = normalize(fromPath)
             String normalizedToPath = normalize(toPath)
             
             // Create the TO directory entry first so that the destination path exists when you 
             // move the children. Remove the FROM path after all children have been moved 
             if (!createDirectory(normalizedToPath)) {
                 return false
             }

             List children = descendents(fromPath)
             children.each { childPath ->
                 AbstractFileSystemEntry child = getRequiredEntry(childPath)
                 String normalizedChildPath = normalize(child.getPath())
                 assert normalizedChildPath.startsWith(normalizedFromPath), "Starts with FROM path"
                 String childToPath = normalizedToPath + normalizedChildPath.substring(normalizedFromPath.length())
                 renamePath(child, childToPath)
             }
             entries.remove(normalizedFromPath)
             return true
         }
         return false
     }

     /**
      * @see java.lang.Object#toString()
      */
     public String toString() {
         return this.class.name + entries
     }
     
     //-------------------------------------------------------------------------
     // Abstract Methods
     //-------------------------------------------------------------------------

     /**
      * @return true if the specified dir/file path name is valid according to the current filesystem.
      */
     protected abstract boolean isValidName(String path)

     /**
      * @return the file system-specific file separator
      */
     protected abstract String getSeparator()

     //-------------------------------------------------------------------------
     
    /**
     * TODO Refactor with common code from DefaultFileSystem 
     * 
     * Build a path from the two path components. Concatenate path1 and path2. Insert the path
     * separator character in between if necessary (i.e., if both are non-empty and path1 does not already
     * end with a separator character AND path2 does not begin with one).
     * 
     * @param path1 - the first path component may be null or empty
     * @param path2 - the second path component may be null or empty
     * @return the path resulting from concatenating path1 to path2
     */
    String path(String path1, String path2) {
        StringBuffer buf = new StringBuffer()
        if (path1 != null && path1.length() > 0) {
            buf.append(path1)
        }
        if (path2 != null && path2.length() > 0) {
            if ((path1 != null && path1.length() > 0)
                && (!isSeparator(path1.charAt(path1.length()-1)))
                && (!isSeparator(path2.charAt(0)))) {
                buf.append(this.separator)
            }
            buf.append(path2)
        }
        return buf.toString()
    }

    /**
     * Return the standard, normalized form of the path. 
     * @param path - the path
     * @return the path in a standard, unique, canonical form
     * 
     * @throws AssertionError - if path is null
     */
    String normalize(String path) {
        return componentsToPath(normalizedComponents(path))
    }

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
    String getParent(String path) {
         def parts = normalizedComponents(path)
         if (parts.size() < 2) {
             return null
         }
         parts.remove(parts.size()-1)
         return componentsToPath(parts)
    }
    
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
    String getName(String path) {
        assert path != null
        def normalized = normalize(path)
        int separatorIndex = normalized.lastIndexOf(this.separator)
        return (separatorIndex == -1) ? normalized : normalized.substring(separatorIndex+1)
    }
     
     //-------------------------------------------------------------------------
     // Internal Helper Methods
     //-------------------------------------------------------------------------

     /**
      * Throw an InvalidFilenameException if the specified path is not valid.
      */
     protected void checkForInvalidFilename(String path) {
         if (!isValidName(path)) {
             throw new InvalidFilenameException(path);
         }
     }
     
     /**
      * Rename the file system entry to the specified path name
      * @param entry - the file system entry
      * @param toPath - the TO path (normalized)
      */
     protected void renamePath(AbstractFileSystemEntry entry, String toPath) {
         def normalizedFrom = normalize(entry.path)
         def normalizedTo = normalize(toPath)
         LOG.info("renaming from [" + normalizedFrom + "] to [" + normalizedTo + "]")
         entries.remove(normalizedFrom)
         entry.setPath(normalizedTo)
         addEntry(entry)
     }
     
     /**
      * Return the AbstractFileSystemEntry for the specified path, or else null
      * 
      * @param path - the path
      * @return the AbstractFileSystemEntry or null if no entry exists for that path
      */
     private AbstractFileSystemEntry getEntry(String path) {
         return (AbstractFileSystemEntry) entries.get(normalize(path))
     }

     /**
      * Return the AbstractFileSystemEntry for the specified path throw FileSystemException if the
      * specified path does not exist.
      * 
      * @param path - the path
      * @return the AbstractFileSystemEntry
      * 
      * @throws FileSystemException - if the specified path does not exist
      */
     protected AbstractFileSystemEntry getRequiredEntry(String path) {
         AbstractFileSystemEntry entry = getEntry(path)
         if (entry == null) {
             throw new FileSystemException("The path [" + normalize(path) + "] does not exist")
         }
         return entry
     }

     /**
      * Return true if the specified path exists
      * 
      * @param path - the path
      * @return true if the path exists
      */
     private boolean pathExists(String path) {
         return entries.get(normalize(path)) != null
     }

     /**
      * Throw AssertionError if the path is null. Throw FileSystemException if the specified
      * path does not exist.
      * 
      * @param path - the path
      * @throws AssertionError - if the specified path is null
      * @throws FileSystemException - if the specified path does not exist
      */
     private void verifyPathExists(String path) {
         assert path != null
         getRequiredEntry(path)
     }

     /**
      * Verify that the path refers to an existing directory (if isDirectory==true) or an existing
      * file (if isDirectory==false). Throw AssertionError if the path is null. Throw
      * FileSystemException if the specified path does not exist or is not a directory/file as
      * specified by isDirectory.
      * 
      * @param path - the path
      * @param isDirectory - true if the path should reference a directory false if it should be a
      *        file
      * 
      * @throws AssertionError - if the specified path is null
      * @throws FileSystemException - if the specified path does not exist or is not a directory/file
      *         as expected
      */
     private void verifyIsDirectory(String path, boolean isDirectory) {
         assert path != null
         AbstractFileSystemEntry entry = getRequiredEntry(path)
         if (entry.isDirectory() != isDirectory) {
             throw new FileSystemException("Path [" + path + "] is directory is " + entry.isDirectory())
         }
     }

     /**
      * Throw a FileSystemException if the parent directory for the specified path does not exist.
      * @param path - the path
      * @throws FileSystemException - if the parent directory of the path does not exist
      */
     private void verifyParentDirectoryExists(String path) throws FileSystemException {
         if (!parentDirectoryExists(path)) {
             throw new FileSystemException("Parent directory does not exist: " + getParent(path))
         }
     }

     /**
      * If the specified path has a parent, then verify that the parent exists
      * @param path - the path
      */
     private boolean parentDirectoryExists(String path) {
         String parent = getParent(path)
         if (parent != null) {
             return pathExists(parent)
         }
         return true
     }

     /**
      * Return true if the specified path represents a directory that contains one or more files or subdirectories
      * @param path - the path
      * @return true if the path has child entries
      */
     private boolean hasChildren(String path) {
         if (!isDirectory(path)) {
             return false
         }
         String normalizedPath = normalize(path)
         return entries.keySet().find { p -> p.startsWith(normalizedPath) && !normalizedPath.equals(p) }
     }

     /**
      * Return the List of files or subdirectory paths that are descendents of the specified path
      * @param path - the path
      * @return the List of the paths for the files and subdirectories that are children, grandchildren, etc. 
      */
     private List descendents(String path) {
         if (isDirectory(path)) {
             String normalizedPath = normalize(path)
             String normalizedDirPrefix = normalizedPath + SEPARATOR
             List descendents = new ArrayList()
             entries.keySet().each { p ->
                 if (p.startsWith(normalizedDirPrefix) && !normalizedPath.equals(p)) {
                     descendents.add(p)
                 }
             }
             return descendents
         }
         return Collections.EMPTY_LIST
     }
     
     /**
      * Return the List of files or subdirectory paths that are children of the specified path
      * @param path - the path
      * @return the List of the paths for the files and subdirectories that are children 
      */
     private List children(String path) {
         List descendents = descendents(path)
         List children = new ArrayList()
         String normalizedPath = normalize(path)
         descendents.each { descendentPath ->
             if (normalizedPath.equals(getParent(descendentPath))) {
                 children.add(descendentPath)
             }
         }
         return children
     }

 }