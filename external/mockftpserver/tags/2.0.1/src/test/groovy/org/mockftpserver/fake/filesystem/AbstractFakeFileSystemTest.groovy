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

import org.mockftpserver.core.util.IoUtil

/**
 * Tests for subclasses of AbstractFakeFileSystem. Subclasses must define
 *
 * @version $Revision$ - $Date$
 *
 * @author Chris Mair
 */
abstract class AbstractFakeFileSystemTest extends AbstractFileSystemTest {

    // -------------------------------------------------------------------------
    // Tests
    // -------------------------------------------------------------------------

    void testDefaultDirectoryListingFormatterClass() {
        assert fileSystem.directoryListingFormatter.class == expectedDirectoryListingFormatterClass
    }

    void testAdd_PathLocked() {
        def dirEntry = new DirectoryEntry(NEW_DIR)
        fileSystem.add(dirEntry)
        def fileEntry = new FileEntry(NEW_FILE)
        fileSystem.add(fileEntry)

        // The path should be locked for both entries
        shouldFail { dirEntry.setPath('abc') }
        shouldFail { fileEntry.setPath('abc') }
    }

    void testAdd_Directory_CreateParentDirectoriesAutomatically() {
        final NEW_SUBDIR = fileSystem.path(NEW_DIR, "sub")
        assert !fileSystem.exists(NEW_DIR), "Before createDirectory"
        assert !fileSystem.exists(NEW_SUBDIR), "Before createDirectory"

        fileSystem.createParentDirectoriesAutomatically = true
        fileSystem.add(new DirectoryEntry(NEW_SUBDIR))
        assert fileSystem.exists(NEW_DIR), "After createDirectory"
        assert fileSystem.exists(NEW_SUBDIR), "$NEW_SUBDIR: After createDirectory"
    }

    void testAdd_File_CreateParentDirectoriesAutomatically() {
        final NEW_FILE_IN_SUBDIR = fileSystem.path(NEW_DIR, "abc.txt")
        assert !fileSystem.exists(NEW_DIR), "Before createDirectory"
        assert !fileSystem.exists(NEW_FILE_IN_SUBDIR), "Before createDirectory"

        fileSystem.createParentDirectoriesAutomatically = true
        fileSystem.add(new FileEntry(NEW_FILE_IN_SUBDIR))
        assert fileSystem.exists(NEW_DIR), "After createDirectory"
        assert fileSystem.exists(NEW_FILE_IN_SUBDIR), "$NEW_FILE_IN_SUBDIR: After createDirectory"
    }

    void testAdd_File_CreateParentDirectoriesAutomatically_False() {
        fileSystem.createParentDirectoriesAutomatically = false
        final NEW_FILE_IN_SUBDIR = fileSystem.path(NEW_DIR, "abc.txt")
        assert !fileSystem.exists(NEW_DIR), "Before createDirectory"

        shouldFail(FileSystemException) { fileSystem.add(new FileEntry(NEW_FILE_IN_SUBDIR)) }
        assert !fileSystem.exists(NEW_DIR), "After createDirectory"
    }

    void testSetEntries() {
        fileSystem.createParentDirectoriesAutomatically = false
        def entries = [new FileEntry(NEW_FILE), new DirectoryEntry(NEW_DIR)]
        fileSystem.setEntries(entries)
        assert fileSystem.exists(NEW_DIR)
        assert fileSystem.exists(NEW_FILE)
    }

    void testToString() {
        String toString = fileSystem.toString()
        LOG.info("toString=" + toString)
        assert toString.contains(EXISTING_DIR)
        assert toString.contains(EXISTING_FILE)
    }

    void testFormatDirectoryListing() {
        def fileEntry = new FileEntry(path: 'abc')
        def formatter = [format: {f ->
            assert f == fileEntry
            return 'abc'
        }] as DirectoryListingFormatter
        fileSystem.directoryListingFormatter = formatter
        assert fileSystem.formatDirectoryListing(fileEntry) == 'abc'
    }

    void testFormatDirectoryListing_NullDirectoryListingFormatter() {
        fileSystem.directoryListingFormatter = null
        def fileEntry = new FileEntry('abc')
        shouldFailWithMessageContaining('directoryListingFormatter') { assert fileSystem.formatDirectoryListing(fileEntry) }
    }

    void testFormatDirectoryListing_NullFileSystemEntry() {
        def formatter = [format: {f -> }] as DirectoryListingFormatter
        fileSystem.directoryListingFormatter = formatter
        shouldFailWithMessageContaining('fileSystemEntry') { assert fileSystem.formatDirectoryListing(null) }
    }

    void testGetEntry() {
        assert fileSystem.getEntry(NO_SUCH_DIR) == null
        assert fileSystem.getEntry(NO_SUCH_FILE) == null

        assert fileSystem.getEntry(EXISTING_FILE).path == EXISTING_FILE
        assert fileSystem.getEntry(EXISTING_DIR).path == EXISTING_DIR

        def permissions = new Permissions('-wxrwx---')
        def fileEntry = new FileEntry(path: NEW_FILE, lastModified: DATE, contents: 'abc', owner: 'owner',
                group: 'group', permissions: permissions)
        fileSystem.add(fileEntry)
        def entry = fileSystem.getEntry(NEW_FILE)
        LOG.info(entry)
        assert entry.path == NEW_FILE
        assert !entry.directory
        assert entry.size == 3
        assert entry.owner == 'owner'
        assert entry.group == 'group'
        assert entry.permissions == permissions
    }

    void testNormalize_Null() {
        shouldFailWithMessageContaining("path") { fileSystem.normalize(null) }
    }

    void testGetName_Null() {
        shouldFailWithMessageContaining("path") { fileSystem.getName(null) }
    }

    //--------------------------------------------------------------------------
    // Abstract Methods
    //--------------------------------------------------------------------------

    protected abstract Class getExpectedDirectoryListingFormatterClass()

    //--------------------------------------------------------------------------
    // Internal Helper Methods
    //--------------------------------------------------------------------------

    /**
     * Verify the contents of the file at the specified path read from its InputSteam
     *
     * @param fileSystem - the FileSystem instance
     * @param expectedContents - the expected contents
     * @throws IOException
     * @see org.mockftpserver.fake.filesystem.AbstractFileSystemTest#verifyFileContents(FileSystem,
     * String , String )
     */
    protected void verifyFileContents(FileSystem fileSystem, String path, String expectedContents) throws IOException {
        def fileEntry = fileSystem.getEntry(path)
        InputStream input = fileEntry.createInputStream()
        byte[] bytes = IoUtil.readBytes(input)
        LOG.info("bytes=[" + new String(bytes) + "]")
        assertEquals("contents: actual=[" + new String(bytes) + "]", expectedContents.getBytes(), bytes)
    }

}