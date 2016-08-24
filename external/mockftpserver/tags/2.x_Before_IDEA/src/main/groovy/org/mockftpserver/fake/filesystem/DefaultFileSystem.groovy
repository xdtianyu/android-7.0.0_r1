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
import java.io.FileInputStream
import java.io.FileNotFoundException
import java.io.FileOutputStream
import java.io.IOException
import java.io.InputStream
import java.io.OutputStream
import java.util.ArrayList
import java.util.Arrays
import java.util.Collections
import java.util.List

/**
 * Implementation of the FileSystem interface that uses a real underlying file system.
 * You can optionally set a root of this virtual filesystem within the "real" filesystem by setting
 * the <code>root</code> property. If this value is non-null, then all paths are considered relative 
 * to this root path.
 * 
 * @version $Revision: $ - $Date: $
 * 
 * @author Chris Mair
 */
class DefaultFileSystem implements FileSystem {

     /**
      * Root of this virtual filesystem in the "real" filesystem. If this value is non-null, then all
      * paths are considered relative to this root path.
      */
     private String root

     /**
      * @see org.mockftpserver.fake.filesystem.FileSystem#exists(java.lang.String)
      */
     public boolean exists(String path) {
         assert path != null
         return fileForPath(path).exists()
     }

     /**
      * @see org.mockftpserver.fake.filesystem.FileSystem#isDirectory(java.lang.String)
      */
     public boolean isDirectory(String path) {
         assert path != null
         return fileForPath(path).isDirectory()
     }

     /**
      * @see org.mockftpserver.fake.filesystem.FileSystem#isFile(java.lang.String)
      */
     public boolean isFile(String path) {
         assert path != null
         return fileForPath(path).isFile()
     }

     /**
      * @see org.mockftpserver.fake.filesystem.FileSystem#createDirectory(java.lang.String)
      */
     public boolean createDirectory(String path) {
         assert path != null
         return fileForPath(path).mkdir()
     }

     /**
      * Creates an empty file with the specified pathname.
      * 
      * @param path - the path of the filename to create
      * @return true if and only if the file was created false otherwise
      * 
      * @throws AssertionError - if path is null
      * @throws InvalidFilenameException - if an I/O error occurs indicating that the file path is not valid
      */
     public boolean createFile(String path) {
         assert path != null
         try {
             return fileForPath(path).createNewFile()
         }
         catch (IOException e) {
             throw new InvalidFilenameException(e, path)
         }
     }

     /**
      * Create and return a new OutputStream for writing to the file at the specified path
      * @param path - the path of the file
      * @param append - true if the OutputStream should append to the end of the file if the file already exists
      * 
      * @throws AssertionError - if path is null
      * @throws FileNotFoundException - wraps a FileNotFoundException if thrown
      * 
      * @see org.mockftpserver.fake.filesystem.FileSystem#createOutputStream(java.lang.String, boolean)
      */
     public OutputStream createOutputStream(String path, boolean append) {
         assert path != null
         try {
             return new FileOutputStream(fileForPath(path), append)
         }
         catch (FileNotFoundException e) {
             throw new InvalidFilenameException(e, path)
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
         assert path != null
         try {
             return new FileInputStream(fileForPath(path))
         }
         catch (FileNotFoundException e) {
             throw new FileSystemException(e)
         }
     }
         
     /**
      * @see org.mockftpserver.fake.filesystem.FileSystem#listFiles(java.lang.String)
      */
     public List listFiles(String path) {
         assert path != null
         File pathFile = fileForPath(path)
         
         if (pathFile.isFile()) {
             return [buildFileInfoForFile(pathFile)]
         }
         
         def files = pathFile.listFiles()
         List fileInfoList = []
         if (files != null) {
             files.each { file ->
                 fileInfoList.add(buildFileInfoForFile(file))
             }
         }
         return fileInfoList
     }

     /**
      * Build a FileInfo based on the file or directory represented by the File object
      * @param file - the File object for the file or directory
      */ 
     private FileInfo buildFileInfoForFile(File file) {
         file.isDirectory()  \
             ? FileInfo.forDirectory(file.getName(), new Date(file.lastModified()))  \
             : FileInfo.forFile(file.getName(), file.length(), new Date(file.lastModified()))
     }
     
     /**
      * @see org.mockftpserver.fake.filesystem.FileSystem#listNames(java.lang.String)
      */
     public List listNames(String path) {
         assert path != null
         def filenames = fileForPath(path).list()
         return (filenames == null) ? [] : filenames as List
     }

     /**
      * @see org.mockftpserver.fake.filesystem.FileSystem#delete(java.lang.String)
      */
     public boolean delete(String path) {
         assert path != null
         return fileForPath(path).delete()
     }

     /**
      * @see org.mockftpserver.fake.filesystem.FileSystem#rename(java.lang.String, java.lang.String)
      */
     public boolean rename(String fromPath, String toPath) {
         assert fromPath != null
         assert toPath != null
         File toFile = fileForPath(toPath)
         return fileForPath(fromPath).renameTo(toFile)
     }

     /**
      * Set the root of this virtual filesystem in the "real" filesystem. If this value is non-null,
      * then all paths are considered relative to this root path.
      * 
      * @param rootPath - the root of this virtual filesystem in the "real" filesystem
      */
     public void setRoot(String rootPath) {
         this.root = rootPath
     }

     //-------------------------------------------------------------------------
     // Path-related Methods
     //-------------------------------------------------------------------------

     /**
      * Returns the name of the file or directory denoted by this abstract pathname.  
      * This is just the last name in the pathname's name sequence. If the pathname's 
      * name sequence is empty, then the empty string is returned.
      *
      * @return  The name of the file or directory denoted by this abstract pathname, or the 
      *          empty string if this pathname's name sequence is empty
      *          
      * @see File#getName()         
      * @see org.mockftpserver.fake.filesystem.FileSystem#getName(java.lang.String)
      */
     public String getName(String path) {
         assert path != null
         return new File(path).getName()
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
      * 
      * @see org.mockftpserver.fake.filesystem.FileSystem#getParent(java.lang.String)
      */
     public String getParent(String path) {
         assert path != null
         return new File(path).getParent()
     }

     /**
      * Return the standard, normalized form of the path. 
      * @param path - the abstract path
      * @return the normalized path
      * 
      * @throws AssertionError - if path is null
      * @throws InvalidFilenameException - if an IOException occurs while determining the canonical path from the real" file system.
      * 
      * @see org.mockftpserver.fake.filesystem.FileSystem#normalize(java.lang.String)
      */
     public String normalize(String path) {
         assert path != null
         try {
             return new File(path).getCanonicalPath()
         }
         catch (IOException e) {
             throw new InvalidFilenameException(e, path)
         }
     }

     /**
      * Build a path from the two path components. Concatenate path1 and path2. Insert the system-dependent
      * separator character in between if necessary (i.e., if both are non-empty and path1 does not already
      * end with a separator character AND path2 does not begin with one).
      * 
      * @param path1 - the first path component may be null or empty
      * @param path2 - the second path component may be null or empty
      * @return the path resulting from concatenating path1 to path2
      * 
      * @see org.mockftpserver.fake.filesystem.FileSystem#path(java.lang.String, java.lang.String)
      */
      public String path(String path1, String path2) {
         StringBuffer buf = new StringBuffer()
         if (path1 != null && path1.length() > 0) {
             buf.append(path1)
         }
         if (path2 != null && path2.length() > 0) {
             if ((path1 != null && path1.length() > 0)
                 && (isNotSeparator(path1.charAt(path1.length()-1)))
                 && (isNotSeparator(path2.charAt(0)))) {
                 buf.append(File.separator)
             }
             buf.append(path2)
         }
         
         return buf.toString()
     }

     /**
      * Return true if the specified path designates an absolute file path. For Unix 
      * paths, a path is absolute if it starts with the '/' character. For Windows
      * paths, a path is absolute if it starts with a drive specifier followed by 
      * '\' or '/', or if it starts with "\\". 
      * 
      * @param path - the path
      * @return true if path is absolute, false otherwise
      * 
      * @throws AssertionError - if path is null
      */
     boolean isAbsolute(String path) {
          assert path != null
          return fileForPath(path).isAbsolute()
     }
      
     // -------------------------------------------------------------------------
     // Internal Helper Methods
     // -------------------------------------------------------------------------

     /**
      * Return a java.io.File object for the specified path. If the root is set, then the path
      * is considered relative to the root path.
      * @param path - the path
      * @return a File object for the specified path
      */
     private File fileForPath(String path) {
         return (root == null) ? new File(path) : new File(root, path) 
     }

     /**
      * Return true if the specified char is NOT a separator character ('\' or '/')
      * @param c - the character to test
      * @return true if the specified char is NOT a separator character ('\' or '/')
      */
     private boolean isNotSeparator(char c) {
         return c != '\\' && c != '/'
     }
     
}