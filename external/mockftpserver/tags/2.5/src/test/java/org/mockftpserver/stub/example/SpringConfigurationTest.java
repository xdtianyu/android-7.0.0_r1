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

import org.apache.commons.net.ftp.FTPClient;
import org.apache.commons.net.ftp.FTPFile;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.mockftpserver.stub.StubFtpServer;
import org.mockftpserver.test.AbstractTestCase;
import org.springframework.context.ApplicationContext;
import org.springframework.context.support.ClassPathXmlApplicationContext;

import java.io.ByteArrayOutputStream;

/**
 * Example test for StubFtpServer, using the Spring Framework ({@link http://www.springframework.org/}) 
 * for configuration.
 */
public class SpringConfigurationTest extends AbstractTestCase {

    private static final Logger LOG = LoggerFactory.getLogger(SpringConfigurationTest.class);
    private static final String SERVER = "localhost";
    private static final int PORT = 9981;

    private StubFtpServer stubFtpServer;
    private FTPClient ftpClient;
    
    /**
     * Test starting the StubFtpServer configured within the example Spring configuration file 
     */
    public void testStubFtpServer() throws Exception {
        stubFtpServer.start();
        
        ftpClient.connect(SERVER, PORT);

        // PWD
        String dir = ftpClient.printWorkingDirectory();
        assertEquals("PWD", "foo/bar", dir);
        
        // LIST
        FTPFile[] files = ftpClient.listFiles();
        LOG.info("FTPFile[0]=" + files[0]);
        LOG.info("FTPFile[1]=" + files[1]);
        assertEquals("number of files from LIST", 2, files.length);
        
        // DELE
        assertFalse("DELE", ftpClient.deleteFile("AnyFile.txt"));
        
        // RETR
        ByteArrayOutputStream outputStream = new ByteArrayOutputStream();
        assertTrue(ftpClient.retrieveFile("SomeFile.txt", outputStream));
        LOG.info("File contents=[" + outputStream.toString() + "]");
    }

    /**
     * @see org.mockftpserver.test.AbstractTestCase#setUp()
     */
    protected void setUp() throws Exception {
        super.setUp();
        
        ApplicationContext context = new ClassPathXmlApplicationContext("stubftpserver-beans.xml");
        stubFtpServer = (StubFtpServer) context.getBean("stubFtpServer");

        ftpClient = new FTPClient();
    }

    /**
     * @see org.mockftpserver.test.AbstractTestCase#tearDown()
     */
    protected void tearDown() throws Exception {
        super.tearDown();
        stubFtpServer.stop();
    }

}
