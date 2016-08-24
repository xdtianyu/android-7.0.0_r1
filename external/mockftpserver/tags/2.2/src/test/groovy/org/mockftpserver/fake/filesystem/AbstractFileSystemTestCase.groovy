/*
 * Copyright 2010 the original author or authors.
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

import org.mockftpserver.test.AbstractGroovyTestCase

/**
 * Abstract superclass for tests of FileSystem implementation classes. Contains common
 * tests and test infrastructure. 
 *
 * @version $Revision$ - $Date$
 *
 * @author Chris Mair
 */
abstract class AbstractFileSystemTestCase extends AbstractGroovyTestCase {

    public static final FILENAME1 = "File1.txt"
    public static final FILENAME2 = "file2.txt"
    public static final DIR1 = "dir1"
    public static final NEW_DIRNAME = "testDir"
    public static final ILLEGAL_FILE = "xx/yy////z!<>?*z.txt"
    public static final EXISTING_FILE_CONTENTS = "abc 123 %^& xxx"
    public static final DATE = new Date()

    // These must be set by the concrete subclass (in its constructor)
    protected String NEW_DIR = null
    protected String NEW_FILE = null
    protected String EXISTING_DIR = null
    protected String EXISTING_FILE = null
    protected NO_SUCH_DIR = null
    protected NO_SUCH_FILE = null

    protected FileSystem fileSystem

    //-------------------------------------------------------------------------
    // Common Tests
    //-------------------------------------------------------------------------

    void testExists() {
        assert !fileSystem.exists(NEW_FILE)
        assert !fileSystem.exists(NEW_DIR)
        assert !fileSystem.exists(ILLEGAL_FILE)
        assert fileSystem.exists(EXISTING_FILE)
        assert fileSystem.exists(EXISTING_DIR)

        shouldFailWithMessageContaining("path") { fileSystem.exists(null) }
    }

    void testIsDirectory() {
        assert fileSystem.isDirectory(EXISTING_DIR)
        assert !fileSystem.isDirectory(EXISTING_FILE)
        assert !fileSystem.isDirectory(NO_SUCH_DIR)
        assert !fileSystem.isDirectory(NO_SUCH_FILE)
        assert !fileSystem.isDirectory(ILLEGAL_FILE)

        shouldFailWithMessageContaining("path") { fileSystem.isDirectory(null) }
    }

    void testIsFile() {
        assert fileSystem.isFile(EXISTING_FILE)
        assert !fileSystem.isFile(EXISTING_DIR)
        assert !fileSystem.isFile(NO_SUCH_DIR)
        assert !fileSystem.isFile(NO_SUCH_FILE)
        assert !fileSystem.isFile(ILLEGAL_FILE)

        shouldFailWithMessageContaining("path") { fileSystem.isFile(null) }
    }

    void testAdd_Directory() {
        assert !fileSystem.exists(NEW_DIR), "Before createDirectory"
        fileSystem.add(new DirectoryEntry(NEW_DIR))
        assert fileSystem.exists(NEW_DIR), "After createDirectory"

        // Duplicate directory
        shouldThrowFileSystemExceptionWithMessageKey('filesystem.pathAlreadyExists') {
            fileSystem.add(new DirectoryEntry(NEW_DIR))
        }

        // The parent of the path does not exist
        shouldThrowFileSystemExceptionWithMessageKey('filesystem.parentDirectoryDoesNotExist') {
            fileSystem.add(new DirectoryEntry(NEW_DIR + "/abc/def"))
        }

        shouldFail(InvalidFilenameException) { fileSystem.add(new DirectoryEntry(ILLEGAL_FILE)) }
        shouldFailWithMessageContaining("path") { fileSystem.add(new DirectoryEntry(null)) }
    }

    void testAdd_File() {
        assert !fileSystem.exists(NEW_FILE), "Before createFile"
        fileSystem.add(new FileEntry(NEW_FILE))
        assert fileSystem.exists(NEW_FILE), "After createFile"

        // File already exists
        shouldThrowFileSystemExceptionWithMessageKey('filesystem.pathAlreadyExists') {
            fileSystem.add(new FileEntry(NEW_FILE))
        }

        // The parent of the path does not exist
        shouldThrowFileSystemExceptionWithMessageKey('filesystem.parentDirectoryDoesNotExist') {
            fileSystem.add(new FileEntry(NEW_DIR + "/abc/def"))
        }

        shouldThrowFileSystemExceptionWithMessageKey('filesystem.parentDirectoryDoesNotExist') {
            fileSystem.add(new FileEntry(NO_SUCH_DIR))
        }

        shouldFail(InvalidFilenameException) { fileSystem.add(new FileEntry(ILLEGAL_FILE)) }

        shouldFailWithMessageContaining("path") { fileSystem.add(new FileEntry(null)) }
    }

    void testRename_NullFromPath() {
        shouldFailWithMessageContaining("fromPath") { fileSystem.rename(null, FILENAME1) }
    }

    void testRename_NullToPath() {
        shouldFailWithMessageContaining("toPath") { fileSystem.rename(FILENAME1, null) }
    }

    void testListNames() {
        fileSystem.add(new DirectoryEntry(NEW_DIR))
        assert fileSystem.listNames(NEW_DIR) == []

        fileSystem.add(new FileEntry(p(NEW_DIR, FILENAME1)))
        fileSystem.add(new FileEntry(p(NEW_DIR, FILENAME2)))
        fileSystem.add(new DirectoryEntry(p(NEW_DIR, DIR1)))
        fileSystem.add(new FileEntry(p(NEW_DIR, DIR1, "/abc.def")))

        List filenames = fileSystem.listNames(NEW_DIR)
        LOG.info("filenames=" + filenames)
        assertSameIgnoringOrder(filenames, [FILENAME1, FILENAME2, DIR1])

        // Specify a filename instead of a directory name
        assert [FILENAME1] == fileSystem.listNames(p(NEW_DIR, FILENAME1))

        assert [] == fileSystem.listNames(NO_SUCH_DIR)

        shouldFailWithMessageContaining("path") { fileSystem.listNames(null) }
    }

    void testListNames_Wildcards() {
        fileSystem.add(new DirectoryEntry(NEW_DIR))
        fileSystem.add(new FileEntry(p(NEW_DIR, 'abc.txt')))
        fileSystem.add(new FileEntry(p(NEW_DIR, 'def.txt')))

        assertSameIgnoringOrder(fileSystem.listNames(p(NEW_DIR, '*.txt')), ['abc.txt', 'def.txt'])
        assertSameIgnoringOrder(fileSystem.listNames(p(NEW_DIR, '*')), ['abc.txt', 'def.txt'])
        assertSameIgnoringOrder(fileSystem.listNames(p(NEW_DIR, '???.???')), ['abc.txt', 'def.txt'])
        assertSameIgnoringOrder(fileSystem.listNames(p(NEW_DIR, '*.exe')), [])
        assertSameIgnoringOrder(fileSystem.listNames(p(NEW_DIR, 'abc.???')), ['abc.txt'])
        assertSameIgnoringOrder(fileSystem.listNames(p(NEW_DIR, 'a?c.?xt')), ['abc.txt'])
        assertSameIgnoringOrder(fileSystem.listNames(p(NEW_DIR, 'd?f.*')), ['def.txt'])
    }

    void testListFiles() {
        fileSystem.add(new DirectoryEntry(NEW_DIR))
        assert [] == fileSystem.listFiles(NEW_DIR)

        def path1 = p(NEW_DIR, FILENAME1)
        def fileEntry1 = new FileEntry(path1)
        fileSystem.add(fileEntry1)
        assert fileSystem.listFiles(NEW_DIR) == [fileEntry1]

        // Specify a filename instead of a directory name
        assert fileSystem.listFiles(p(NEW_DIR, FILENAME1)) == [fileEntry1]

        def fileEntry2 = new FileEntry(p(NEW_DIR, FILENAME2))
        fileSystem.add(fileEntry2)
        assert fileSystem.listFiles(NEW_DIR) as Set == [fileEntry1, fileEntry2] as Set

        // Write to the file to get a non-zero length
        final byte[] CONTENTS = "1234567890".getBytes()
        OutputStream out = fileEntry1.createOutputStream(false)
        out.write(CONTENTS)
        out.close()
        assert fileSystem.listFiles(NEW_DIR) as Set == [fileEntry1, fileEntry2] as Set

        def dirEntry3 = new DirectoryEntry(p(NEW_DIR, DIR1))
        fileSystem.add(dirEntry3)
        assert fileSystem.listFiles(NEW_DIR) as Set == [fileEntry1, fileEntry2, dirEntry3] as Set

        assert fileSystem.listFiles(NO_SUCH_DIR) == []

        shouldFailWithMessageContaining("path") { fileSystem.listFiles(null) }
    }

    void testListFiles_Wildcards() {
        def dirEntry = new DirectoryEntry(NEW_DIR)
        def fileEntry1 = new FileEntry(p(NEW_DIR, 'abc.txt'))
        def fileEntry2 = new FileEntry(p(NEW_DIR, 'def.txt'))

        fileSystem.add(dirEntry)
        fileSystem.add(fileEntry1)
        fileSystem.add(fileEntry2)

        assert fileSystem.listFiles(p(NEW_DIR, '*.txt')) as Set == [fileEntry1, fileEntry2] as Set
        assert fileSystem.listFiles(p(NEW_DIR, '*')) as Set == [fileEntry1, fileEntry2] as Set
        assert fileSystem.listFiles(p(NEW_DIR, '???.???')) as Set == [fileEntry1, fileEntry2] as Set
        assert fileSystem.listFiles(p(NEW_DIR, '*.exe')) as Set == [] as Set
        assert fileSystem.listFiles(p(NEW_DIR, 'abc.???')) as Set == [fileEntry1] as Set
        assert fileSystem.listFiles(p(NEW_DIR, 'a?c.?xt')) as Set == [fileEntry1] as Set
        assert fileSystem.listFiles(p(NEW_DIR, 'd?f.*')) as Set == [fileEntry2] as Set
    }

    void testDelete() {
        fileSystem.add(new FileEntry(NEW_FILE))
        assert fileSystem.delete(NEW_FILE)
        assert !fileSystem.exists(NEW_FILE)

        assert !fileSystem.delete(NO_SUCH_FILE)

        fileSystem.add(new DirectoryEntry(NEW_DIR))
        assert fileSystem.delete(NEW_DIR)
        assert !fileSystem.exists(NEW_DIR)

        fileSystem.add(new DirectoryEntry(NEW_DIR))
        fileSystem.add(new FileEntry(NEW_DIR + "/abc.txt"))

        assert !fileSystem.delete(NEW_DIR), "Directory containing files"
        assert fileSystem.exists(NEW_DIR)

        shouldFailWithMessageContaining("path") { fileSystem.delete(null) }
    }

    void testRename() {
        final FROM_FILE = NEW_FILE + "2"
        fileSystem.add(new FileEntry(FROM_FILE))

        fileSystem.rename(FROM_FILE, NEW_FILE)
        assert fileSystem.exists(NEW_FILE)

        fileSystem.add(new DirectoryEntry(NEW_DIR))

        // Rename existing directory
        final String TO_DIR = NEW_DIR + "2"
        fileSystem.rename(NEW_DIR, TO_DIR)
        assert !fileSystem.exists(NEW_DIR)
        assert fileSystem.exists(TO_DIR)
    }

    void testRename_ToPathFileAlreadyExists() {
        final FROM_FILE = EXISTING_FILE
        final String TO_FILE = NEW_FILE
        fileSystem.add(new FileEntry(TO_FILE))
         shouldThrowFileSystemExceptionWithMessageKey('filesystem.alreadyExists') {
             fileSystem.rename(FROM_FILE, TO_FILE) 
         }
    }

    void testRename_FromPathDoesNotExist() {
        final TO_FILE2 = NEW_FILE + "2"
        shouldThrowFileSystemExceptionWithMessageKey('filesystem.doesNotExist') {
            fileSystem.rename(NO_SUCH_FILE, TO_FILE2)
        }
        assert !fileSystem.exists(TO_FILE2), "After failed rename"
    }

    void testRename_ToPathIsChildOfFromPath() {
        final FROM_DIR = NEW_DIR
        final TO_DIR = FROM_DIR + "/child"
        fileSystem.add(new DirectoryEntry(FROM_DIR))
        shouldThrowFileSystemExceptionWithMessageKey('filesystem.renameFailed') {
            fileSystem.rename(FROM_DIR, TO_DIR)
        }
        assert !fileSystem.exists(TO_DIR), "After failed rename"
    }

    void testRename_EmptyDirectory() {
        final FROM_DIR = NEW_DIR
        final TO_DIR = FROM_DIR + "2"
        fileSystem.add(new DirectoryEntry(FROM_DIR))
        fileSystem.rename(FROM_DIR, TO_DIR)
        assert !fileSystem.exists(FROM_DIR)
        assert fileSystem.exists(TO_DIR)
    }

    void testRename_DirectoryContainsFiles() {
        fileSystem.add(new DirectoryEntry(NEW_DIR))
        fileSystem.add(new FileEntry(NEW_DIR + "/a.txt"))
        fileSystem.add(new FileEntry(NEW_DIR + "/b.txt"))
        fileSystem.add(new DirectoryEntry(NEW_DIR + "/subdir"))

        final String TO_DIR = NEW_DIR + "2"
        fileSystem.rename(NEW_DIR, TO_DIR)
        assert !fileSystem.exists(NEW_DIR)
        assert !fileSystem.exists(NEW_DIR + "/a.txt")
        assert !fileSystem.exists(NEW_DIR + "/b.txt")
        assert !fileSystem.exists(NEW_DIR + "/subdir")

        assert fileSystem.exists(TO_DIR)
        assert fileSystem.exists(TO_DIR + "/a.txt")
        assert fileSystem.exists(TO_DIR + "/b.txt")
        assert fileSystem.exists(TO_DIR + "/subdir")
    }

    void testRename_ParentOfToPathDoesNotExist() {
        final String FROM_FILE = NEW_FILE
        final String TO_FILE = fileSystem.path(NO_SUCH_DIR, "abc")
        fileSystem.add(new FileEntry(FROM_FILE))

        shouldThrowFileSystemExceptionWithMessageKey('filesystem.parentDirectoryDoesNotExist') {
            fileSystem.rename(FROM_FILE, TO_FILE)
        }
        assert fileSystem.exists(FROM_FILE)
        assert !fileSystem.exists(TO_FILE)
    }

    void testGetParent_Null() {
        shouldFailWithMessageContaining("path") { fileSystem.getParent(null) }
    }

    //-------------------------------------------------------------------------
    // Test setup
    //-------------------------------------------------------------------------

    void setUp() {
        super.setUp()
        fileSystem = createFileSystem()
    }

    //-------------------------------------------------------------------------
    // Helper Methods
    //-------------------------------------------------------------------------

    protected void shouldThrowFileSystemExceptionWithMessageKey(String messageKey, Closure closure) {
        def e = shouldThrow(FileSystemException, closure)
        assert e.messageKey == messageKey, "Expected message key [$messageKey], but was [${e.messageKey}]"
    }
    
    private verifyEntries(List expected, List actual) {
        expected.eachWithIndex {entry, index ->
            def entryStr = entry.toString()
            LOG.info("expected=$entryStr")
            assert actual.find {actualEntry -> actualEntry.toString() == entryStr }
        }
    }

    protected void assertSameIgnoringOrder(list1, list2) {
        LOG.info("Comparing $list1 to $list2")
        assert list1 as Set == list2 as Set, "list1=$list1  list2=$list2"
    }

    /**
     * Return a new instance of the FileSystem implementation class under test
     * @return a new FileSystem instance
     * @throws Exception
     */
    protected abstract FileSystem createFileSystem()

    /**
     * Verify the contents of the file at the specified path read from its InputSteam
     *
     * @param fileSystem - the FileSystem instance
     * @param expectedContents - the expected contents
     * @throws IOException
     */
    protected abstract void verifyFileContents(FileSystem fileSystem, String path, String contents) throws Exception

}