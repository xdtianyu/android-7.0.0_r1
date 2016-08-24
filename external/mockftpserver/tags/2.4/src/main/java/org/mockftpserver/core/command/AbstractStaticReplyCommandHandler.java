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

import org.mockftpserver.core.session.Session;

/**
 * The abstract superclass for CommandHandler classes that default to sending
 * back a configured reply code and text. You can customize the returned reply
 * code by setting the required <code>replyCode</code> property. If only the
 * <code>replyCode</code> property is set, then the default reply text corresponding to that
 * reply code is used in the response. You can optionally configure the reply text by setting
 * the <code>replyMessageKey</code> or <code>replyText</code> property.
 * <p>
 * Subclasses can optionally override the reply code and/or text for the reply by calling
 * {@link #setReplyCode(int)}, {@link #setReplyMessageKey(String)} and {@link #setReplyText(String)}.
 *
 * @version $Revision$ - $Date$
 *
 * @author Chris Mair
 */
public abstract class AbstractStaticReplyCommandHandler extends AbstractTrackingCommandHandler {

    // Defaults to zero; must be set to non-zero
    protected int replyCode = 0;

    // Defaults to null; if set to non-null, this value will override the default reply text associated with
    // the replyCode.
    protected String replyText = null;

    // The message key for the reply text. Defaults to null. If null, use the default message associated
    // with the reply code
    protected String replyMessageKey = null;

    /**
     * Set the reply code.
     *
     * @param replyCode - the replyCode
     *
     * @throws org.mockftpserver.core.util.AssertFailedException - if the replyCode is not valid
     */
    public void setReplyCode(int replyCode) {
        assertValidReplyCode(replyCode);
        this.replyCode = replyCode;
    }

    /**
     * Set the reply text. If null, then use the (default) message key for the replyCode.
     *
     * @param replyText - the replyText
     */
    public void setReplyText(String replyText) {
        this.replyText = replyText;
    }

    /**
     * Set the message key for the reply text. If null, then use the default message key.
     *
     * @param replyMessageKey - the replyMessageKey to set
     */
    public void setReplyMessageKey(String replyMessageKey) {
        this.replyMessageKey = replyMessageKey;
    }

    // -------------------------------------------------------------------------
    // Utility methods for subclasses
    // -------------------------------------------------------------------------

    /**
     * Send the reply using the replyCode and message key/text configured for this command handler.
     * @param session - the Session
     *
     * @throws org.mockftpserver.core.util.AssertFailedException if the replyCode is not valid
     */
    protected void sendReply(Session session) {
        sendReply(session, null);
    }

    /**
     * Send the reply using the replyCode and message key/text configured for this command handler.
     * @param session - the Session
     * @param messageParameter - message parameter; may be null
     *
     * @throws org.mockftpserver.core.util.AssertFailedException if the replyCode is not valid
     */
    protected void sendReply(Session session, Object messageParameter) {
        Object[] parameters = (messageParameter == null) ? null : new Object[] { messageParameter };
        sendReply(session, replyCode, replyMessageKey, replyText, parameters);
    }

}