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

import org.mockftpserver.core.session.SessionKeys
import org.mockftpserver.core.command.ReplyCodes
import org.mockftpserver.fake.user.UserAccount

/**
 * Abstract superclass for tests of CommandHandler classes that require a user login.
 * 
 * @version $Revision: $ - $Date: $
 *
 * @author Chris Mair
 */
abstract class AbstractLoginRequiredCommandHandlerTest extends AbstractFakeCommandHandlerTest {

     // I am not thrilled about bloating the class hierarchy for this. We'll see how it goes. CNM Apr 2008
     
     protected userAccount

    //-------------------------------------------------------------------------
    // Tests (common to all subclasses)
    //-------------------------------------------------------------------------
    
    void testHandleCommand_NotLoggedIn() {
        def command = createValidCommand()
        session.removeAttribute(SessionKeys.USER_ACCOUNT)
		commandHandler.handleCommand(command, session)
        assertSessionReply(ReplyCodes.NOT_LOGGED_IN)
    }

    //-------------------------------------------------------------------------
    // Test Setup
    //-------------------------------------------------------------------------
    
	void setUp() {
	    super.setUp()
	    userAccount = new UserAccount()
        session.setAttribute(SessionKeys.USER_ACCOUNT, userAccount)
	}

}