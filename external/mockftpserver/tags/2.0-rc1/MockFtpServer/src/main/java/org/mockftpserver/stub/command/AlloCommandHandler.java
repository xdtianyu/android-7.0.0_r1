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

import org.mockftpserver.core.command.Command;
import org.mockftpserver.core.command.CommandHandler;
import org.mockftpserver.core.command.InvocationRecord;
import org.mockftpserver.core.command.ReplyCodes;
import org.mockftpserver.core.session.Session;
import org.mockftpserver.core.util.Assert;

import java.util.StringTokenizer;

/**
 * CommandHandler for the ALLO (Allocate) command. Send back a reply code of 200.
 * <p/>
 * Each invocation record stored by this CommandHandler includes the following data element key/values:
 * <ul>
 * <li>{@link #NUMBER_OF_BYTES_KEY} ("numberOfBytes") - the number of bytes submitted
 * on the invocation (the first command parameter)
 * <li>{@link #RECORD_SIZE_KEY} ("recordSize") - the record size optionally submitted
 * on the invocation (the second command parameter)
 * </ul>
 *
 * @author Chris Mair
 * @version $Revision$ - $Date$
 */
public class AlloCommandHandler extends AbstractStubCommandHandler implements CommandHandler {

    public static final String NUMBER_OF_BYTES_KEY = "numberOfBytes";
    public static final String RECORD_SIZE_KEY = "recordSize";
    private static final String RECORD_SIZE_DELIMITER = " R ";

    /**
     * Constructor. Initialize the replyCode.
     */
    public AlloCommandHandler() {
        setReplyCode(ReplyCodes.ALLO_OK);
    }

    /**
     * @see org.mockftpserver.core.command.CommandHandler#handleCommand(org.mockftpserver.core.command.Command, org.mockftpserver.core.session.Session)
     */
    public void handleCommand(Command command, Session session, InvocationRecord invocationRecord) {
        String parametersString = command.getRequiredParameter(0);

        if (parametersString.indexOf(RECORD_SIZE_DELIMITER) == -1) {
            invocationRecord.set(NUMBER_OF_BYTES_KEY, Integer.valueOf(parametersString));
        } else {
            // If the recordSize delimiter (" R ") is specified, then it must be followed by the recordSize.
            StringTokenizer tokenizer = new StringTokenizer(parametersString, RECORD_SIZE_DELIMITER);
            invocationRecord.set(NUMBER_OF_BYTES_KEY, Integer.valueOf(tokenizer.nextToken()));
            Assert.isTrue(tokenizer.hasMoreTokens(), "Missing record size: [" + parametersString + "]");
            invocationRecord.set(RECORD_SIZE_KEY, Integer.valueOf(tokenizer.nextToken()));
        }

        sendReply(session);
    }

}
