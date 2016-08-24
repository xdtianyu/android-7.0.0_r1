/*
 * Copyright 2007 the original author or authors.
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
package org.mockftpserver.stub.example;

import org.mockftpserver.core.command.CommandNames;
import org.mockftpserver.stub.StubFtpServer;
import org.mockftpserver.stub.command.PwdCommandHandler;
import org.mockftpserver.test.AbstractTestCase;
import org.mockftpserver.test.IntegrationTest;

/**
 * Example test using StubFtpServer, with programmatic configuration.
 */
public class FtpWorkingDirectoryTest extends AbstractTestCase implements IntegrationTest {

    private static final int PORT = 9981;
    private FtpWorkingDirectory ftpWorkingDirectory;
    private StubFtpServer stubFtpServer;
    
    /**
     * Test FtpWorkingDirectory getWorkingDirectory() method 
     */
    public void testGetWorkingDirectory() throws Exception {
        
        // Replace the existing (default) CommandHandler; customize returned directory pathname
        final String DIR = "some/dir";
        PwdCommandHandler pwdCommandHandler = new PwdCommandHandler();
        pwdCommandHandler.setDirectory(DIR);
        stubFtpServer.setCommandHandler(CommandNames.PWD, pwdCommandHandler);
        
        stubFtpServer.start();
        
        String workingDir = ftpWorkingDirectory.getWorkingDirectory();

        assertEquals("workingDirectory", DIR, workingDir);
    }

    /**
     * @see org.mockftpserver.test.AbstractTestCase#setUp()
     */
    protected void setUp() throws Exception {
        super.setUp();
        ftpWorkingDirectory = new FtpWorkingDirectory();
        ftpWorkingDirectory.setPort(PORT);
        stubFtpServer = new StubFtpServer();
        stubFtpServer.setServerControlPort(PORT);
    }

    /**
     * @see org.mockftpserver.test.AbstractTestCase#tearDown()
     */
    protected void tearDown() throws Exception {
        super.tearDown();
        stubFtpServer.stop();
    }

}
