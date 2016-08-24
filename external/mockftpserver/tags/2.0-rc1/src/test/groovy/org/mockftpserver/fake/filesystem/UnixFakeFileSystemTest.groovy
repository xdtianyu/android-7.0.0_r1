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
 * Tests for UnixFakeFileSystem.
 *
 * @version $Revision$ - $Date$
 *
 * @author Chris Mair
 */
class UnixFakeFileSystemTest extends AbstractFakeFileSystemTest {

    private static final String SEP = "/"

    UnixFakeFileSystemTest() {
        // These need to be set in the constructor because these values are used in setUp()
        NEW_DIR = SEP + NEW_DIRNAME
        NEW_FILE = "/NewFile.txt"
        EXISTING_DIR = "/"
        EXISTING_FILE = "/ExistingFile.txt"
        NO_SUCH_DIR = "/xx/yy"
        NO_SUCH_FILE = "/xx/yy/zz.txt"
    }


    void testListNames_FromRoot() {
        final DIR = '/'
        final FILENAME = 'abc.txt'
        final FILE = p(DIR, FILENAME)

        assert !fileSystem.exists(FILE)
        fileSystem.add(new FileEntry(FILE))
        def names = fileSystem.listNames(DIR)
        assert names.find { it == FILENAME }
    }

    void testPath() {
        assert fileSystem.path(null, null) == ""
        assert fileSystem.path(null, "abc") == "abc"
        assert fileSystem.path("abc", null) == "abc"
        assert fileSystem.path("", "") == ""
        assert fileSystem.path("", "abc") == "abc"
        assert fileSystem.path("abc", "") == "abc"
        assert fileSystem.path("abc", "DEF") == "abc/DEF"
        assert fileSystem.path("abc/", "def") == "abc/def"
        assert fileSystem.path("/abc/", "def") == "/abc/def"
        assert fileSystem.path("/ABC", "/def") == "/ABC/def"
        assert fileSystem.path("abc", "/def") == "abc/def"
    }

    void testNormalize() {
        assert fileSystem.normalize("/") == "/"
        assert fileSystem.normalize("/aBc") == "/aBc"
        assert fileSystem.normalize("/abc/DEF") == "/abc/DEF"
        assert fileSystem.normalize("/Abc/def/..") == "/Abc"
        assert fileSystem.normalize("/abc/def/../ghi") == "/abc/ghi"
        assert fileSystem.normalize("/abc/def/.") == "/abc/def"
        assert fileSystem.normalize("/abc/def/./gHI") == "/abc/def/gHI"
    }

    void testGetName() {
        assert fileSystem.getName("/") == ""
        assert fileSystem.getName("/aBC") == "aBC"
        assert fileSystem.getName("/abc/def") == "def"
        assert fileSystem.getName("/abc/def/../GHI") == "GHI"
    }

    public void testGetParent() {
        assert fileSystem.getParent("/") == null
        assert fileSystem.getParent("/abc") == "/"
        assert fileSystem.getParent("/abc/def") == "/abc"
    }

    void testIsValidName() {
        ["/abc",
                "/ABC/def",
                "/abc/d!ef",
                "/abc/DEF/h(ij)!@#\$%^&*()-_+=~`,.<>?;:[]{}\\|abc",
        ].each {
            assert fileSystem.isValidName(it), "[$it]"
        }

        ["",
                "abc",
                "abc/def",
                "a:/abc:",
                "//a*bc",
                "C:/?abc",
        ].each {
            assert !fileSystem.isValidName(it), "[$it]"
        }
    }

    void testIsAbsolute() {
        assert fileSystem.isAbsolute("/")
        assert fileSystem.isAbsolute("/abc")

        assert !fileSystem.isAbsolute("abc")
        assert !fileSystem.isAbsolute("c:\\usr")

        shouldFailWithMessageContaining("path") { fileSystem.isAbsolute(null) }
    }

    //-----------------------------------------------------------------------------------
    // Helper Methods
    //-----------------------------------------------------------------------------------

    /**
     * Return a new instance of the FileSystem implementation class under test
     * @return a new FileSystem instance
     */
    protected FileSystem createFileSystem() {
        UnixFakeFileSystem fs = new UnixFakeFileSystem()
        fs.add(new DirectoryEntry(EXISTING_DIR))
        fs.add(new FileEntry(EXISTING_FILE, EXISTING_FILE_CONTENTS))
        assert fs.createParentDirectoriesAutomatically
        fs.createParentDirectoriesAutomatically = false
        return fs
    }

    protected Class getExpectedDirectoryListingFormatterClass() {
        return UnixDirectoryListingFormatter
    }

}
