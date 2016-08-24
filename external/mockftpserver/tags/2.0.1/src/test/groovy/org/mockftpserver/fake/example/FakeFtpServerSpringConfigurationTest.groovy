package org.mockftpserver.fake.example

import org.apache.commons.net.ftp.FTPClient
import org.apache.commons.net.ftp.FTPFile
import org.mockftpserver.fake.FakeFtpServer
import org.mockftpserver.test.AbstractGroovyTest
import org.springframework.context.ApplicationContext
import org.springframework.context.support.ClassPathXmlApplicationContext

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
class FakeFtpServerSpringConfigurationTest extends AbstractGroovyTest {

    static final SERVER = "localhost"
    static final PORT = 9981
    static final USERNAME = 'joe'           // Must match Spring config
    static final PASSWORD = 'password'      // Must match Spring config 

    private FakeFtpServer fakeFtpServer
    private FTPClient ftpClient

    void testFakeFtpServer_Unix() {
        startFtpServer('fakeftpserver-beans.xml')
        connectAndLogin()

        // PWD
        String dir = ftpClient.printWorkingDirectory()
        assert dir == '/'

        // LIST
        FTPFile[] files = ftpClient.listFiles()
        LOG.info("FTPFile[0]=" + files[0])
        assert files.length == 1

        // RETR
        ByteArrayOutputStream outputStream = new ByteArrayOutputStream()
        assert ftpClient.retrieveFile("File.txt", outputStream)
        LOG.info("File contents=[" + outputStream.toString() + "]")
    }

    void testFakeFtpServer_Windows_WithPermissions() {
        startFtpServer('fakeftpserver-permissions-beans.xml')
        connectAndLogin()

        // PWD
        String dir = ftpClient.printWorkingDirectory()
        assert dir == 'c:\\'

        // LIST
        FTPFile[] files = ftpClient.listFiles()
        assert files.length == 2
        LOG.info("FTPFile[0]=" + files[0])
        LOG.info("FTPFile[1]=" + files[1])

        // RETR - File1.txt; we have required permissions
        ByteArrayOutputStream outputStream = new ByteArrayOutputStream()
        assert ftpClient.retrieveFile("File1.txt", outputStream)
        LOG.info("File contents=[" + outputStream.toString() + "]")

        // RETR - File2.txt; we DO NOT have required permissions
        outputStream = new ByteArrayOutputStream()
        assert !ftpClient.retrieveFile("File2.txt", outputStream)
        assert ftpClient.replyCode == 550
    }

    void setUp() {
        super.setUp()
        ftpClient = new FTPClient()
    }

    void tearDown() {
        super.tearDown()
        fakeFtpServer?.stop()
    }

    private void startFtpServer(String springConfigFile) {
        ApplicationContext context = new ClassPathXmlApplicationContext(springConfigFile)
        fakeFtpServer = (FakeFtpServer) context.getBean("fakeFtpServer")
        fakeFtpServer.start()
    }

    private void connectAndLogin() {
        ftpClient.connect(SERVER, PORT)
        assert ftpClient.login(USERNAME, PASSWORD)
    }

}