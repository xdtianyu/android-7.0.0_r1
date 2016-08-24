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

import org.apache.log4j.Logger;
import org.mockftpserver.core.util.AssertFailedException;
import org.mockftpserver.test.AbstractTest;

import java.util.ListResourceBundle;
import java.util.ResourceBundle;

/**
 * Tests for the ReplyTextBundleUtil class.
 * 
 * @version $Revision$ - $Date$
 *
 * @author Chris Mair
 */
public final class ReplyTextBundleUtilTest extends AbstractTest {

    private static final Logger LOG = Logger.getLogger(ReplyTextBundleUtilTest.class);
    
    private ResourceBundle resourceBundle1;
    private ResourceBundle resourceBundle2;
    
    /**
     * Test the setReplyTextBundleIfAppropriate() method, when the CommandHandler implements 
     * the ResourceBundleAware interface, and the replyTextBundle has not yet been set. 
     */
    public void testSetReplyTextBundleIfAppropriate_ReplyTextBundleAware_NotSetYet() {
        AbstractTrackingCommandHandler commandHandler = new StaticReplyCommandHandler();
        ReplyTextBundleUtil.setReplyTextBundleIfAppropriate(commandHandler, resourceBundle1);
        assertSame(resourceBundle1, commandHandler.getReplyTextBundle());
    }

    /**
     * Test the setReplyTextBundleIfAppropriate() method, when the CommandHandler implements 
     * the ResourceBundleAware interface, and the replyTextBundle has already been set. 
     */
    public void testSetReplyTextBundleIfAppropriate_ReplyTextBundleAware_AlreadySet() {
        AbstractTrackingCommandHandler commandHandler = new StaticReplyCommandHandler();
        commandHandler.setReplyTextBundle(resourceBundle2);
        ReplyTextBundleUtil.setReplyTextBundleIfAppropriate(commandHandler, resourceBundle1);
        assertSame(resourceBundle2, commandHandler.getReplyTextBundle());
    }

    /**
     * Test the setReplyTextBundleIfAppropriate() method, when the CommandHandler does not 
     * implement the ResourceBundleAware interface. 
     */
    public void testSetReplyTextBundleIfAppropriate_NotReplyTextBundleAware() {
        CommandHandler commandHandler = (CommandHandler) createMock(CommandHandler.class);
        replay(commandHandler);
        ReplyTextBundleUtil.setReplyTextBundleIfAppropriate(commandHandler, resourceBundle1);
        verify(commandHandler);         // expect no method calls
    }
    
    /**
     * Test the setReplyTextBundleIfAppropriate() method, when the CommandHandler is null. 
     */
    public void testSetReplyTextBundleIfAppropriate_NullCommandHandler() {
        try {
            ReplyTextBundleUtil.setReplyTextBundleIfAppropriate(null, resourceBundle1);
            fail("Expected AssertFailedException");
        }
        catch (AssertFailedException expected) {
            LOG.info("Expected: " + expected);
        }
    }
    
    /**
     * @see org.mockftpserver.test.AbstractTest#setUp()
     */
    protected void setUp() throws Exception {
        super.setUp();

        resourceBundle1 = new ListResourceBundle() {
            protected Object[][] getContents() {
                return null;
            }
        };

        resourceBundle2 = new ListResourceBundle() {
            protected Object[][] getContents() {
                return new Object[][] { { "a", "b" } };
            }
        };
    }
    
}
