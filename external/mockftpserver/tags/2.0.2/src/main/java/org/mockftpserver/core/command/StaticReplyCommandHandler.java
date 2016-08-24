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
import org.mockftpserver.core.util.AssertFailedException;

/**
 * CommandHandler that sends back the configured reply code and text. You can customize the 
 * returned reply code by setting the required <code>replyCode</code> property. If only the
 * <code>replyCode</code> property is set, then the default reply text corresponding to that
 * reply code is used in the response. You can optionally configure the reply text by setting
 * the <code>replyMessageKey</code> or <code>replyText</code> property.
 * <p>
 * Each invocation record stored by this CommandHandler contains no data elements.
 * 
 * @version $Revision$ - $Date$
 * 
 * @author Chris Mair
 */
public final class StaticReplyCommandHandler extends AbstractStaticReplyCommandHandler {

    /**
     * Create a new uninitialized instance
     */
    public StaticReplyCommandHandler() {
    }
    
    /**
     * Create a new instance with the specified replyCode
     * @param replyCode - the replyCode to use
     * @throws AssertFailedException - if the replyCode is null
     */
    public StaticReplyCommandHandler(int replyCode) {
        setReplyCode(replyCode);
    }
    
    /**
     * Create a new instance with the specified replyCode and replyText
     * @param replyCode - the replyCode to use
     * @param replyText - the replyText
     * @throws AssertFailedException - if the replyCode is null
     */
    public StaticReplyCommandHandler(int replyCode, String replyText) {
        setReplyCode(replyCode);
        setReplyText(replyText);
    }

    /**
     * @see AbstractTrackingCommandHandler#handleCommand(Command, org.mockftpserver.core.session.Session, InvocationRecord)
     */
    public void handleCommand(Command command, Session session, InvocationRecord invocationRecord) {
        sendReply(session);
    }

}
