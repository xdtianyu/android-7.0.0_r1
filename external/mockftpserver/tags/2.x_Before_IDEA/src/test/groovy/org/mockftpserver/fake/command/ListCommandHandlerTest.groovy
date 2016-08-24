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
package org.mockftpserver.fake.command

import org.mockftpserver.test.AbstractGroovyTest
import org.mockftpserver.core.command.Command
import org.mockftpserver.core.command.CommandHandler
import org.mockftpserver.core.command.CommandNamesimport org.mockftpserver.core.session.StubSession
import org.mockftpserver.core.session.SessionKeys
import org.mockftpserver.fake.StubServerConfiguration
import org.mockftpserver.fake.user.UserAccount
import org.apache.log4j.Loggerimport org.mockftpserver.core.command.ReplyCodesimport org.mockftpserver.fake.filesystem.FileInfoimport java.text.SimpleDateFormatimport org.mockftpserver.fake.filesystem.FileEntryimport org.mockftpserver.fake.filesystem.DirectoryEntry
/**
 * Tests for ListCommandHandler
 * 
 * @version $Revision: $ - $Date: $
 *
 * @author Chris Mair
 */
class ListCommandHandlerTest extends AbstractLoginRequiredCommandHandlerTest {

    private static final SIZE_WIDTH = ListCommandHandler.SIZE_WIDTH
    private static final DIR = "/usr"
    private static final NAME = "abc.txt"
    private static final LAST_MODIFIED = new Date()
    private static final SIZE = 1000
    
    def dateFormat
    def lastModifiedFormatted
    
    void testHandleCommand_SingleFile() {
        fileSystem.addEntry(new FileEntry(path:p(DIR,NAME), lastModified:LAST_MODIFIED, contents:"abc"))
        handleCommandAndVerifySendDataReplies([DIR])
        assertSessionData(listingForFile(LAST_MODIFIED, "abc".size(), NAME),)
	}

    void testHandleCommand_FilesAndDirectories() {
        def NAME1 = "abc.txt"
        def NAME2 = "OtherFiles"
        def NAME3 = "another_file.doc"
        def DATA1 = "abc"
        def DATA3 = "".padRight(1000, 'x')
        fileSystem.addEntry(new FileEntry(path:p(DIR,NAME1), lastModified:LAST_MODIFIED, contents:DATA1))
        fileSystem.addEntry(new DirectoryEntry(path:p(DIR,NAME2), lastModified:LAST_MODIFIED))
        fileSystem.addEntry(new FileEntry(path:p(DIR,NAME3), lastModified:LAST_MODIFIED, contents:DATA3))

        handleCommandAndVerifySendDataReplies([DIR])
                
        def actualLines = session.sentData[0].tokenize(endOfLine()) as Set
        LOG.info("actualLines=$actualLines")
        def EXPECTED = [
            listingForFile(LAST_MODIFIED, DATA1.size(), NAME1),
            listingForDirectory(LAST_MODIFIED, NAME2),
            listingForFile(LAST_MODIFIED, DATA3.size(), NAME3) ] as Set
        assert actualLines == EXPECTED
	}
    
    void testHandleCommand_NoPath_UseCurrentDirectory() {
        fileSystem.addEntry(new FileEntry(path:p(DIR,NAME), lastModified:LAST_MODIFIED, contents:"abc"))
        session.setAttribute(SessionKeys.CURRENT_DIRECTORY, DIR)
        handleCommandAndVerifySendDataReplies([])
        assertSessionData(listingForFile(LAST_MODIFIED, "abc".size(), NAME),)
	}

    void testHandleCommand_EmptyDirectory() {
        handleCommandAndVerifySendDataReplies([DIR])
        assertSessionData("")
	}
    
    void testHandleCommand_PathSpecifiesAFile() {
        fileSystem.addEntry(new FileEntry(path:p(DIR,NAME), lastModified:LAST_MODIFIED, contents:"abc"))
        handleCommandAndVerifySendDataReplies([p(DIR,NAME)])
        assertSessionData(listingForFile(LAST_MODIFIED, "abc".size(), NAME),)
	}
    
    void testHandleCommand_PathDoesNotExist() {
        handleCommandAndVerifySendDataReplies(["/DoesNotExist"])
        assertSessionData("")
	}
    
    void testDirectoryListing_File() {
        def fileInfo = FileInfo.forFile(NAME, SIZE, LAST_MODIFIED)
        def sizeStr = SIZE.toString().padLeft(SIZE_WIDTH)
        def expected = "$lastModifiedFormatted  $sizeStr  $NAME"
        def actual = commandHandler.directoryListing(fileInfo)
        assert actual == expected 
    }
    
    void testDirectoryListing_Directory() {
        def fileInfo = FileInfo.forDirectory(NAME, LAST_MODIFIED)
        def dirStr = "<DIR>".padRight(SIZE_WIDTH)
        def expected = "$lastModifiedFormatted  $dirStr  $NAME"
        def actual = commandHandler.directoryListing(fileInfo)
        assert actual == expected 
    }
    
    //-------------------------------------------------------------------------
    // Helper Methods
    //-------------------------------------------------------------------------
    
	CommandHandler createCommandHandler() {
	    new ListCommandHandler()
	}
	
    Command createValidCommand() {
        return new Command(CommandNames.LIST, [DIR])
    }

    void setUp() {
        super.setUp()
        assert fileSystem.createDirectory("/usr")
        dateFormat = new SimpleDateFormat(ListCommandHandler.DATE_FORMAT)
        lastModifiedFormatted = dateFormat.format(LAST_MODIFIED)
    }
    
    private String listingForFile(lastModified, size, name) {
        def lastModifiedFormatted = dateFormat.format(lastModified)
        "$lastModifiedFormatted  ${size.toString().padLeft(SIZE_WIDTH)}  $name"
    }

    private String listingForDirectory(lastModified, name) {
        def lastModifiedFormatted = dateFormat.format(lastModified)
        "$lastModifiedFormatted  ${'<DIR>'.padRight(SIZE_WIDTH)}  $name"    
    }

}