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

import org.mockftpserver.core.command.ReplyCodes;
import org.mockftpserver.core.session.Session;

/**
 * CommandHandler for the STOU (Store Unique) command. Send back two replies on the control connection: a
 * reply code of 150 and another of 226. The text accompanying the final reply (226) is the
 * unique filename, which is "" by default. You can customize the returned filename by setting
 * the <code>filename</code> property.
 * <p/>
 * Each invocation record stored by this CommandHandler includes the following data element key/values:
 * <ul>
 * <li>{@link #FILE_CONTENTS_KEY} ("fileContents") - the file contents (<code>byte[]</code>) sent on the data connection
 * </ul>
 *
 * @author Chris Mair
 * @version $Revision$ - $Date$
 */
public class StouCommandHandler extends AbstractStorCommandHandler {

    private static final String FINAL_REPLY_TEXT_KEY = "226.WithFilename";

    private String filename = "";

    /**
     * Override the default implementation to send a custom reply text that includes the STOU response filename
     *
     * @see org.mockftpserver.stub.command.AbstractStubDataCommandHandler#sendFinalReply(org.mockftpserver.core.session.Session)
     */
    protected void sendFinalReply(Session session) {
        final String[] ARGS = {filename};
        sendReply(session, ReplyCodes.TRANSFER_DATA_FINAL_OK, FINAL_REPLY_TEXT_KEY, null, ARGS);
    }

    /**
     * Set the filename returned with the final reply of the STOU command
     *
     * @param filename - the filename
     */
    public void setFilename(String filename) {
        this.filename = filename;
    }

}
