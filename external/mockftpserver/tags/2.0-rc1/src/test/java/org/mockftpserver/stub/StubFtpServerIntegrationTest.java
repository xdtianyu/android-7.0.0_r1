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

import org.apache.commons.net.ftp.FTP;
import org.apache.commons.net.ftp.FTPClient;
import org.apache.commons.net.ftp.FTPFile;
import org.apache.log4j.Logger;
import org.mockftpserver.core.command.CommandHandler;
import org.mockftpserver.core.command.CommandNames;
import org.mockftpserver.core.command.InvocationRecord;
import org.mockftpserver.core.command.SimpleCompositeCommandHandler;
import org.mockftpserver.core.command.StaticReplyCommandHandler;
import org.mockftpserver.stub.command.*;
import org.mockftpserver.test.AbstractTest;
import org.mockftpserver.test.IntegrationTest;
import org.mockftpserver.test.PortTestUtil;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;

/**
 * Tests for StubFtpServer using the Apache Jakarta Commons Net FTP client.
 *
 * @author Chris Mair
 * @version $Revision$ - $Date$
 */
public final class StubFtpServerIntegrationTest extends AbstractTest implements IntegrationTest {

    private static final Logger LOG = Logger.getLogger(StubFtpServerIntegrationTest.class);
    private static final String SERVER = "localhost";
    private static final String USERNAME = "user123";
    private static final String PASSWORD = "password";
    private static final String FILENAME = "abc.txt";
    private static final String ASCII_CONTENTS = "abcdef\tghijklmnopqr";
    private static final byte[] BINARY_CONTENTS = new byte[256];

    private StubFtpServer stubFtpServer;
    private FTPClient ftpClient;
    private RetrCommandHandler retrCommandHandler;
    private StorCommandHandler storCommandHandler;

    //-------------------------------------------------------------------------
    // Tests
    //-------------------------------------------------------------------------

    /**
     * Test connecting and logging in
     */
    public void testLogin() throws Exception {
        // Connect
        LOG.info("Conecting to " + SERVER);
        ftpClientConnect();
        verifyReplyCode("connect", 220);

        // Login
        String userAndPassword = USERNAME + "/" + PASSWORD;
        LOG.info("Logging in as " + userAndPassword);
        boolean success = ftpClient.login(USERNAME, PASSWORD);
        assertTrue("Unable to login with " + userAndPassword, success);
        verifyReplyCode("login with " + userAndPassword, 230);

        // Quit
        LOG.info("Quit");
        ftpClient.quit();
        verifyReplyCode("quit", 221);
    }

    /**
     * Test the ACCT command
     */
    public void testAcct() throws Exception {
        ftpClientConnect();

        // ACCT
        int replyCode = ftpClient.acct("123456");
        assertEquals("acct", 230, replyCode);
    }

    /**
     * Test the stop() method when no session has ever been started
     */
    public void testStop_NoSessionEverStarted() throws Exception {
        LOG.info("Testing a stop() when no session has ever been started");
    }

    /**
     * Test help (HELP)
     */
    public void testHelp() throws Exception {
        // Modify HELP CommandHandler to return a predefined help message
        final String HELP = "help message";
        HelpCommandHandler helpCommandHandler = (HelpCommandHandler) stubFtpServer.getCommandHandler(CommandNames.HELP);
        helpCommandHandler.setHelpMessage(HELP);

        ftpClientConnect();

        // HELP
        String help = ftpClient.listHelp();
        assertTrue("Wrong response", help.indexOf(HELP) != -1);
        verifyReplyCode("listHelp", 214);
    }

    /**
     * Test the LIST and SYST commands.
     */
    public void testList() throws Exception {
        ftpClientConnect();

        // Set directory listing
        ListCommandHandler listCommandHandler = (ListCommandHandler) stubFtpServer.getCommandHandler(CommandNames.LIST);
        listCommandHandler.setDirectoryListing("11-09-01 12:30PM  406348 File2350.log\n"
                + "11-01-01 1:30PM <DIR> 0 archive");

        // LIST
        FTPFile[] files = ftpClient.listFiles();
        assertEquals("number of files", 2, files.length);
        verifyFTPFile(files[0], FTPFile.FILE_TYPE, "File2350.log", 406348L);
        verifyFTPFile(files[1], FTPFile.DIRECTORY_TYPE, "archive", 0L);
        verifyReplyCode("list", 226);
    }

    /**
     * Test the LIST, PASV and SYST commands, transferring a directory listing in passive mode
     */
    public void testList_PassiveMode() throws Exception {
        ftpClientConnect();

        ftpClient.enterLocalPassiveMode();

        // Set directory listing
        ListCommandHandler listCommandHandler = (ListCommandHandler) stubFtpServer.getCommandHandler(CommandNames.LIST);
        listCommandHandler.setDirectoryListing("11-09-01 12:30PM  406348 File2350.log");

        // LIST
        FTPFile[] files = ftpClient.listFiles();
        assertEquals("number of files", 1, files.length);
        verifyReplyCode("list", 226);
    }

    /**
     * Test the NLST command.
     */
    public void testNlst() throws Exception {
        ftpClientConnect();

        // Set directory listing
        NlstCommandHandler nlstCommandHandler = (NlstCommandHandler) stubFtpServer.getCommandHandler(CommandNames.NLST);
        nlstCommandHandler.setDirectoryListing("File1.txt\nfile2.data");

        // NLST
        String[] filenames = ftpClient.listNames();
        assertEquals("number of files", 2, filenames.length);
        assertEquals(filenames[0], "File1.txt");
        assertEquals(filenames[1], "file2.data");
        verifyReplyCode("listNames", 226);
    }

    /**
     * Test printing the current working directory (PWD)
     */
    public void testPwd() throws Exception {
        // Modify PWD CommandHandler to return a predefined directory
        final String DIR = "some/dir";
        PwdCommandHandler pwdCommandHandler = (PwdCommandHandler) stubFtpServer.getCommandHandler(CommandNames.PWD);
        pwdCommandHandler.setDirectory(DIR);

        ftpClientConnect();

        // PWD
        String dir = ftpClient.printWorkingDirectory();
        assertEquals("Unable to PWD", DIR, dir);
        verifyReplyCode("printWorkingDirectory", 257);
    }

    /**
     * Test getting the status (STAT)
     */
    public void testStat() throws Exception {
        // Modify Stat CommandHandler to return predefined text
        final String STATUS = "some information 123";
        StatCommandHandler statCommandHandler = (StatCommandHandler) stubFtpServer.getCommandHandler(CommandNames.STAT);
        statCommandHandler.setStatus(STATUS);

        ftpClientConnect();

        // STAT
        String status = ftpClient.getStatus();
        assertEquals("STAT reply", "211 " + STATUS + ".", status.trim());
        verifyReplyCode("getStatus", 211);
    }

    /**
     * Test getting the status (STAT), when the reply text contains multiple lines
     */
    public void testStat_MultilineReplyText() throws Exception {
        // Modify Stat CommandHandler to return predefined text
        final String STATUS = "System name: abc.def\nVersion 3.5.7\nNumber of failed logins: 2";
        final String FORMATTED_REPLY_STATUS = "211-System name: abc.def\r\nVersion 3.5.7\r\n211 Number of failed logins: 2.";
        StatCommandHandler statCommandHandler = (StatCommandHandler) stubFtpServer.getCommandHandler(CommandNames.STAT);
        statCommandHandler.setStatus(STATUS);

        ftpClientConnect();

        // STAT
        String status = ftpClient.getStatus();
        assertEquals("STAT reply", FORMATTED_REPLY_STATUS, status.trim());
        verifyReplyCode("getStatus", 211);
    }

    /**
     * Test the System (SYST) command
     */
    public void testSyst() throws Exception {
        ftpClientConnect();

        // SYST
        assertEquals("getSystemName()", "\"WINDOWS\" system type.", ftpClient.getSystemName());
        verifyReplyCode("syst", 215);
    }

    /**
     * Test changing the current working directory (CWD)
     */
    public void testCwd() throws Exception {
        // Connect
        LOG.info("Conecting to " + SERVER);
        ftpClientConnect();
        verifyReplyCode("connect", 220);

        // CWD
        boolean success = ftpClient.changeWorkingDirectory("dir1/dir2");
        assertTrue("Unable to CWD", success);
        verifyReplyCode("changeWorkingDirectory", 250);
    }

    /**
     * Test changing the current working directory (CWD), when it causes a remote error
     */
    public void testCwd_Error() throws Exception {
        // Override CWD CommandHandler to return error reply code
        final int REPLY_CODE = 500;
        StaticReplyCommandHandler cwdCommandHandler = new StaticReplyCommandHandler(REPLY_CODE);
        stubFtpServer.setCommandHandler("CWD", cwdCommandHandler);

        ftpClientConnect();

        // CWD
        boolean success = ftpClient.changeWorkingDirectory("dir1/dir2");
        assertFalse("Expected failure", success);
        verifyReplyCode("changeWorkingDirectory", REPLY_CODE);
    }

    /**
     * Test changing to the parent directory (CDUP)
     */
    public void testCdup() throws Exception {
        ftpClientConnect();

        // CDUP
        boolean success = ftpClient.changeToParentDirectory();
        assertTrue("Unable to CDUP", success);
        verifyReplyCode("changeToParentDirectory", 200);
    }

    /**
     * Test delete (DELE)
     */
    public void testDele() throws Exception {
        ftpClientConnect();

        // DELE
        boolean success = ftpClient.deleteFile(FILENAME);
        assertTrue("Unable to DELE", success);
        verifyReplyCode("deleteFile", 250);
    }

    /**
     * Test make directory (MKD)
     */
    public void testMkd() throws Exception {
        ftpClientConnect();

        // MKD
        boolean success = ftpClient.makeDirectory("dir1/dir2");
        assertTrue("Unable to CWD", success);
        verifyReplyCode("makeDirectory", 257);
    }

    /**
     * Test NOOP
     */
    public void testNoop() throws Exception {
        ftpClientConnect();

        // NOOP
        boolean success = ftpClient.sendNoOp();
        assertTrue("Unable to NOOP", success);
        verifyReplyCode("NOOP", 200);
    }

    /**
     * Test restart (REST)
     */
    public void testRest() throws Exception {
        ftpClientConnect();

        // REST
        int replyCode = ftpClient.rest("marker");
        assertEquals("Unable to REST", 350, replyCode);
    }

    /**
     * Test changing the current working directory (RMD)
     */
    public void testRmd() throws Exception {
        ftpClientConnect();

        // RMD
        boolean success = ftpClient.removeDirectory("dir1/dir2");
        assertTrue("Unable to RMD", success);
        verifyReplyCode("removeDirectory", 250);
    }

    /**
     * Test rename (RNFR/RNTO)
     */
    public void testRename() throws Exception {
        ftpClientConnect();

        // Rename (RNFR, RNTO)
        boolean success = ftpClient.rename(FILENAME, "new_" + FILENAME);
        assertTrue("Unable to RENAME", success);
        verifyReplyCode("rename", 250);
    }

    /**
     * Test the ALLO command
     */
    public void testAllo() throws Exception {
        ftpClientConnect();

        // ALLO
        assertTrue("ALLO", ftpClient.allocate(1024));
        assertTrue("ALLO with recordSize", ftpClient.allocate(1024, 64));
    }

    /**
     * Test GET and PUT of ASCII files
     */
    public void testTransferAsciiFile() throws Exception {
        retrCommandHandler.setFileContents(ASCII_CONTENTS);

        ftpClientConnect();

        // Get File
        LOG.info("Get File for remotePath [" + FILENAME + "]");
        ByteArrayOutputStream outputStream = new ByteArrayOutputStream();
        assertTrue(ftpClient.retrieveFile(FILENAME, outputStream));
        LOG.info("File contents=[" + outputStream.toString());
        assertEquals("File contents", ASCII_CONTENTS, outputStream.toString());

        // Put File
        LOG.info("Put File for local path [" + FILENAME + "]");
        ByteArrayInputStream inputStream = new ByteArrayInputStream(ASCII_CONTENTS.getBytes());
        assertTrue(ftpClient.storeFile(FILENAME, inputStream));
        InvocationRecord invocationRecord = storCommandHandler.getInvocation(0);
        byte[] contents = (byte[]) invocationRecord.getObject(StorCommandHandler.FILE_CONTENTS_KEY);
        LOG.info("File contents=[" + contents + "]");
        assertEquals("File contents", ASCII_CONTENTS.getBytes(), contents);
    }

    /**
     * Test GET and PUT of binary files
     */
    public void testTransferBinaryFiles() throws Exception {
        retrCommandHandler.setFileContents(BINARY_CONTENTS);

        ftpClientConnect();
        ftpClient.setFileType(FTP.BINARY_FILE_TYPE);

        // Get File
        LOG.info("Get File for remotePath [" + FILENAME + "]");
        ByteArrayOutputStream outputStream = new ByteArrayOutputStream();
        assertTrue("GET", ftpClient.retrieveFile(FILENAME, outputStream));
        LOG.info("GET File length=" + outputStream.size());
        assertEquals("File contents", BINARY_CONTENTS, outputStream.toByteArray());

        // Put File
        LOG.info("Put File for local path [" + FILENAME + "]");
        ByteArrayInputStream inputStream = new ByteArrayInputStream(BINARY_CONTENTS);
        assertTrue("PUT", ftpClient.storeFile(FILENAME, inputStream));
        InvocationRecord invocationRecord = storCommandHandler.getInvocation(0);
        byte[] contents = (byte[]) invocationRecord.getObject(StorCommandHandler.FILE_CONTENTS_KEY);
        LOG.info("PUT File length=" + contents.length);
        assertEquals("File contents", BINARY_CONTENTS, contents);
    }

    /**
     * Test the STOU command
     */
    public void testStou() throws Exception {
        StouCommandHandler stouCommandHandler = (StouCommandHandler) stubFtpServer.getCommandHandler(CommandNames.STOU);
        stouCommandHandler.setFilename(FILENAME);

        ftpClientConnect();

        // Stor a File (STOU)
        ByteArrayInputStream inputStream = new ByteArrayInputStream(ASCII_CONTENTS.getBytes());
        assertTrue(ftpClient.storeUniqueFile(FILENAME, inputStream));
        InvocationRecord invocationRecord = stouCommandHandler.getInvocation(0);
        byte[] contents = (byte[]) invocationRecord.getObject(StorCommandHandler.FILE_CONTENTS_KEY);
        LOG.info("File contents=[" + contents + "]");
        assertEquals("File contents", ASCII_CONTENTS.getBytes(), contents);
    }

    /**
     * Test the APPE command
     */
    public void testAppe() throws Exception {
        AppeCommandHandler appeCommandHandler = (AppeCommandHandler) stubFtpServer.getCommandHandler(CommandNames.APPE);

        ftpClientConnect();

        // Append a File (APPE)
        ByteArrayInputStream inputStream = new ByteArrayInputStream(ASCII_CONTENTS.getBytes());
        assertTrue(ftpClient.appendFile(FILENAME, inputStream));
        InvocationRecord invocationRecord = appeCommandHandler.getInvocation(0);
        byte[] contents = (byte[]) invocationRecord.getObject(AppeCommandHandler.FILE_CONTENTS_KEY);
        LOG.info("File contents=[" + contents + "]");
        assertEquals("File contents", ASCII_CONTENTS.getBytes(), contents);
    }

    /**
     * Test the ABOR command
     */
    public void testAbor() throws Exception {
        ftpClientConnect();

        // ABOR
        assertTrue("ABOR", ftpClient.abort());
    }

    /**
     * Test the Passive (PASV) command
     */
    public void testPasv() throws Exception {
        ftpClientConnect();

        // PASV
        ftpClient.enterLocalPassiveMode();
        // no reply code; the PASV command is sent only when the data connection is opened 
    }

    /**
     * Test Mode (MODE)
     */
    public void testMode() throws Exception {
        ftpClientConnect();

        // MODE
        boolean success = ftpClient.setFileTransferMode(FTP.STREAM_TRANSFER_MODE);
        assertTrue("Unable to MODE", success);
        verifyReplyCode("setFileTransferMode", 200);
    }

    /**
     * Test file structure (STRU)
     */
    public void testStru() throws Exception {
        ftpClientConnect();

        // STRU
        boolean success = ftpClient.setFileStructure(FTP.FILE_STRUCTURE);
        assertTrue("Unable to STRU", success);
        verifyReplyCode("setFileStructure", 200);
    }

    /**
     * Test the SimpleCompositeCommandHandler
     */
    public void testSimpleCompositeCommandHandler() throws Exception {
        // Replace CWD CommandHandler with a SimpleCompositeCommandHandler
        CommandHandler commandHandler1 = new StaticReplyCommandHandler(500);
        CommandHandler commandHandler2 = new CwdCommandHandler();
        SimpleCompositeCommandHandler simpleCompositeCommandHandler = new SimpleCompositeCommandHandler();
        simpleCompositeCommandHandler.addCommandHandler(commandHandler1);
        simpleCompositeCommandHandler.addCommandHandler(commandHandler2);
        stubFtpServer.setCommandHandler("CWD", simpleCompositeCommandHandler);

        // Connect
        ftpClientConnect();

        // CWD
        assertFalse("first", ftpClient.changeWorkingDirectory("dir1/dir2"));
        assertTrue("first", ftpClient.changeWorkingDirectory("dir1/dir2"));
    }

    /**
     * Test site parameters (SITE)
     */
    public void testSite() throws Exception {
        ftpClientConnect();

        // SITE
        int replyCode = ftpClient.site("parameters,1,2,3");
        assertEquals("SITE", 200, replyCode);
    }

    /**
     * Test structure mount (SMNT)
     */
    public void testSmnt() throws Exception {
        ftpClientConnect();

        // SMNT
        assertTrue("SMNT", ftpClient.structureMount("dir1/dir2"));
        verifyReplyCode("structureMount", 250);
    }

    /**
     * Test reinitialize (REIN)
     */
    public void testRein() throws Exception {
        ftpClientConnect();

        // REIN
        assertEquals("REIN", 220, ftpClient.rein());
    }

    /**
     * Test that command names in lowercase or mixed upper/lower case are accepted
     */
    public void testCommandNamesInLowerOrMixedCase() throws Exception {
        ftpClientConnect();

        assertEquals("rein", 220, ftpClient.sendCommand("rein"));
        assertEquals("rEIn", 220, ftpClient.sendCommand("rEIn"));
        assertEquals("reiN", 220, ftpClient.sendCommand("reiN"));
        assertEquals("Rein", 220, ftpClient.sendCommand("Rein"));
    }

    // -------------------------------------------------------------------------
    // Test setup and tear-down
    // -------------------------------------------------------------------------

    /**
     * Perform initialization before each test
     *
     * @see org.mockftpserver.test.AbstractTest#setUp()
     */
    protected void setUp() throws Exception {
        super.setUp();

        for (int i = 0; i < BINARY_CONTENTS.length; i++) {
            BINARY_CONTENTS[i] = (byte) i;
        }

        stubFtpServer = new StubFtpServer();
        stubFtpServer.setServerControlPort(PortTestUtil.getFtpServerControlPort());
        stubFtpServer.start();
        ftpClient = new FTPClient();
        retrCommandHandler = (RetrCommandHandler) stubFtpServer.getCommandHandler(CommandNames.RETR);
        storCommandHandler = (StorCommandHandler) stubFtpServer.getCommandHandler(CommandNames.STOR);
    }

    /**
     * Perform cleanup after each test
     *
     * @see org.mockftpserver.test.AbstractTest#tearDown()
     */
    protected void tearDown() throws Exception {
        super.tearDown();
        stubFtpServer.stop();
    }

    // -------------------------------------------------------------------------
    // Internal Helper Methods
    // -------------------------------------------------------------------------

    /**
     * Connect to the server from the FTPClient
     */
    private void ftpClientConnect() throws IOException {
        ftpClient.connect(SERVER, PortTestUtil.getFtpServerControlPort());
    }

    /**
     * Assert that the FtpClient reply code is equal to the expected value
     *
     * @param operation         - the description of the operation performed; used in the error message
     * @param expectedReplyCode - the expected FtpClient reply code
     */
    private void verifyReplyCode(String operation, int expectedReplyCode) {
        int replyCode = ftpClient.getReplyCode();
        LOG.info("Reply: operation=\"" + operation + "\" replyCode=" + replyCode);
        assertEquals("Unexpected replyCode for " + operation, expectedReplyCode, replyCode);
    }

    /**
     * Verify that the FTPFile has the specified properties
     *
     * @param ftpFile - the FTPFile to verify
     * @param type    - the expected file type
     * @param name    - the expected file name
     * @param size    - the expected file size (will be zero for a directory)
     */
    private void verifyFTPFile(FTPFile ftpFile, int type, String name, long size) {
        LOG.info(ftpFile);
        assertEquals("type: " + ftpFile, type, ftpFile.getType());
        assertEquals("name: " + ftpFile, name, ftpFile.getName());
        assertEquals("size: " + ftpFile, size, ftpFile.getSize());
    }

}
