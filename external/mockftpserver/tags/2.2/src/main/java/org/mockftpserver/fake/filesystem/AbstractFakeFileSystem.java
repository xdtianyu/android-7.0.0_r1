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

import org.apache.log4j.Logger;
import org.mockftpserver.core.util.Assert;
import org.mockftpserver.core.util.PatternUtil;
import org.mockftpserver.core.util.StringUtil;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Date;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;

/**
 * Abstract superclass for implementation of the FileSystem interface that manage the files
 * and directories in memory, simulating a real file system.
 * <p/>
 * If the <code>createParentDirectoriesAutomatically</code> property is set to <code>true</code>,
 * then creating a directory or file will automatically create any parent directories (recursively)
 * that do not already exist. If <code>false</code>, then creating a directory or file throws an
 * exception if its parent directory does not exist. This value defaults to <code>true</code>.
 * <p/>
 * The <code>directoryListingFormatter</code> property holds an instance of            {@link DirectoryListingFormatter}                          ,
 * used by the <code>formatDirectoryListing</code> method to format directory listings in a
 * filesystem-specific manner. This property must be initialized by concrete subclasses.
 *
 * @author Chris Mair
 * @version $Revision$ - $Date$
 */
public abstract class AbstractFakeFileSystem implements FileSystem {

    private static final Logger LOG = Logger.getLogger(AbstractFakeFileSystem.class);

    /**
     * If <code>true</code>, creating a directory or file will automatically create
     * any parent directories (recursively) that do not already exist. If <code>false</code>,
     * then creating a directory or file throws an exception if its parent directory
     * does not exist. This value defaults to <code>true</code>.
     */
    private boolean createParentDirectoriesAutomatically = true;

    /**
     * The {@link DirectoryListingFormatter} used by the {@link #formatDirectoryListing(FileSystemEntry)}
     * method. This must be initialized by concrete subclasses.
     */
    private DirectoryListingFormatter directoryListingFormatter;

    private Map entries = new HashMap();

    //-------------------------------------------------------------------------
    // Public API
    //-------------------------------------------------------------------------

    public boolean isCreateParentDirectoriesAutomatically() {
        return createParentDirectoriesAutomatically;
    }

    public void setCreateParentDirectoriesAutomatically(boolean createParentDirectoriesAutomatically) {
        this.createParentDirectoriesAutomatically = createParentDirectoriesAutomatically;
    }

    public DirectoryListingFormatter getDirectoryListingFormatter() {
        return directoryListingFormatter;
    }

    public void setDirectoryListingFormatter(DirectoryListingFormatter directoryListingFormatter) {
        this.directoryListingFormatter = directoryListingFormatter;
    }

    /**
     * Add each of the entries in the specified List to this filesystem. Note that this does not affect
     * entries already existing within this filesystem.
     *
     * @param entriesToAdd - the List of FileSystemEntry entries to add
     */
    public void setEntries(List entriesToAdd) {
        for (Iterator iter = entriesToAdd.iterator(); iter.hasNext();) {
            FileSystemEntry entry = (FileSystemEntry) iter.next();
            add(entry);
        }
    }

    /**
     * Add the specified file system entry (file or directory) to this file system
     *
     * @param entry - the FileSystemEntry to add
     */
    public void add(FileSystemEntry entry) {
        String path = entry.getPath();
        checkForInvalidFilename(path);
        if (getEntry(path) != null) {
            throw new FileSystemException(path, "filesystem.pathAlreadyExists");
        }

        if (!parentDirectoryExists(path)) {
            String parent = getParent(path);
            if (createParentDirectoriesAutomatically) {
                add(new DirectoryEntry(parent));
            } else {
                throw new FileSystemException(parent, "filesystem.parentDirectoryDoesNotExist");
            }
        }

        // Set lastModified, if not already set
        if (entry.getLastModified() == null) {
            entry.setLastModified(new Date());
        }

        entries.put(getFileSystemEntryKey(path), entry);
        entry.lockPath();
    }

    /**
     * Delete the file or directory specified by the path. Return true if the file is successfully
     * deleted, false otherwise. If the path refers to a directory, it must be empty. Return false
     * if the path does not refer to a valid file or directory or if it is a non-empty directory.
     *
     * @param path - the path of the file or directory to delete
     * @return true if the file or directory is successfully deleted
     * @throws org.mockftpserver.core.util.AssertFailedException
     *          - if path is null
     * @see org.mockftpserver.fake.filesystem.FileSystem#delete(java.lang.String)
     */
    public boolean delete(String path) {
        Assert.notNull(path, "path");

        if (getEntry(path) != null && !hasChildren(path)) {
            removeEntry(path);
            return true;
        }
        return false;
    }

    /**
     * Return true if there exists a file or directory at the specified path
     *
     * @param path - the path
     * @return true if the file/directory exists
     * @throws AssertionError - if path is null
     * @see org.mockftpserver.fake.filesystem.FileSystem#exists(java.lang.String)
     */
    public boolean exists(String path) {
        Assert.notNull(path, "path");
        return getEntry(path) != null;
    }

    /**
     * Return true if the specified path designates an existing directory, false otherwise
     *
     * @param path - the path
     * @return true if path is a directory, false otherwise
     * @throws AssertionError - if path is null
     * @see org.mockftpserver.fake.filesystem.FileSystem#isDirectory(java.lang.String)
     */
    public boolean isDirectory(String path) {
        Assert.notNull(path, "path");
        FileSystemEntry entry = getEntry(path);
        return entry != null && entry.isDirectory();
    }

    /**
     * Return true if the specified path designates an existing file, false otherwise
     *
     * @param path - the path
     * @return true if path is a file, false otherwise
     * @throws AssertionError - if path is null
     * @see org.mockftpserver.fake.filesystem.FileSystem#isFile(java.lang.String)
     */
    public boolean isFile(String path) {
        Assert.notNull(path, "path");
        FileSystemEntry entry = getEntry(path);
        return entry != null && !entry.isDirectory();
    }

    /**
     * Return the List of FileSystemEntry objects for the files in the specified directory or group of
     * files. If the path specifies a single file, then return a list with a single FileSystemEntry
     * object representing that file. If the path does not refer to an existing directory or
     * group of files, then an empty List is returned.
     *
     * @param path - the path specifying a directory or group of files; may contain wildcards (? or *)
     * @return the List of FileSystemEntry objects for the specified directory or file; may be empty
     * @see org.mockftpserver.fake.filesystem.FileSystem#listFiles(java.lang.String)
     */
    public List listFiles(String path) {
        if (isFile(path)) {
            return Collections.singletonList(getEntry(path));
        }

        List entryList = new ArrayList();
        List children = children(path);
        Iterator iter = children.iterator();
        while (iter.hasNext()) {
            String childPath = (String) iter.next();
            FileSystemEntry fileSystemEntry = getEntry(childPath);
            entryList.add(fileSystemEntry);
        }
        return entryList;
    }

    /**
     * Return the List of filenames in the specified directory path or file path. If the path specifies
     * a single file, then return that single filename. The returned filenames do not
     * include a path. If the path does not refer to a valid directory or file path, then an empty List
     * is returned.
     *
     * @param path - the path specifying a directory or group of files; may contain wildcards (? or *)
     * @return the List of filenames (not including paths) for all files in the specified directory
     *         or file path; may be empty
     * @throws AssertionError - if path is null
     * @see org.mockftpserver.fake.filesystem.FileSystem#listNames(java.lang.String)
     */
    public List listNames(String path) {
        if (isFile(path)) {
            return Collections.singletonList(getName(path));
        }

        List filenames = new ArrayList();
        List children = children(path);
        Iterator iter = children.iterator();
        while (iter.hasNext()) {
            String childPath = (String) iter.next();
            FileSystemEntry fileSystemEntry = getEntry(childPath);
            filenames.add(fileSystemEntry.getName());
        }
        return filenames;
    }

    /**
     * Rename the file or directory. Specify the FROM path and the TO path. Throw an exception if the FROM path or
     * the parent directory of the TO path do not exist; or if the rename fails for another reason.
     *
     * @param fromPath - the source (old) path + filename
     * @param toPath   - the target (new) path + filename
     * @throws AssertionError      - if fromPath or toPath is null
     * @throws FileSystemException - if the rename fails.
     */
    public void rename(String fromPath, String toPath) {
        Assert.notNull(toPath, "toPath");
        Assert.notNull(fromPath, "fromPath");

        FileSystemEntry entry = getRequiredEntry(fromPath);

        if (exists(toPath)) {
            throw new FileSystemException(toPath, "filesystem.alreadyExists");
        }

        String normalizedFromPath = normalize(fromPath);
        String normalizedToPath = normalize(toPath);

        if (!entry.isDirectory()) {
            renamePath(entry, normalizedToPath);
            return;
        }

        if (normalizedToPath.startsWith(normalizedFromPath + this.getSeparator())) {
            throw new FileSystemException(toPath, "filesystem.renameFailed");
        }

        // Create the TO directory entry first so that the destination path exists when you
        // move the children. Remove the FROM path after all children have been moved
        add(new DirectoryEntry(normalizedToPath));

        List children = descendents(fromPath);
        Iterator iter = children.iterator();
        while (iter.hasNext()) {
            String childPath = (String) iter.next();
            FileSystemEntry child = getRequiredEntry(childPath);
            String normalizedChildPath = normalize(child.getPath());
            Assert.isTrue(normalizedChildPath.startsWith(normalizedFromPath), "Starts with FROM path");
            String childToPath = normalizedToPath + normalizedChildPath.substring(normalizedFromPath.length());
            renamePath(child, childToPath);
        }
        Assert.isTrue(children(normalizedFromPath).isEmpty(), "Must have no children: " + normalizedFromPath);
        removeEntry(normalizedFromPath);
    }

    /**
     * @see java.lang.Object#toString()
     */
    public String toString() {
        return this.getClass().getName() + entries;
    }

    /**
     * Return the formatted directory listing entry for the file represented by the specified FileSystemEntry
     *
     * @param fileSystemEntry - the FileSystemEntry representing the file or directory entry to be formatted
     * @return the the formatted directory listing entry
     */
    public String formatDirectoryListing(FileSystemEntry fileSystemEntry) {
        Assert.notNull(directoryListingFormatter, "directoryListingFormatter");
        Assert.notNull(fileSystemEntry, "fileSystemEntry");
        return directoryListingFormatter.format(fileSystemEntry);
    }

    /**
     * Build a path from the two path components. Concatenate path1 and path2. Insert the path
     * separator character in between if necessary (i.e., if both are non-empty and path1 does not already
     * end with a separator character AND path2 does not begin with one).
     *
     * @param path1 - the first path component may be null or empty
     * @param path2 - the second path component may be null or empty
     * @return the normalized path resulting from concatenating path1 to path2
     */
    public String path(String path1, String path2) {
        StringBuffer buf = new StringBuffer();
        if (path1 != null && path1.length() > 0) {
            buf.append(path1);
        }
        if (path2 != null && path2.length() > 0) {
            if ((path1 != null && path1.length() > 0)
                    && (!isSeparator(path1.charAt(path1.length() - 1)))
                    && (!isSeparator(path2.charAt(0)))) {
                buf.append(this.getSeparator());
            }
            buf.append(path2);
        }
        return normalize(buf.toString());
    }

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
    public String getParent(String path) {
        List parts = normalizedComponents(path);
        if (parts.size() < 2) {
            return null;
        }
        parts.remove(parts.size() - 1);
        return componentsToPath(parts);
    }

    /**
     * Returns the name of the file or directory denoted by this abstract
     * pathname.  This is just the last name in the pathname's name
     * sequence.  If the pathname's name sequence is empty, then the empty string is returned.
     *
     * @param path - the path
     * @return The name of the file or directory denoted by this abstract pathname, or the
     *         empty string if this pathname's name sequence is empty
     */
    public String getName(String path) {
        Assert.notNull(path, "path");
        String normalized = normalize(path);
        int separatorIndex = normalized.lastIndexOf(this.getSeparator());
        return (separatorIndex == -1) ? normalized : normalized.substring(separatorIndex + 1);
    }

    /**
     * Returns the FileSystemEntry object representing the file system entry at the specified path, or null
     * if the path does not specify an existing file or directory within this file system.
     *
     * @param path - the path of the file or directory within this file system
     * @return the FileSystemEntry containing the information for the file or directory, or else null
     * @see FileSystem#getEntry(String)
     */
    public FileSystemEntry getEntry(String path) {
        return (FileSystemEntry) entries.get(getFileSystemEntryKey(path));
    }

    //-------------------------------------------------------------------------
    // Abstract Methods
    //-------------------------------------------------------------------------

    /**
     * @param path - the path
     * @return true if the specified dir/file path name is valid according to the current filesystem.
     */
    protected abstract boolean isValidName(String path);

    /**
     * @return the file system-specific file separator as a char
     */
    protected abstract char getSeparatorChar();

    /**
     * @param pathComponent - the component (piece) of the path to check
     * @return true if the specified path component is a root for this filesystem
     */
    protected abstract boolean isRoot(String pathComponent);

    /**
     * Return true if the specified char is a separator character for this filesystem
     *
     * @param c - the character to test
     * @return true if the specified char is a separator character
     */
    protected abstract boolean isSeparator(char c);

    //-------------------------------------------------------------------------
    // Internal Helper Methods
    //-------------------------------------------------------------------------

    /**
     * @return the file system-specific file separator as a String
     */
    protected String getSeparator() {
        return Character.toString(getSeparatorChar());
    }

    /**
     * Return the normalized and unique key used to access the file system entry
     *
     * @param path - the path
     * @return the corresponding normalized key
     */
    protected String getFileSystemEntryKey(String path) {
        return normalize(path);
    }

    /**
     * Return the standard, normalized form of the path.
     *
     * @param path - the path
     * @return the path in a standard, unique, canonical form
     * @throws AssertionError - if path is null
     */
    protected String normalize(String path) {
        return componentsToPath(normalizedComponents(path));
    }

    /**
     * Throw an InvalidFilenameException if the specified path is not valid.
     *
     * @param path - the path
     */
    protected void checkForInvalidFilename(String path) {
        if (!isValidName(path)) {
            throw new InvalidFilenameException(path);
        }
    }

    /**
     * Rename the file system entry to the specified path name
     *
     * @param entry  - the file system entry
     * @param toPath - the TO path (normalized)
     */
    protected void renamePath(FileSystemEntry entry, String toPath) {
        String normalizedFrom = normalize(entry.getPath());
        String normalizedTo = normalize(toPath);
        LOG.info("renaming from [" + normalizedFrom + "] to [" + normalizedTo + "]");
        FileSystemEntry newEntry = entry.cloneWithNewPath(normalizedTo);
        add(newEntry);
        // Do this at the end, in case the addEntry() failed
        removeEntry(normalizedFrom);
    }

    /**
     * Return the FileSystemEntry for the specified path. Throw FileSystemException if the
     * specified path does not exist.
     *
     * @param path - the path
     * @return the FileSystemEntry
     * @throws FileSystemException - if the specified path does not exist
     */
    protected FileSystemEntry getRequiredEntry(String path) {
        FileSystemEntry entry = getEntry(path);
        if (entry == null) {
            LOG.error("Path does not exist: " + path);
            throw new FileSystemException(normalize(path), "filesystem.doesNotExist");
        }
        return entry;
    }

    /**
     * Return the components of the specified path as a List. The components are normalized, and
     * the returned List does not include path separator characters.
     *
     * @param path - the path
     * @return the List of normalized components
     */
    protected List normalizedComponents(String path) {
        Assert.notNull(path, "path");
        char otherSeparator = this.getSeparatorChar() == '/' ? '\\' : '/';
        String p = path.replace(otherSeparator, this.getSeparatorChar());

        // TODO better way to do this
        if (p.equals(this.getSeparator())) {
            return Collections.singletonList("");
        }
        List result = new ArrayList();
        if (p.length() > 0) {
            String[] parts = p.split("\\" + this.getSeparator());
            for (int i = 0; i < parts.length; i++) {
                String part = parts[i];
                if (part.equals("..")) {
                    result.remove(result.size() - 1);
                } else if (!part.equals(".")) {
                    result.add(part);
                }
            }
        }
        return result;
    }

    /**
     * Build a path from the specified list of path components
     *
     * @param components - the list of path components
     * @return the resulting path
     */
    protected String componentsToPath(List components) {
        if (components.size() == 1) {
            String first = (String) components.get(0);
            if (first.length() == 0 || isRoot(first)) {
                return first + this.getSeparator();
            }
        }
        return StringUtil.join(components, this.getSeparator());
    }

    /**
     * Return true if the specified path designates an absolute file path.
     *
     * @param path - the path
     * @return true if path is absolute, false otherwise
     * @throws AssertionError - if path is null
     */
    public boolean isAbsolute(String path) {
        return isValidName(path);
    }

    /**
     * Return true if the specified path exists
     *
     * @param path - the path
     * @return true if the path exists
     */
    private boolean pathExists(String path) {
        return getEntry(path) != null;
    }

    /**
     * If the specified path has a parent, then verify that the parent exists
     *
     * @param path - the path
     * @return true if the parent of the specified path exists
     */
    private boolean parentDirectoryExists(String path) {
        String parent = getParent(path);
        return parent == null || pathExists(parent);
    }

    /**
     * Return true if the specified path represents a directory that contains one or more files or subdirectories
     *
     * @param path - the path
     * @return true if the path has child entries
     */
    private boolean hasChildren(String path) {
        if (!isDirectory(path)) {
            return false;
        }
        String key = getFileSystemEntryKey(path);
        Iterator iter = entries.keySet().iterator();
        while (iter.hasNext()) {
            String p = (String) iter.next();
            if (p.startsWith(key) && !key.equals(p)) {
                return true;
            }
        }
        return false;
    }

    /**
     * Return the List of files or subdirectory paths that are descendents of the specified path
     *
     * @param path - the path
     * @return the List of the paths for the files and subdirectories that are children, grandchildren, etc.
     */
    private List descendents(String path) {
        if (isDirectory(path)) {
            String normalizedPath = getFileSystemEntryKey(path);
            String separator = (normalizedPath.endsWith(getSeparator())) ? "" : getSeparator();
            String normalizedDirPrefix = normalizedPath + separator;
            List descendents = new ArrayList();
            Iterator iter = entries.entrySet().iterator();
            while (iter.hasNext()) {
                Map.Entry mapEntry = (Map.Entry) iter.next();
                String p = (String) mapEntry.getKey();
                if (p.startsWith(normalizedDirPrefix) && !normalizedPath.equals(p)) {
                    FileSystemEntry fileSystemEntry = (FileSystemEntry) mapEntry.getValue();
                    descendents.add(fileSystemEntry.getPath());
                }
            }
            return descendents;
        }
        return Collections.EMPTY_LIST;
    }

    /**
     * Return the List of files or subdirectory paths that are children of the specified path
     *
     * @param path - the path
     * @return the List of the paths for the files and subdirectories that are children
     */
    private List children(String path) {
        String lastComponent = getName(path);
        boolean containsWildcards = PatternUtil.containsWildcards(lastComponent);
        String dir = containsWildcards ? getParent(path) : path;
        String pattern = containsWildcards ? PatternUtil.convertStringWithWildcardsToRegex(getName(path)) : null;
        LOG.debug("path=" + path + " lastComponent=" + lastComponent + " containsWildcards=" + containsWildcards + " dir=" + dir + " pattern=" + pattern);

        List descendents = descendents(dir);
        List children = new ArrayList();
        String normalizedDir = normalize(dir);
        Iterator iter = descendents.iterator();
        while (iter.hasNext()) {
            String descendentPath = (String) iter.next();

            boolean patternEmpty = pattern == null || pattern.length() == 0;
            if (normalizedDir.equals(getParent(descendentPath)) &&
                    (patternEmpty || (getName(descendentPath).matches(pattern)))) {
                children.add(descendentPath);
            }
        }
        return children;
    }

    private void removeEntry(String path) {
        entries.remove(getFileSystemEntryKey(path));
    }

}