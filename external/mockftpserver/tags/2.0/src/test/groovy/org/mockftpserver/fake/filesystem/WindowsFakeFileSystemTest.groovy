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

import org.mockftpserver.fake.filesystem.AbstractFakeFileSystemTest

/**
 * Tests for WindowsFakeFileSystem.
 *
 * @version $Revision$ - $Date$
 *
 * @author Chris Mair
 */
class WindowsFakeFileSystemTest extends AbstractFakeFileSystemTest {

    private static final String SEP = "\\"

    WindowsFakeFileSystemTest() {
        // These need to be set in the constructor because these values are used in setUp()
        NEW_DIR = "d:/" + NEW_DIRNAME
        NEW_FILE = "d:/NewFile.txt"
        EXISTING_DIR = "d:/"
        EXISTING_FILE = "d:/ExistingFile.txt"
        NO_SUCH_DIR = 'x:/xx/yy'
        NO_SUCH_FILE = "x:/xx/yy/zz.txt"
    }

    // -------------------------------------------------------------------------
    // Tests
    // -------------------------------------------------------------------------

    void testOtherRoots() {
        final String X = "x:/"
        final String Y = "y:\\"
        assertFalse(X, fileSystem.exists(X))
        assertFalse(Y, fileSystem.exists(Y))

        fileSystem.add(new DirectoryEntry(X))
        fileSystem.add(new DirectoryEntry(Y))

        assertTrue(X, fileSystem.exists(X))
        assertTrue(Y, fileSystem.exists(Y))
    }

    void testPath() {
        assert fileSystem.path(null, null) == ""
        assert fileSystem.path(null, "abc") == "abc"
        assert fileSystem.path("abc", null) == "abc"
        assert fileSystem.path("", "") == ""
        assert fileSystem.path("", "abc") == "abc"
        assert fileSystem.path("abc", "") == "abc"
        assert fileSystem.path("abc", "def") == "abc" + SEP + "def"
        assert fileSystem.path("abc\\", "def") == "abc\\def"
        assert fileSystem.path("c:/abc/", "def") == "c:/abc/def"
        assert fileSystem.path("d:\\abc", "\\def") == "d:\\abc\\def"
        assert fileSystem.path("abc", "/def") == "abc/def"
    }

    void testNormalize() {
        assert fileSystem.normalize("a:\\") == "a:\\"
        assert fileSystem.normalize("a:/") == "a:\\"
        assert fileSystem.normalize("b:/abc") == path("b:", "abc")
        assert fileSystem.normalize("c:\\abc\\def") == path("c:", "abc", "def")
        assert fileSystem.normalize("d:/abc/def") == path("d:", "abc", "def")
        assert fileSystem.normalize("e:\\abc/def/..") == path("e:", "abc")
        assert fileSystem.normalize("f:/abc/def/../ghi") == path("f:", "abc", "ghi")
        assert fileSystem.normalize("g:\\abc\\def\\.") == path("g:", "abc", "def")
        assert fileSystem.normalize("h:/abc\\def\\./ghi") == path("h:", "abc", "def", "ghi")
        assert fileSystem.normalize("c:\\abc").toLowerCase() == path("c:", "abc")
        assert fileSystem.normalize("c:/abc").toLowerCase() == path("c:", "abc")
        assert fileSystem.normalize("z:/abc").toLowerCase() == path("z:", "abc")
    }

    void testGetName() {
        assert fileSystem.getName("l:\\") == ""
        assert fileSystem.getName("m:\\abc") == "abc"
        assert fileSystem.getName("n:/abc\\def") == "def"
        assert fileSystem.getName("o:/abc/def") == "def"
    }

    public void testGetParent() {
        assert fileSystem.getParent("p:/") == null
        assert fileSystem.getParent("q:\\abc") == "q:\\"
        assert fileSystem.getParent("r:/abc\\def") == path("r:", "abc")
        assert fileSystem.getParent("s:\\abc/def") == path("s:", "abc")
    }

    void testIsValidName() {
        // \/:*?"<>|
        ["a:\\abc",
                "c:/abc",
                "d:/abc\\def",
                "e:/abc\\d!ef",
                "f:\\abc\\def\\h(ij)",
                "g:\\abc",
                "z:/abc/def",
                "\\\\shared"
        ].each {
            assert fileSystem.isValidName(it), "[$it]"
        }

        ["",
                "abc",
                "abc/def",
                "a:/abc:",
                "B:\\a*bc",
                "C:/?abc",
                "D:\\abc/<def",
                "E:/abc/def>",
                "aa:\\abc",
                "X:X:/abc",
                ":\\abc\\def",
                "X:\\\\abc"
        ].each {
            assert !fileSystem.isValidName(it), "[$it]"
        }
    }

    void testIsAbsolute() {
        assert fileSystem.isAbsolute("c:\\")
        assert fileSystem.isAbsolute("x:\\Documents")
        assert fileSystem.isAbsolute("a:/")
        assert fileSystem.isAbsolute("\\\\shared\\docs")

        assert !fileSystem.isAbsolute("abc")
        assert !fileSystem.isAbsolute("/usr")
        assert !fileSystem.isAbsolute("c:usr")

        shouldFailWithMessageContaining("path") { fileSystem.isAbsolute(null) }
    }

    void testCaseInsensitive() {
        def fileEntry = fileSystem.getEntry(EXISTING_FILE)
        assert fileEntry
        assert fileEntry == fileSystem.getEntry(EXISTING_FILE.toLowerCase())
    }

    //-------------------------------------------------------------------------
    // Test setup
    //-------------------------------------------------------------------------

    void setUp() {
        super.setUp()
    }

    protected Class getExpectedDirectoryListingFormatterClass() {
        return WindowsDirectoryListingFormatter
    }

    //-----------------------------------------------------------------------------------
    // Helper Methods
    //-----------------------------------------------------------------------------------

    /**
     * Return a new instance of the FileSystem implementation class under test
     *
     * @return a new FileSystem instance
     */
    protected FileSystem createFileSystem() {
        WindowsFakeFileSystem fs = new WindowsFakeFileSystem()
        fs.add(new DirectoryEntry(EXISTING_DIR))
        fs.add(new FileEntry(EXISTING_FILE, EXISTING_FILE_CONTENTS))
        fs.createParentDirectoriesAutomatically = false
        return fs
    }

    /**
     * Return the specified paths concatenated with the system-dependent separator in between
     * @param p1 - the first path
     * @param p2 - the second path
     * @return p1 + SEPARATOR + p2
     */
    private String path(String[] paths) {
        return paths.join(SEP)
    }
}
