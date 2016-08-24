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

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.mockftpserver.core.util.IoUtil

/**
 * Tests for FileEntry
 *
 * @version $Revision$ - $Date$
 *
 * @author Chris Mair
 */
public class FileEntryTest extends AbstractFileSystemEntryTestCase {

    private static final LOG = LoggerFactory.getLogger(FileEntryTest)
    private static final CONTENTS = "abc 123 %^& xxx"

    private FileEntry entry

    void testConstructorWithStringContents() {
        entry = new FileEntry(PATH, CONTENTS)
        verifyContents(CONTENTS)
    }

    void testSettingContentsFromString() {
        entry.setContents(CONTENTS)
        verifyContents(CONTENTS)
    }

    void testSettingContentsFromBytes() {
        byte[] contents = CONTENTS.getBytes()
        entry.setContents(contents)
        // Now corrupt the original byte array to make sure the file entry is not affected
        contents[1] = (byte) '#'
        verifyContents(CONTENTS)
    }

    void testSetContents_BytesNotInCharSet() {
        byte[] contents = [65, -99, 91, -115] as byte[]
        entry.setContents(contents)
        verifyContents(contents)
    }

    void testSetContents_NullString() {
        entry.setContents((String) null)
        assert entry.size == 0
    }

    void testSetContents_NullBytes() {
        entry.setContents((byte[]) null)
        assert entry.size == 0
    }

    void testCreateOutputStream() {
        // New, empty file
        OutputStream out = entry.createOutputStream(false)
        out.write(CONTENTS.getBytes())
        verifyContents(CONTENTS)

        // Another OutputStream, append=false
        out = entry.createOutputStream(false)
        out.write(CONTENTS.getBytes())
        verifyContents(CONTENTS)

        // Another OutputStream, append=true
        out = entry.createOutputStream(true)
        out.write(CONTENTS.getBytes())
        verifyContents(CONTENTS + CONTENTS)

        // Set contents directly
        final String NEW_CONTENTS = ",./'\t\r[]-\n="
        entry.setContents(NEW_CONTENTS)
        verifyContents(NEW_CONTENTS)

        // New OutputStream, append=true (so should append to contents we set directly)
        out = entry.createOutputStream(true)
        out.write(CONTENTS.getBytes())
        verifyContents(NEW_CONTENTS + CONTENTS)

        // Yet another OutputStream, append=true (so should append to accumulated contents)
        OutputStream out2 = entry.createOutputStream(true)
        out2.write(CONTENTS.getBytes())
        out2.close()       // should have no effect
        verifyContents(NEW_CONTENTS + CONTENTS + CONTENTS)

        // Write with the previous OutputStream (simulate 2 OututStreams writing "concurrently")
        out.write(NEW_CONTENTS.getBytes())
        verifyContents(NEW_CONTENTS + CONTENTS + CONTENTS + NEW_CONTENTS)
    }

    void testCreateInputStream_NullContents() {
        verifyContents("")
    }

    void testCloneWithNewPath() {
        entry.lastModified = LAST_MODIFIED
        entry.owner = USER
        entry.group = GROUP
        entry.permissions = PERMISSIONS
        entry.setContents('abc')
        def clone = entry.cloneWithNewPath(NEW_PATH)

        assert !clone.is(entry)
        assert clone.path == NEW_PATH
        assert clone.lastModified == LAST_MODIFIED
        assert clone.owner == USER
        assert clone.group == GROUP
        assert clone.permissions == PERMISSIONS
        assert clone.createInputStream().text == 'abc'
        assert !clone.directory
    }

    void testCloneWithNewPath_WriteToOutputStream() {
        def outputStream = entry.createOutputStream(false)
        outputStream.withWriter { writer -> writer.write('ABCDEF') }
        def clone = entry.cloneWithNewPath(NEW_PATH)

        assert !clone.is(entry)
        assert clone.path == NEW_PATH
        assert clone.createInputStream().text == 'ABCDEF' 
        assert !clone.directory
    }

//    void testEquals() {
//        assert entry.equals(entry)
//        assert entry.equals(new FileEntry(path:PATH, lastModified:LAST_MODIFIED))
//        assert entry.equals(new FileEntry(path:PATH, lastModified:new Date())) // lastModified ignored
//
//        assert !entry.equals(new FileEntry("xyz", lastModified:LAST_MODIFIED))
//        assert !entry.equals(new FileEntry(path:PATH, contents:'abc', lastModified:LAST_MODIFIED))
//        assert !entry.equals("ABC")
//        assert !entry.equals(null)
//    }
//
//    void testHashCode() {
//        assert entry.hashCode() == entry.hashCode()
//        assert entry.hashCode() == new FileEntry(path:PATH, contents:'abc', lastModified:LAST_MODIFIED).hashCode()
//        assert entry.hashCode() == new FileEntry(path:PATH, contents:'abc', new Date()).hashCode()  // lastModified ignored
//
//        assert entry.hashCode() != new FileEntry(path:PATH, contents:'abc', lastModified:LAST_MODIFIED).hashCode()
//        assert entry.hashCode() != new FileEntry(path:PATH, contents:'abcdef', lastModified:LAST_MODIFIED).hashCode()
//
//        assert entry.hashCode() == new DirectoryEntry(path:PATH, lastModified:LAST_MODIFIED).hashCode()
//    }

    //-------------------------------------------------------------------------
    // Implementation of Required Abstract Methods
    //-------------------------------------------------------------------------

    /**
     * @see org.mockftpserver.fake.filesystem.AbstractFileSystemEntryTestCase#getImplementationClass()
     */
    protected Class getImplementationClass() {
        return FileEntry.class
    }

    /**
     * @see org.mockftpserver.fake.filesystem.AbstractFileSystemEntryTestCase#isDirectory()
     */
    protected boolean isDirectory() {
        return false
    }

    //-------------------------------------------------------------------------
    // Test setup
    //-------------------------------------------------------------------------

    void setUp() {
        super.setUp()
        entry = new FileEntry(PATH)
    }

    //-------------------------------------------------------------------------
    // Internal Helper Methods
    //-------------------------------------------------------------------------

    /**
     * Verify the expected contents of the file entry, read from its InputSteam
     * @param expectedContents - the expected contents, as a String
     * @throws IOException
     */
    private void verifyContents(String expectedContents) {
        LOG.info("expectedContents=$expectedContents")
        verifyContents(expectedContents.bytes)
    }

    /**
     * Verify the expected contents of the file entry, read from its InputSteam
     * @param expectedContents - the expected contents, as a byte[]
     * @throws IOException
     */
    private void verifyContents(byte[] expectedContents) {
        byte[] bytes = IoUtil.readBytes(entry.createInputStream())
        def bytesAsList = bytes as List
        LOG.info("bytes=$bytesAsList")
        assert bytes == expectedContents, "actual=$bytesAsList  expected=${expectedContents as byte[]}"
        assert entry.getSize() == expectedContents.length
    }

}
