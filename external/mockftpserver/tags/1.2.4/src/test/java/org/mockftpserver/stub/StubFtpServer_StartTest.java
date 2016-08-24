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
package org.mockftpserver.stub;

import org.apache.commons.net.ftp.FTPClient;
import org.mockftpserver.core.command.CommandNames;
import org.mockftpserver.stub.StubFtpServer;
import org.mockftpserver.stub.command.PwdCommandHandler;
import org.mockftpserver.test.AbstractTest;
import org.mockftpserver.test.PortTestUtil;

/**
 * Tests for StubFtpServer that require the StubFtpServer thread to be started. 
 * 
 * @version $Revision$ - $Date$
 * 
 * @author Chris Mair
 */
public final class StubFtpServer_StartTest extends AbstractTest {

    private static final String SERVER = "localhost";

    private StubFtpServer stubFtpServer;
    
    /**
     * Test the start() and stop() methods. Start the StubFtpServer and then stop it immediately. 
     */
    public void testStartAndStop() throws Exception {
        stubFtpServer = new StubFtpServer();
        stubFtpServer.setServerControlPort(PortTestUtil.getFtpServerControlPort());
        assertEquals("started - before", false, stubFtpServer.isStarted());
        
        stubFtpServer.start();
        Thread.sleep(200L);     // give it some time to get started
        assertEquals("started - after start()", true, stubFtpServer.isStarted());
        assertEquals("shutdown - after start()", false, stubFtpServer.isShutdown());

        stubFtpServer.stop();
        
        assertEquals("shutdown - after stop()", true, stubFtpServer.isShutdown());
    }
    
    /**
     * Test setting a non-default port number for the StubFtpServer control connection socket. 
     */
    public void testCustomServerControlPort() throws Exception {
        final int SERVER_CONTROL_PORT = 9187;
        final String DIR = "abc 1234567";
        PwdCommandHandler pwd = new PwdCommandHandler();
        pwd.setDirectory(DIR);
        
        stubFtpServer = new StubFtpServer();
        stubFtpServer.setServerControlPort(SERVER_CONTROL_PORT);
        stubFtpServer.setCommandHandler(CommandNames.PWD, pwd);
        
        stubFtpServer.start();

        try {
            FTPClient ftpClient = new FTPClient();
            ftpClient.connect(SERVER, SERVER_CONTROL_PORT);
            
            assertEquals("pwd", DIR, ftpClient.printWorkingDirectory());
        }
        finally {
            stubFtpServer.stop();
        }
    }
    
}
