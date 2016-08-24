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

import org.mockftpserver.core.command.AbstractCommandHandler;
import org.mockftpserver.core.session.Session;
import org.mockftpserver.core.util.AssertFailedException;
import org.mockftpserver.stub.StubFtpServer;

/**
 * The abstract superclass for CommandHandler classes for the {@link StubFtpServer}.
 * <p>
 * Subclasses can optionally override the reply code and/or text for the reply by calling
 * {@link #setReplyCode(int)}, {@link #setReplyMessageKey(String)} and {@link #setReplyText(String)}.
 * 
 * @version $Revision$ - $Date$
 * 
 * @author Chris Mair
 */
public abstract class AbstractStubCommandHandler extends AbstractCommandHandler {

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
     * @throws AssertFailedException - if the replyCode is not valid
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
     * @throws AssertFailedException if the replyCode is not valid
     */
    protected void sendReply(Session session) {
        sendReply(session, null);
    }
    
    /**
     * Send the reply using the replyCode and message key/text configured for this command handler.
     * @param session - the Session
     * @param messageParameter - message parameter; may be null
     * 
     * @throws AssertFailedException if the replyCode is not valid
     */
    protected void sendReply(Session session, Object messageParameter) {
        Object[] parameters = (messageParameter == null) ? null : new Object[] { messageParameter };
        sendReply(session, replyCode, replyMessageKey, replyText, parameters);
    }

}
