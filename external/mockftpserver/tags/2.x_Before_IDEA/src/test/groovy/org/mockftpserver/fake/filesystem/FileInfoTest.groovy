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

import org.mockftpserver.test.AbstractGroovyTest

/**
 * Tests for FileInfo
 * 
 * @version $Revision: $ - $Date: $
 *
 * @author Chris Mair
 */
public final class FileInfoTest extends AbstractGroovyTest {

    private static final String NAME = "def.txt"
    private static final long SIZE = 1234567L
    private static final Date LAST_MODIFIED = new Date()

    private FileInfo fileInfoFile
    private FileInfo fileInfoDirectory

    /**
     * Test the forFile() constructor
     */
    void testFileConstructor() {
        assert fileInfoFile.isDirectory() == false
        assert fileInfoFile.getName() == NAME
        assert fileInfoFile.getSize() == SIZE
        assert fileInfoFile.lastModified == LAST_MODIFIED
    }
    
    /**
     * Test the forDirectory() constructor
     */
    void testDirectoryConstructor() {
        assert fileInfoDirectory.isDirectory()
        assert fileInfoDirectory.getName() == NAME
        assert fileInfoDirectory.getSize() == 0
        assert fileInfoDirectory.lastModified == LAST_MODIFIED
    }
    
    /**
     * Test the equals() method 
     */
    void testEquals() {
        assert fileInfoFile.equals(fileInfoFile)
        assert fileInfoFile.equals(FileInfo.forFile(NAME, SIZE, LAST_MODIFIED))
        assert fileInfoFile.equals(FileInfo.forFile(NAME, SIZE, new Date())) // lastModified ignored

        assert !fileInfoFile.equals(FileInfo.forFile("xyz", SIZE, LAST_MODIFIED))
        assert !fileInfoFile.equals(FileInfo.forFile(NAME, 999L, LAST_MODIFIED))
        assert !fileInfoFile.equals("ABC")
        assert !fileInfoFile.equals(null)
    }
    
    /**
     * Test the hashCode() method 
     */
    void testHashCode() {
        assert fileInfoFile.hashCode() == fileInfoFile.hashCode()
        assert fileInfoFile.hashCode() == FileInfo.forFile(NAME, SIZE, LAST_MODIFIED).hashCode()
        assert fileInfoFile.hashCode() == FileInfo.forFile(NAME, SIZE, new Date()).hashCode()  // lastModified ignored
        
        assert fileInfoFile.hashCode() != FileInfo.forFile("xyz", SIZE, LAST_MODIFIED).hashCode()
        assert fileInfoFile.hashCode() != FileInfo.forFile(NAME, 33, LAST_MODIFIED).hashCode()
        
        assert fileInfoDirectory.hashCode() == FileInfo.forDirectory(NAME, LAST_MODIFIED).hashCode()
    }
    
    /**
     * Test the toString() method 
     */
    void testToString() {
        String toString = fileInfoFile.toString() 
        assert toString.contains(NAME)
        assert toString.contains(Long.toString(SIZE))
        assert toString.contains(LAST_MODIFIED.toString())
    }

    /**
     * @see org.mockftpserver.test.AbstractTest#setUp()
     */
    void setUp() {
        super.setUp()
        fileInfoFile = FileInfo.forFile(NAME, SIZE, LAST_MODIFIED)
        fileInfoDirectory = FileInfo.forDirectory(NAME, LAST_MODIFIED)
    }
}
