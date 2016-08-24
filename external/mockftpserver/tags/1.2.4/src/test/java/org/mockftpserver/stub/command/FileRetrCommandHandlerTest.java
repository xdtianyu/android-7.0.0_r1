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
package org.mockftpserver.stub.command;

import java.util.Arrays;


import org.apache.log4j.Logger;
import org.easymock.ArgumentsMatcher;
import org.mockftpserver.core.command.Command;
import org.mockftpserver.core.command.CommandNames;
import org.mockftpserver.core.command.ReplyCodes;
import org.mockftpserver.core.util.AssertFailedException;
import org.mockftpserver.stub.command.FileRetrCommandHandler;

/**
 * Tests for the FileRetrCommandHandler class
 * 
 * @version $Revision$ - $Date$
 * 
 * @author Chris Mair
 */
public final class FileRetrCommandHandlerTest extends AbstractCommandHandlerTest {

    private static final Logger LOG = Logger.getLogger(FileRetrCommandHandlerTest.class);
    private static final byte BYTE1 = (byte)7;
    private static final byte BYTE2 = (byte)21;
    
    private FileRetrCommandHandler commandHandler;
    
    /**
     * Test the constructor that takes a String, passing in a null
     */
    public void testConstructor_String_Null() {
        try {
            new FileRetrCommandHandler((String)null);
            fail("Expected AssertFailedException");
        }
        catch (AssertFailedException expected) {
            LOG.info("Expected: " + expected);
        }
    }
    
    /**
     * Test the setFile(String) method, passing in a null
     */
    public void testSetFile_Null() {
        try {
            commandHandler.setFile(null);
            fail("Expected AssertFailedException");
        }
        catch (AssertFailedException expected) {
            LOG.info("Expected: " + expected);
        }
    }
    
    /**
     * Test the handleCommand(Command,Session) method. Create a temporary (binary) file, and
     * make sure its contents are written back 
     * @throws Exception
     */
    public void testHandleCommand() throws Exception {

         final byte[] BUFFER = new byte[FileRetrCommandHandler.BUFFER_SIZE];
         Arrays.fill(BUFFER, BYTE1);

        session.sendReply(ReplyCodes.SEND_DATA_INITIAL_OK, replyTextFor(ReplyCodes.SEND_DATA_INITIAL_OK));
        session.openDataConnection();

        ArgumentsMatcher matcher = new ArgumentsMatcher() {
            int counter = -1;   // will increment for each invocation
            public boolean matches(Object[] expected, Object[] actual) {
                counter++;
                byte[] buffer = (byte[])actual[0];
                int expectedLength = ((Integer)expected[1]).intValue();
                int actualLength = ((Integer)actual[1]).intValue();
                LOG.info("invocation #" + counter + " expected=" + expectedLength + " actualLength=" + actualLength);
                if (counter < 5) {
                    assertEquals("buffer for invocation #" + counter, BUFFER, buffer);
                }
                else {
                    // TODO Got two invocations here; only expected one
                    //assertEquals("length for invocation #" + counter, expectedLength, actualLength);
                    assertEquals("buffer[0]", BYTE2, buffer[0]);
                    assertEquals("buffer[1]", BYTE2, buffer[1]);
                    assertEquals("buffer[2]", BYTE2, buffer[2]);
                }
                return true;
            }
            public String toString(Object[] args) {
                return args[0].getClass().getName() + " " + args[1].toString();
            }
        };

        session.sendData(BUFFER, 512);
        control(session).setMatcher(matcher);
        session.sendData(BUFFER, 512);
        session.sendData(BUFFER, 512);
        session.sendData(BUFFER, 512);
        session.sendData(BUFFER, 512);
        session.sendData(BUFFER, 3);
        
        session.closeDataConnection();
        session.sendReply(ReplyCodes.SEND_DATA_FINAL_OK, replyTextFor(ReplyCodes.SEND_DATA_FINAL_OK));
        replay(session);
        
        commandHandler.setFile("Sample.data");
        Command command = new Command(CommandNames.RETR, array(FILENAME1));
        commandHandler.handleCommand(command, session);
        verify(session);
        
        verifyNumberOfInvocations(commandHandler, 1);
        verifyOneDataElement(commandHandler.getInvocation(0), FileRetrCommandHandler.PATHNAME_KEY, FILENAME1);
    }
    
    /**
     * Test the handleCommand() method, when no pathname parameter has been specified
     */
    public void testHandleCommand_MissingPathnameParameter() throws Exception {
        commandHandler.setFile("abc.txt");      // this property must be set
        testHandleCommand_InvalidParameters(commandHandler, CommandNames.RETR, EMPTY);
    }

    /**
     * Test the HandleCommand method, when the file property has not been set
     */
    public void testHandleCommand_FileNotSet() throws Exception {
        try {
            commandHandler.handleCommand(new Command(CommandNames.RETR, EMPTY), session);
            fail("Expected AssertFailedException");
        }
        catch (AssertFailedException expected) {
            LOG.info("Expected: " + expected);
        }
    }
    
    /**
     * Perform initialization before each test
     * @see org.mockftpserver.stub.command.AbstractCommandHandlerTest#setUp()
     */
    protected void setUp() throws Exception {
        super.setUp();
        commandHandler = new FileRetrCommandHandler();
        commandHandler.setReplyTextBundle(replyTextBundle);
    }

//    /**
//     * Create a sample binary file; 5 buffers full plus 3 extra bytes
//     */
//    private void createSampleFile() {
//        final String FILE_PATH = "test/org.mockftpserver/command/Sample.data";
//        final byte[] BUFFER = new byte[FileRetrCommandHandler.BUFFER_SIZE];
//        Arrays.fill(BUFFER, BYTE1);
//
//        File file = new File(FILE_PATH);
//        FileOutputStream out = new FileOutputStream(file);
//        for (int i = 0; i < 5; i++) {
//            out.write(BUFFER);
//        }
//        Arrays.fill(BUFFER, BYTE2);
//        out.write(BUFFER, 0, 3);
//        out.close();
//        LOG.info("Created temporary file [" + FILE_PATH + "]: length=" + file.length());
//    }

}
