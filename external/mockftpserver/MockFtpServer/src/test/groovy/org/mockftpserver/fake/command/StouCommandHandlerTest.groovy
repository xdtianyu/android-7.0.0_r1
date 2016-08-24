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

import org.mockftpserver.core.command.Command
import org.mockftpserver.core.command.CommandHandler
import org.mockftpserver.core.command.CommandNames

/**
 * Tests for StouCommandHandler
 *
 * @version $Revision$ - $Date$
 *
 * @author Chris Mair
 */
class StouCommandHandlerTest extends AbstractStoreFileCommandHandlerTestCase {

    def expectedBaseName

    void testHandleCommand_SpecifyBaseFilename() {
        setCurrentDirectory(DIR)
        expectedBaseName = FILENAME
        testHandleCommand([expectedBaseName], 'stou', CONTENTS)
    }

    void testHandleCommand_UseDefaultBaseFilename() {
        setCurrentDirectory(DIR)
        expectedBaseName = 'Temp'
        testHandleCommand([expectedBaseName], 'stou', CONTENTS)
    }

    void testHandleCommand_AbsolutePath() {
        expectedBaseName = FILENAME
        testHandleCommand([FILE], 'stou', CONTENTS)
    }

    void testHandleCommand_NoWriteAccessToExistingFile() {
        // This command always stores a new (unique) file, so this test does not apply
    }
    //-------------------------------------------------------------------------
    // Helper Methods
    //-------------------------------------------------------------------------

    CommandHandler createCommandHandler() {
        new StouCommandHandler()
    }

    Command createValidCommand() {
        return new Command(CommandNames.STOU, [])
    }

    void setUp() {
        super.setUp()
        session.dataToRead = CONTENTS.bytes
    }

    protected String verifyOutputFile() {
        def names = fileSystem.listNames(DIR)
        def filename = names.find {name -> name.startsWith(expectedBaseName) }
        assert filename
        return p(DIR, filename)
    }

}