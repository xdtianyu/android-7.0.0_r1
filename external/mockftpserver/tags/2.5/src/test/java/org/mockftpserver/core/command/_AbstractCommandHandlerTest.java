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
package org.mockftpserver.core.command;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.easymock.MockControl;
import org.mockftpserver.core.session.Session;
import org.mockftpserver.core.util.AssertFailedException;
import org.mockftpserver.stub.command.AbstractStubCommandHandler;
import org.mockftpserver.test.AbstractTestCase;

import java.util.ListResourceBundle;
import java.util.ResourceBundle;

/**
 * Tests for the AbstractCommandHandler class. The class name is prefixed with an
 * underscore so that it is not filtered out by Maven's Surefire test plugin.
 *
 * @author Chris Mair
 * @version $Revision$ - $Date$
 */
public final class _AbstractCommandHandlerTest extends AbstractTestCase {

    private static final Logger LOG = LoggerFactory.getLogger(_AbstractTrackingCommandHandlerTest.class);
    private static final int REPLY_CODE1 = 777;
    private static final int REPLY_CODE2 = 888;
    private static final String REPLY_TEXT1 = "reply1 ... abcdef";
    private static final String REPLY_TEXT2 = "abc {0} def";
    private static final String MESSAGE_KEY = "key.123";
    private static final String MESSAGE_TEXT = "message.123";

    private AbstractCommandHandler commandHandler;

    /**
     * Test the quotes utility method
     */
    public void testQuotes() {
        assertEquals("abc", "\"abc\"", AbstractStubCommandHandler.quotes("abc"));
        assertEquals("<empty>", "\"\"", AbstractStubCommandHandler.quotes(""));
    }

    /**
     * Test the quotes utility method, passing in a null
     */
    public void testQuotes_Null() {
        try {
            AbstractStubCommandHandler.quotes(null);
            fail("Expected AssertFailedException");
        }
        catch (AssertFailedException expected) {
            LOG.info("Expected: " + expected);
        }
    }

    /**
     * Test the assertValidReplyCode() method
     */
    public void testAssertValidReplyCode() {
        // These are valid, so expect no exceptions
        commandHandler.assertValidReplyCode(1);
        commandHandler.assertValidReplyCode(100);

        // These are invalid
        testAssertValidReplyCodeWithInvalid(0);
        testAssertValidReplyCodeWithInvalid(-1);
    }

    /**
     * Test the assertValidReplyCode() method , passing in an invalid replyCode value
     *
     * @param invalidReplyCode - a reply code that is expected to be invalid
     */
    private void testAssertValidReplyCodeWithInvalid(int invalidReplyCode) {
        try {
            commandHandler.assertValidReplyCode(invalidReplyCode);
            fail("Expected AssertFailedException");
        }
        catch (AssertFailedException expected) {
            LOG.info("Expected: " + expected);
        }
    }

    //-------------------------------------------------------------------------
    // Test setup
    //-------------------------------------------------------------------------

    /**
     * Perform initialization before each test
     *
     * @see org.mockftpserver.test.AbstractTestCase#setUp()
     */
    protected void setUp() throws Exception {
        super.setUp();
        Session session = (Session) createMock(Session.class);
        control(session).setDefaultMatcher(MockControl.ARRAY_MATCHER);
        commandHandler = new AbstractCommandHandler() {
            public void handleCommand(Command command, Session session) throws Exception {
            }
        };
        ResourceBundle replyTextBundle = new ListResourceBundle() {
            protected Object[][] getContents() {
                return new Object[][]{
                        {Integer.toString(REPLY_CODE1), REPLY_TEXT1},
                        {Integer.toString(REPLY_CODE2), REPLY_TEXT2},
                        {MESSAGE_KEY, MESSAGE_TEXT}
                };
            }
        };
        commandHandler.setReplyTextBundle(replyTextBundle);
    }

}
