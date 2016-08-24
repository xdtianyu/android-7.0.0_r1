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
import org.mockftpserver.core.util.Assert;

import java.util.ResourceBundle;

/**
 * The abstract superclass for CommandHandler classes.
 *
 * @author Chris Mair
 * @version $Revision$ - $Date$
 */
public abstract class AbstractCommandHandler implements CommandHandler, ReplyTextBundleAware {

    protected final Logger LOG = Logger.getLogger(getClass());

    private ResourceBundle replyTextBundle;

    //-------------------------------------------------------------------------
    // Support for reply text ResourceBundle
    //-------------------------------------------------------------------------

    /**
     * Return the ResourceBundle containing the reply text messages
     *
     * @return the replyTextBundle
     * @see ReplyTextBundleAware#getReplyTextBundle()
     */
    public ResourceBundle getReplyTextBundle() {
        return replyTextBundle;
    }

    /**
     * Set the ResourceBundle containing the reply text messages
     *
     * @param replyTextBundle - the replyTextBundle to set
     * @see ReplyTextBundleAware#setReplyTextBundle(java.util.ResourceBundle)
     */
    public void setReplyTextBundle(ResourceBundle replyTextBundle) {
        this.replyTextBundle = replyTextBundle;
    }

    // -------------------------------------------------------------------------
    // Utility methods for subclasses
    // -------------------------------------------------------------------------

    /**
     * Return the specified text surrounded with double quotes
     *
     * @param text - the text to surround with quotes
     * @return the text with leading and trailing double quotes
     * @throws org.mockftpserver.core.util.AssertFailedException
     *          - if text is null
     */
    protected static String quotes(String text) {
        Assert.notNull(text, "text");
        final String QUOTES = "\"";
        return QUOTES + text + QUOTES;
    }

    /**
     * Assert that the specified number is a valid reply code
     *
     * @param replyCode - the reply code to check
     * @throws org.mockftpserver.core.util.AssertFailedException
     *          - if the replyCode is invalid
     */
    protected void assertValidReplyCode(int replyCode) {
        Assert.isTrue(replyCode > 0, "The number [" + replyCode + "] is not a valid reply code");
    }

}