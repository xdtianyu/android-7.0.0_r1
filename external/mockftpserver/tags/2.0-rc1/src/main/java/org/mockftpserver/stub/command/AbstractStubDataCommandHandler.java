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
import org.mockftpserver.core.command.Command;
import org.mockftpserver.core.command.CommandHandler;
import org.mockftpserver.core.command.InvocationRecord;
import org.mockftpserver.core.command.ReplyCodes;
import org.mockftpserver.core.session.Session;
import org.mockftpserver.core.util.AssertFailedException;

/**
 * Abstract superclass for CommandHandlers that read from or write to the data connection.
 * <p/>
 * Return two replies on the control connection: by default a reply code of 150 before the
 * data transfer across the data connection and another reply of 226 after the data transfer
 * is complete.
 * <p/>
 * This class implements the <i>Template Method</i> pattern. Subclasses must implement the abstract
 * {@link #processData()} method to perform read or writes across the data connection.
 * <p/>
 * Subclasses can optionally override the {@link #beforeProcessData(Command, Session, InvocationRecord)}
 * method for logic before the data transfer or the {@link #afterProcessData(Command, Session, InvocationRecord)}
 * method for logic after the data transfer.
 * <p/>
 * Subclasses can optionally override the reply code and/or text for the initial reply (before
 * the data transfer across the data connection) by calling {@link #setPreliminaryReplyCode(int)},
 * {@link #setPreliminaryReplyMessageKey(String)} and/or {@link #setPreliminaryReplyText(String)}
 * methods.
 * <p/>
 * Subclasses can optionally override the reply code and/or text for the final reply (after the
 * the data transfer is complete) by calling {@link #setFinalReplyCode(int)},
 * {@link #setFinalReplyMessageKey(String)} and/or {@link #setFinalReplyText(String)} methods.
 *
 * @author Chris Mair
 * @version $Revision$ - $Date$
 */
public abstract class AbstractStubDataCommandHandler extends AbstractCommandHandler implements CommandHandler {

    // The completion reply code sent before the data transfer
    protected int preliminaryReplyCode = 0;

    // The text for the preliminary reply. If null, use the default message associated with the reply code.
    // If not null, this value overrides the preliminaryReplyMessageKey - i.e., this text is used instead of
    // a localized message. 
    protected String preliminaryReplyText = null;

    // The message key for the preliminary reply text. If null, use the default message associated with 
    // the reply code.
    protected String preliminaryReplyMessageKey = null;

    // The completion reply code sent after data transfer
    protected int finalReplyCode = 0;

    // The text for the completion reply. If null, use the default message associated with the reply code.
    // If not null, this value overrides the finalReplyMessageKey - i.e., this text is used instead of
    // a localized message. 
    protected String finalReplyText = null;

    // The message key for the completion reply text. If null, use the default message associated with the reply code 
    protected String finalReplyMessageKey = null;

    /**
     * Constructor. Initialize the preliminary and final reply code.
     */
    protected AbstractStubDataCommandHandler() {
        setPreliminaryReplyCode(ReplyCodes.TRANSFER_DATA_INITIAL_OK);
        setFinalReplyCode(ReplyCodes.TRANSFER_DATA_FINAL_OK);
    }

    /**
     * Handle the command. Perform the following steps:
     * <ol>
     * <li>Invoke the {@link #beforeProcessData()} method</li>
     * <li>Open the data connection</li>
     * <li>Send an preliminary reply, default reply code 150</li>
     * <li>Invoke the {@link #processData()} method</li>
     * <li>Close the data connection</li>
     * <li>Send the final reply, default reply code 226</li>
     * <li>Invoke the {@link #afterProcessData()} method</li>
     * </ol>
     *
     * @see org.mockftpserver.core.command.CommandHandler#handleCommand(Command, Session, InvocationRecord)
     */
    public final void handleCommand(Command command, Session session, InvocationRecord invocationRecord) throws Exception {

        beforeProcessData(command, session, invocationRecord);

        sendPreliminaryReply(session);
        session.openDataConnection();
        processData(command, session, invocationRecord);
        session.closeDataConnection();
        sendFinalReply(session);

        afterProcessData(command, session, invocationRecord);
    }

    /**
     * Send the final reply. The default implementation sends a reply code of 226 with the
     * corresponding associated reply text.
     *
     * @param session - the Session
     */
    protected void sendFinalReply(Session session) {
        sendReply(session, finalReplyCode, finalReplyMessageKey, finalReplyText, null);
    }

    /**
     * Perform any necessary logic before transferring data across the data connection.
     * Do nothing by default. Subclasses should override to validate command parameters and
     * store information in the InvocationRecord.
     *
     * @param command          - the Command to be handled
     * @param session          - the session on which the Command was submitted
     * @param invocationRecord - the InvocationRecord; CommandHandlers are expected to add
     *                         handler-specific data to the InvocationRecord, as appropriate
     * @throws Exception
     */
    protected void beforeProcessData(Command command, Session session, InvocationRecord invocationRecord) throws Exception {
        // Do nothing by default
    }

    /**
     * Abstract method placeholder for subclass transfer of data across the data connection.
     * Subclasses must override. The data connection is opened before this method and is
     * closed after this method completes.
     *
     * @param command          - the Command to be handled
     * @param session          - the session on which the Command was submitted
     * @param invocationRecord - the InvocationRecord; CommandHandlers are expected to add
     *                         handler-specific data to the InvocationRecord, as appropriate
     * @throws Exception
     */
    protected abstract void processData(Command command, Session session, InvocationRecord invocationRecord) throws Exception;

    /**
     * Perform any necessary logic after transferring data across the data connection.
     * Do nothing by default.
     *
     * @param command          - the Command to be handled
     * @param session          - the session on which the Command was submitted
     * @param invocationRecord - the InvocationRecord; CommandHandlers are expected to add
     *                         handler-specific data to the InvocationRecord, as appropriate
     * @throws Exception
     */
    protected void afterProcessData(Command command, Session session, InvocationRecord invocationRecord) throws Exception {
        // Do nothing by default
    }

    /**
     * Send the preliminary reply for this command on the control connection.
     *
     * @param session - the Session
     */
    private void sendPreliminaryReply(Session session) {
        sendReply(session, preliminaryReplyCode, preliminaryReplyMessageKey, preliminaryReplyText, null);
    }

    /**
     * Set the completion reply code sent after data transfer
     *
     * @param finalReplyCode - the final reply code
     * @throws AssertFailedException - if the finalReplyCode is invalid
     */
    public void setFinalReplyCode(int finalReplyCode) {
        assertValidReplyCode(finalReplyCode);
        this.finalReplyCode = finalReplyCode;
    }

    /**
     * Set the message key for the completion reply text sent after data transfer
     *
     * @param finalReplyMessageKey - the final reply message key
     */
    public void setFinalReplyMessageKey(String finalReplyMessageKey) {
        this.finalReplyMessageKey = finalReplyMessageKey;
    }

    /**
     * Set the text of the completion reply sent after data transfer
     *
     * @param finalReplyText - the final reply text
     */
    public void setFinalReplyText(String finalReplyText) {
        this.finalReplyText = finalReplyText;
    }

    /**
     * Set the completion reply code sent before data transfer
     *
     * @param preliminaryReplyCode - the preliminary reply code to set
     * @throws AssertFailedException - if the preliminaryReplyCode is invalid
     */
    public void setPreliminaryReplyCode(int preliminaryReplyCode) {
        assertValidReplyCode(preliminaryReplyCode);
        this.preliminaryReplyCode = preliminaryReplyCode;
    }

    /**
     * Set the message key for the completion reply text sent before data transfer
     *
     * @param preliminaryReplyMessageKey - the preliminary reply message key
     */
    public void setPreliminaryReplyMessageKey(String preliminaryReplyMessageKey) {
        this.preliminaryReplyMessageKey = preliminaryReplyMessageKey;
    }

    /**
     * Set the text of the completion reply sent before data transfer
     *
     * @param preliminaryReplyText - the preliminary reply text
     */
    public void setPreliminaryReplyText(String preliminaryReplyText) {
        this.preliminaryReplyText = preliminaryReplyText;
    }

}
