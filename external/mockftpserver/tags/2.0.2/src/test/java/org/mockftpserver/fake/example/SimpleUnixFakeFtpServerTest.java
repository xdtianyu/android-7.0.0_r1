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

import org.mockftpserver.fake.FakeFtpServer;
import org.mockftpserver.fake.UserAccount;
import org.mockftpserver.fake.filesystem.DirectoryEntry;
import org.mockftpserver.fake.filesystem.FileEntry;
import org.mockftpserver.fake.filesystem.FileSystem;
import org.mockftpserver.fake.filesystem.UnixFakeFileSystem;
import org.mockftpserver.test.AbstractTest;
import org.mockftpserver.test.IntegrationTest;

/**
 * Example code illustrating how to programmatically configure a FakeFtpServer with a (simulated) Unix
 * filesystem.
 */
public class SimpleUnixFakeFtpServerTest extends AbstractTest implements IntegrationTest {

    public void testConfigureAndStart() throws Exception {
        FakeFtpServer fakeFtpServer = new FakeFtpServer();
        fakeFtpServer.setServerControlPort(9981);
        fakeFtpServer.addUserAccount(new UserAccount("user", "password", "c:\\data"));

        FileSystem fileSystem = new UnixFakeFileSystem();
        fileSystem.add(new DirectoryEntry("/data"));
        fileSystem.add(new FileEntry("/data/file1.txt", "abcdef 1234567890"));
        fileSystem.add(new FileEntry("/data/run.exe"));
        fakeFtpServer.setFileSystem(fileSystem);

        fakeFtpServer.start();

        fakeFtpServer.stop();
    }

}