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

/**
 * Tests for DirectoryEntry
 *
 * @version $Revision$ - $Date$
 *
 * @author Chris Mair
 */
public class DirectoryEntryTest extends AbstractFileSystemEntryTestCase {

    private DirectoryEntry entry

    void testCloneWithNewPath() {
        entry.lastModified = LAST_MODIFIED
        entry.owner = USER
        entry.group = GROUP
        entry.permissions = PERMISSIONS
        def clone = entry.cloneWithNewPath(NEW_PATH)

        assert !clone.is(entry)
        assert clone.path == NEW_PATH
        assert clone.lastModified == LAST_MODIFIED
        assert clone.owner == USER
        assert clone.group == GROUP
        assert clone.permissions == PERMISSIONS
        assert clone.size == 0
        assert clone.directory
    }

    /**
     * @see org.mockftpserver.fake.filesystem.AbstractFileSystemEntryTestCase#getImplementationClass()
     */
    protected Class getImplementationClass() {
        return DirectoryEntry.class
    }

    /**
     * @see org.mockftpserver.fake.filesystem.AbstractFileSystemEntryTestCase#isDirectory()
     */
    protected boolean isDirectory() {
        return true
    }

    void setUp() {
        super.setUp()
        entry = new DirectoryEntry(PATH)
    }

}
