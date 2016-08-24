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
import org.mockftpserver.core.CommandSyntaxException;
import org.mockftpserver.core.session.Session;
import org.mockftpserver.core.util.Assert;
import org.mockftpserver.core.util.AssertFailedException;

import java.text.MessageFormat;
import java.util.ArrayList;
import java.util.List;
import java.util.MissingResourceException;
import java.util.ResourceBundle;

/**
 * The abstract superclass for CommandHandler classes. This class manages the List of
 * InvocationRecord objects corresponding to each invocation of the command handler,
 * and provides helper methods for subclasses.
 *
 * @author Chris Mair
 * @version $Revision$ - $Date$
 */
public abstract class AbstractCommandHandler implements CommandHandler, ReplyTextBundleAware, InvocationHistory {

    private static final Logger LOG = Logger.getLogger(AbstractCommandHandler.class);

    private ResourceBundle replyTextBundle;
    private List invocations = new ArrayList();

    // -------------------------------------------------------------------------
    // Template Method
    // -------------------------------------------------------------------------

    /**
     * Handle the specified command for the session. This method is declared to throw Exception,
     * allowing CommandHandler implementations to avoid unnecessary exception-handling. All checked
     * exceptions are expected to be wrapped and handled by the caller.
     *
     * @param command - the Command to be handled
     * @param session - the session on which the Command was submitted
     * @throws Exception
     * @throws AssertFailedException - if the command or session is null
     * @see org.mockftpserver.core.command.CommandHandler#handleCommand(org.mockftpserver.core.command.Command,
     *      org.mockftpserver.core.session.Session)
     */
    public final void handleCommand(Command command, Session session) throws Exception {
        Assert.notNull(command, "command");
        Assert.notNull(session, "session");
        InvocationRecord invocationRecord = new InvocationRecord(command, session.getClientHost());
        invocations.add(invocationRecord);
        try {
            handleCommand(command, session, invocationRecord);
        }
        catch (CommandSyntaxException e) {
            sendReply(session, ReplyCodes.COMMAND_SYNTAX_ERROR, null, null, null);
        }
        invocationRecord.lock();
    }

    /**
     * Handle the specified command for the session. This method is declared to throw Exception,
     * allowing CommandHandler implementations to avoid unnecessary exception-handling. All checked
     * exceptions are expected to be wrapped and handled by the caller.
     *
     * @param command          - the Command to be handled
     * @param session          - the session on which the Command was submitted
     * @param invocationRecord - the InvocationRecord; CommandHandlers are expected to add
     *                         handler-specific data to the InvocationRecord, as appropriate
     * @throws Exception
     */
    protected abstract void handleCommand(Command command, Session session, InvocationRecord invocationRecord)
            throws Exception;

    //-------------------------------------------------------------------------
    // Support for reply text ResourceBundle
    //-------------------------------------------------------------------------

    /**
     * Return the ResourceBundle containing the reply text messages
     *
     * @return the replyTextBundle
     * @see org.mockftpserver.core.command.ReplyTextBundleAware#getReplyTextBundle()
     */
    public ResourceBundle getReplyTextBundle() {
        return replyTextBundle;
    }

    /**
     * Set the ResourceBundle containing the reply text messages
     *
     * @param replyTextBundle - the replyTextBundle to set
     * @see org.mockftpserver.core.command.ReplyTextBundleAware#setReplyTextBundle(java.util.ResourceBundle)
     */
    public void setReplyTextBundle(ResourceBundle replyTextBundle) {
        this.replyTextBundle = replyTextBundle;
    }

    // -------------------------------------------------------------------------
    // Utility methods for subclasses
    // -------------------------------------------------------------------------

    /**
     * Send a reply for this command on the control connection.
     * <p/>
     * The reply code is designated by the <code>replyCode</code> property, and the reply text
     * is determined by the following rules:
     * <ol>
     * <li>If the <code>replyText</code> property is non-null, then use that.</li>
     * <li>Otherwise, if <code>replyMessageKey</code> is non-null, the use that to retrieve a
     * localized message from the <code>replyText</code> ResourceBundle.</li>
     * <li>Otherwise, retrieve the reply text from the <code>replyText</code> ResourceBundle,
     * using the reply code as the key.</li>
     * </ol>
     * If the arguments Object[] is not null, then these arguments are substituted within the
     * reply text using the {@link MessageFormat} class.
     *
     * @param session         - the Session
     * @param replyCode       - the reply code
     * @param replyMessageKey - if not null (and replyText is null), this is used as the ResourceBundle
     *                        message key instead of the reply code.
     * @param replyText       - if non-null, this is used as the reply text
     * @param arguments       - the array of arguments to be formatted and substituted within the reply
     *                        text; may be null
     * @throws AssertFailedException - if session is null
     * @see MessageFormat
     */
    protected void sendReply(Session session, int replyCode, String replyMessageKey, String replyText,
                             Object[] arguments) {

        Assert.notNull(session, "session");
        assertValidReplyCode(replyCode);

        final Logger HANDLER_LOG = Logger.getLogger(getClass());
        String key = (replyMessageKey != null) ? replyMessageKey : Integer.toString(replyCode);
        String text = getTextForReplyCode(replyCode, key, replyText, arguments);
        String replyTextToLog = (text == null) ? "" : " " + text;
        HANDLER_LOG.debug("Sending reply [" + replyCode + replyTextToLog + "]");
        session.sendReply(replyCode, text);
    }

    /**
     * Return the specified text surrounded with double quotes
     *
     * @param text - the text to surround with quotes
     * @return the text with leading and trailing double quotes
     * @throws AssertFailedException - if text is null
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
     * @throws AssertFailedException - if the replyCode is invalid
     */
    protected void assertValidReplyCode(int replyCode) {
        Assert.isTrue(replyCode > 0, "The number [" + replyCode + "] is not a valid reply code");
    }

    // -------------------------------------------------------------------------
    // InvocationHistory - Support for command history
    // -------------------------------------------------------------------------

    /**
     * @return the number of invocation records stored for this command handler instance
     * @see org.mockftpserver.core.command.InvocationHistory#numberOfInvocations()
     */
    public int numberOfInvocations() {
        return invocations.size();
    }

    /**
     * Return the InvocationRecord representing the command invoction data for the nth invocation
     * for this command handler instance. One InvocationRecord should be stored for each invocation
     * of the CommandHandler.
     *
     * @param index - the index of the invocation record to return. The first record is at index zero.
     * @return the InvocationRecord for the specified index
     * @throws AssertFailedException - if there is no invocation record corresponding to the specified index
     * @see org.mockftpserver.core.command.InvocationHistory#getInvocation(int)
     */
    public InvocationRecord getInvocation(int index) {
        return (InvocationRecord) invocations.get(index);
    }

    /**
     * Clear out the invocation history for this CommandHandler. After invoking this method, the
     * <code>numberOfInvocations()</code> method will return zero.
     *
     * @see org.mockftpserver.core.command.InvocationHistory#clearInvocations()
     */
    public void clearInvocations() {
        invocations.clear();
    }

    // -------------------------------------------------------------------------
    // Internal Helper Methods
    // -------------------------------------------------------------------------

    /**
     * Return the text for the specified reply code, formatted using the message arguments, if
     * supplied. If overrideText is not null, then return that. Otherwise, return the text mapped to
     * the code from the replyText ResourceBundle. If the ResourceBundle contains no mapping, then
     * return null.
     * <p/>
     * If arguments is not null, then the returned reply text if formatted using the
     * {@link MessageFormat} class.
     *
     * @param code         - the reply code
     * @param messageKey   - the key used to retrieve the reply text from the replyTextBundle
     * @param overrideText - if not null, this is used instead of the text from the replyTextBundle.
     * @param arguments    - the array of arguments to be formatted and substituted within the reply
     *                     text; may be null
     * @return the text for the reply code; may be null
     */
    private String getTextForReplyCode(int code, String messageKey, String overrideText, Object[] arguments) {
        try {
            String t = (overrideText == null) ? replyTextBundle.getString(messageKey) : overrideText;
            String formattedMessage = MessageFormat.format(t, arguments);
            return (formattedMessage == null) ? null : formattedMessage.trim();
        }
        catch (MissingResourceException e) {
            // No reply text is mapped for the specified key
            LOG.warn("No reply text defined for reply code [" + code + "]");
            return null;
        }
    }

}
