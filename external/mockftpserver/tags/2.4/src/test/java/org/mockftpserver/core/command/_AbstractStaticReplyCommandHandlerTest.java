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
import org.mockftpserver.test.AbstractTestCase;

import java.util.ListResourceBundle;
import java.util.ResourceBundle;

/**
 * Tests for the AbstractStaticReplyCommandHandler class. The class name is prefixed with an underscore
 * so that it is not filtered out by Maven's Surefire test plugin.
 *
 * @version $Revision$ - $Date$
 *
 * @author Chris Mair
 */
public final class _AbstractStaticReplyCommandHandlerTest extends AbstractTestCase {

    private static final Logger LOG = LoggerFactory.getLogger(_AbstractStaticReplyCommandHandlerTest.class);
    private static final int REPLY_CODE1 = 777;
    private static final int REPLY_CODE2 = 888;
    private static final String REPLY_TEXT1 = "reply1 ... abcdef";
    private static final String REPLY_TEXT2 = "abc {0} def";
    private static final String REPLY_TEXT2_FORMATTED = "abc 123 def";
    private static final String MESSAGE_KEY = "key.123";
    private static final String MESSAGE_TEXT = "message.123";
    private static final String OVERRIDE_REPLY_TEXT = "overridden reply ... abcdef";
    private static final Object ARG = "123";

    private AbstractStaticReplyCommandHandler commandHandler;
    private Session session;

    /**
     * Test the sendReply(Session) method
     */
    public void testSendReply() {
        session.sendReply(REPLY_CODE1, REPLY_TEXT1);
        replay(session);

        commandHandler.setReplyCode(REPLY_CODE1);
        commandHandler.sendReply(session);
        verify(session);
    }

    /**
     * Test the sendReply(Session) method, when the replyText has been set
     */
    public void testSendReply_SetReplyText() {
        session.sendReply(REPLY_CODE1, OVERRIDE_REPLY_TEXT);
        replay(session);

        commandHandler.setReplyCode(REPLY_CODE1);
        commandHandler.setReplyText(OVERRIDE_REPLY_TEXT);
        commandHandler.sendReply(session);
        verify(session);
    }

    /**
     * Test the sendReply(Session) method, when the replyMessageKey has been set
     */
    public void testSendReply_SetReplyMessageKey() {
        session.sendReply(REPLY_CODE1, REPLY_TEXT2);
        replay(session);

        commandHandler.setReplyCode(REPLY_CODE1);
        commandHandler.setReplyMessageKey(Integer.toString(REPLY_CODE2));
        commandHandler.sendReply(session);
        verify(session);
    }

    /**
     * Test the sendReply(Session) method, when the replyCode has not been set
     */
    public void testSendReply_ReplyCodeNotSet() {
        try {
            commandHandler.sendReply(session);
            fail("Expected AssertFailedException");
        }
        catch (AssertFailedException expected) {
            LOG.info("Expected: " + expected);
        }
    }

    /**
     * Test the sendReply(Session,Object) method
     */
    public void testSendReply_MessageParameter() {
        session.sendReply(REPLY_CODE2, REPLY_TEXT2);
        session.sendReply(REPLY_CODE2, REPLY_TEXT2_FORMATTED);
        replay(session);

        commandHandler.setReplyCode(REPLY_CODE2);
        commandHandler.sendReply(session);
        commandHandler.sendReply(session, ARG);
        verify(session);
    }

    /**
     * Test the setReplyCode() method, passing in an invalid value
     */
    public void testSetReplyCode_Invalid() {
        try {
            commandHandler.setReplyCode(0);
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
     * @see org.mockftpserver.test.AbstractTestCase#setUp()
     */
    protected void setUp() throws Exception {
        super.setUp();
        session = (Session) createMock(Session.class);
        control(session).setDefaultMatcher(MockControl.ARRAY_MATCHER);
        commandHandler = new AbstractStaticReplyCommandHandler() {
            public void handleCommand(Command command, Session session, InvocationRecord invocationRecord) throws Exception {
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
