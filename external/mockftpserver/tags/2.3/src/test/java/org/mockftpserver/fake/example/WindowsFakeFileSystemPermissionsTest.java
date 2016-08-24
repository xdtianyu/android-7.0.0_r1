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
package org.mockftpserver.fake.example;

import org.apache.log4j.Logger;
import org.mockftpserver.fake.FakeFtpServer;
import org.mockftpserver.fake.filesystem.DirectoryEntry;
import org.mockftpserver.fake.filesystem.FileEntry;
import org.mockftpserver.fake.filesystem.FileSystem;
import org.mockftpserver.fake.filesystem.Permissions;
import org.mockftpserver.fake.filesystem.WindowsFakeFileSystem;
import org.mockftpserver.test.*;
import org.mockftpserver.test.AbstractTestCase;

/**
 * Example code illustrating how to programmatically configure a FakeFtpServer with a (simulated) Windows
 * filesystem, and including file/directory permissions.
 */
public class WindowsFakeFileSystemPermissionsTest extends AbstractTestCase implements IntegrationTest {

    private static final Logger LOG = Logger.getLogger(WindowsFakeFileSystemPermissionsTest.class);

    public void testFilesystemWithPermissions() throws Exception {

        final String USER1 = "joe";
        final String USER2 = "mary";
        final String GROUP = "dev";
        final String CONTENTS = "abcdef 1234567890";

        FileSystem fileSystem = new WindowsFakeFileSystem();
        DirectoryEntry directoryEntry1 = new DirectoryEntry("c:\\");
        directoryEntry1.setPermissions(new Permissions("rwxrwx---"));
        directoryEntry1.setOwner(USER1);
        directoryEntry1.setGroup(GROUP);

        DirectoryEntry directoryEntry2 = new DirectoryEntry("c:\\data");
        directoryEntry2.setPermissions(Permissions.ALL);
        directoryEntry2.setOwner(USER1);
        directoryEntry2.setGroup(GROUP);

        FileEntry fileEntry1 = new FileEntry("c:\\data\\file1.txt", CONTENTS);
        fileEntry1.setPermissionsFromString("rw-rw-rw-");
        fileEntry1.setOwner(USER1);
        fileEntry1.setGroup(GROUP);

        FileEntry fileEntry2 = new FileEntry("c:\\data\\run.exe");
        fileEntry2.setPermissionsFromString("rwxrwx---");
        fileEntry2.setOwner(USER2);
        fileEntry2.setGroup(GROUP);

        fileSystem.add(directoryEntry1);
        fileSystem.add(directoryEntry2);
        fileSystem.add(fileEntry1);
        fileSystem.add(fileEntry2);

        FakeFtpServer fakeFtpServer = new FakeFtpServer();
        fakeFtpServer.setFileSystem(fileSystem);

        LOG.info(fileSystem);
    }

}