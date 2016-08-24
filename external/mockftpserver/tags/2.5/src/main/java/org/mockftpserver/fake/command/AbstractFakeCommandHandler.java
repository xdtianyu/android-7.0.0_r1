/*
 * Copyright 2008 the original author or authors.
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
package org.mockftpserver.fake.command;

import org.mockftpserver.core.CommandSyntaxException;
import org.mockftpserver.core.IllegalStateException;
import org.mockftpserver.core.NotLoggedInException;
import org.mockftpserver.core.command.AbstractCommandHandler;
import org.mockftpserver.core.command.Command;
import org.mockftpserver.core.command.ReplyCodes;
import org.mockftpserver.core.session.Session;
import org.mockftpserver.core.session.SessionKeys;
import org.mockftpserver.core.util.Assert;
import org.mockftpserver.fake.ServerConfiguration;
import org.mockftpserver.fake.ServerConfigurationAware;
import org.mockftpserver.fake.UserAccount;
import org.mockftpserver.fake.filesystem.FileSystem;
import org.mockftpserver.fake.filesystem.FileSystemEntry;
import org.mockftpserver.fake.filesystem.FileSystemException;
import org.mockftpserver.fake.filesystem.InvalidFilenameException;

import java.text.MessageFormat;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.MissingResourceException;

/**
 * Abstract superclass for CommandHandler classes for the "Fake" server.
 *
 * @author Chris Mair
 * @version $Revision$ - $Date$
 */
public abstract class AbstractFakeCommandHandler extends AbstractCommandHandler implements ServerConfigurationAware {

    protected static final String INTERNAL_ERROR_KEY = "internalError";

    private ServerConfiguration serverConfiguration;

    /**
     * Reply code sent back when a FileSystemException is caught by the                 {@link #handleCommand(Command, Session)}
     * This defaults to ReplyCodes.EXISTING_FILE_ERROR (550).
     */
    protected int replyCodeForFileSystemException = ReplyCodes.READ_FILE_ERROR;

    public ServerConfiguration getServerConfiguration() {
        return serverConfiguration;
    }

    public void setServerConfiguration(ServerConfiguration serverConfiguration) {
        this.serverConfiguration = serverConfiguration;
    }

    /**
     * Use template method to centralize and ensure common validation
     */
    public void handleCommand(Command command, Session session) {
        Assert.notNull(serverConfiguration, "serverConfiguration");
        Assert.notNull(command, "command");
        Assert.notNull(session, "session");

        try {
            handle(command, session);
        }
        catch (CommandSyntaxException e) {
            handleException(command, session, e, ReplyCodes.COMMAND_SYNTAX_ERROR);
        }
        catch (IllegalStateException e) {
            handleException(command, session, e, ReplyCodes.ILLEGAL_STATE);
        }
        catch (NotLoggedInException e) {
            handleException(command, session, e, ReplyCodes.NOT_LOGGED_IN);
        }
        catch (InvalidFilenameException e) {
            handleFileSystemException(command, session, e, ReplyCodes.FILENAME_NOT_VALID, e.getPath());
        }
        catch (FileSystemException e) {
            handleFileSystemException(command, session, e, replyCodeForFileSystemException, e.getPath());
        }
    }

    /**
     * Convenience method to return the FileSystem stored in the ServerConfiguration
     *
     * @return the FileSystem
     */
    protected FileSystem getFileSystem() {
        return serverConfiguration.getFileSystem();
    }

    /**
     * Handle the specified command for the session. All checked exceptions are expected to be wrapped or handled
     * by the caller.
     *
     * @param command - the Command to be handled
     * @param session - the session on which the Command was submitted
     */
    protected abstract void handle(Command command, Session session);

    // -------------------------------------------------------------------------
    // Utility methods for subclasses
    // -------------------------------------------------------------------------

    /**
     * Send a reply for this command on the control connection.
     * <p/>
     * The reply code is designated by the <code>replyCode</code> property, and the reply text
     * is retrieved from the <code>replyText</code> ResourceBundle, using the specified messageKey.
     *
     * @param session    - the Session
     * @param replyCode  - the reply code
     * @param messageKey - the resource bundle key for the reply text
     * @throws AssertionError - if session is null
     * @see MessageFormat
     */
    protected void sendReply(Session session, int replyCode, String messageKey) {
        sendReply(session, replyCode, messageKey, Collections.EMPTY_LIST);
    }

    /**
     * Send a reply for this command on the control connection.
     * <p/>
     * The reply code is designated by the <code>replyCode</code> property, and the reply text
     * is retrieved from the <code>replyText</code> ResourceBundle, using the specified messageKey.
     *
     * @param session    - the Session
     * @param replyCode  - the reply code
     * @param messageKey - the resource bundle key for the reply text
     * @param args       - the optional message arguments; defaults to []
     * @throws AssertionError - if session is null
     * @see MessageFormat
     */
    protected void sendReply(Session session, int replyCode, String messageKey, List args) {
        Assert.notNull(session, "session");
        assertValidReplyCode(replyCode);

        String text = getTextForKey(messageKey);
        String replyText = (args != null && !args.isEmpty()) ? MessageFormat.format(text, args.toArray()) : text;

        String replyTextToLog = (replyText == null) ? "" : " " + replyText;
        String argsToLog = (args != null && !args.isEmpty()) ? (" args=" + args) : "";
        LOG.info("Sending reply [" + replyCode + replyTextToLog + "]" + argsToLog);
        session.sendReply(replyCode, replyText);
    }

    /**
     * Send a reply for this command on the control connection.
     * <p/>
     * The reply code is designated by the <code>replyCode</code> property, and the reply text
     * is retrieved from the <code>replyText</code> ResourceBundle, using the reply code as the key.
     *
     * @param session   - the Session
     * @param replyCode - the reply code
     * @throws AssertionError - if session is null
     * @see MessageFormat
     */
    protected void sendReply(Session session, int replyCode) {
        sendReply(session, replyCode, Collections.EMPTY_LIST);
    }

    /**
     * Send a reply for this command on the control connection.
     * <p/>
     * The reply code is designated by the <code>replyCode</code> property, and the reply text
     * is retrieved from the <code>replyText</code> ResourceBundle, using the reply code as the key.
     *
     * @param session   - the Session
     * @param replyCode - the reply code
     * @param args      - the optional message arguments; defaults to []
     * @throws AssertionError - if session is null
     * @see MessageFormat
     */
    protected void sendReply(Session session, int replyCode, List args) {
        sendReply(session, replyCode, Integer.toString(replyCode), args);
    }

    /**
     * Handle the exception caught during handleCommand()
     *
     * @param command   - the Command
     * @param session   - the Session
     * @param exception - the caught exception
     * @param replyCode - the reply code that should be sent back
     */
    private void handleException(Command command, Session session, Throwable exception, int replyCode) {
        LOG.warn("Error handling command: " + command + "; " + exception, exception);
        sendReply(session, replyCode);
    }

    /**
     * Handle the exception caught during handleCommand()
     *
     * @param command   - the Command
     * @param session   - the Session
     * @param exception - the caught exception
     * @param replyCode - the reply code that should be sent back
     * @param arg       - the arg for the reply (message)
     */
    private void handleFileSystemException(Command command, Session session, FileSystemException exception, int replyCode, Object arg) {
        LOG.warn("Error handling command: " + command + "; " + exception, exception);
        sendReply(session, replyCode, exception.getMessageKey(), Collections.singletonList(arg));
    }

    /**
     * Return the value of the named attribute within the session.
     *
     * @param session - the Session
     * @param name    - the name of the session attribute to retrieve
     * @return the value of the named session attribute
     * @throws IllegalStateException - if the Session does not contain the named attribute
     */
    protected Object getRequiredSessionAttribute(Session session, String name) {
        Object value = session.getAttribute(name);
        if (value == null) {
            throw new IllegalStateException("Session missing required attribute [" + name + "]");
        }
        return value;
    }

    /**
     * Verify that the current user (if any) has already logged in successfully.
     *
     * @param session - the Session
     */
    protected void verifyLoggedIn(Session session) {
        if (getUserAccount(session) == null) {
            throw new NotLoggedInException("User has not logged in");
        }
    }

    /**
     * @param session - the Session
     * @return the UserAccount stored in the specified session; may be null
     */
    protected UserAccount getUserAccount(Session session) {
        return (UserAccount) session.getAttribute(SessionKeys.USER_ACCOUNT);
    }

    /**
     * Verify that the specified condition related to the file system is true,
     * otherwise throw a FileSystemException.
     *
     * @param condition  - the condition that must be true
     * @param path       - the path involved in the operation; this will be included in the
     *                   error message if the condition is not true.
     * @param messageKey - the message key for the exception message
     * @throws FileSystemException - if the condition is not true
     */
    protected void verifyFileSystemCondition(boolean condition, String path, String messageKey) {
        if (!condition) {
            throw new FileSystemException(path, messageKey);
        }
    }

    /**
     * Verify that the current user has execute permission to the specified path
     *
     * @param session - the Session
     * @param path    - the file system path
     * @throws FileSystemException - if the condition is not true
     */
    protected void verifyExecutePermission(Session session, String path) {
        UserAccount userAccount = getUserAccount(session);
        FileSystemEntry entry = getFileSystem().getEntry(path);
        verifyFileSystemCondition(userAccount.canExecute(entry), path, "filesystem.cannotExecute");
    }

    /**
     * Verify that the current user has write permission to the specified path
     *
     * @param session - the Session
     * @param path    - the file system path
     * @throws FileSystemException - if the condition is not true
     */
    protected void verifyWritePermission(Session session, String path) {
        UserAccount userAccount = getUserAccount(session);
        FileSystemEntry entry = getFileSystem().getEntry(path);
        verifyFileSystemCondition(userAccount.canWrite(entry), path, "filesystem.cannotWrite");
    }

    /**
     * Verify that the current user has read permission to the specified path
     *
     * @param session - the Session
     * @param path    - the file system path
     * @throws FileSystemException - if the condition is not true
     */
    protected void verifyReadPermission(Session session, String path) {
        UserAccount userAccount = getUserAccount(session);
        FileSystemEntry entry = getFileSystem().getEntry(path);
        verifyFileSystemCondition(userAccount.canRead(entry), path, "filesystem.cannotRead");
    }

    /**
     * Return the full, absolute path for the specified abstract pathname.
     * If path is null, return the current directory (stored in the session). If
     * path represents an absolute path, then return path as is. Otherwise, path
     * is relative, so assemble the full path from the current directory
     * and the specified relative path.
     *
     * @param session - the Session
     * @param path    - the abstract pathname; may be null
     * @return the resulting full, absolute path
     */
    protected String getRealPath(Session session, String path) {
        String currentDirectory = (String) session.getAttribute(SessionKeys.CURRENT_DIRECTORY);
        if (path == null) {
            return currentDirectory;
        }
        if (getFileSystem().isAbsolute(path)) {
            return path;
        }
        return getFileSystem().path(currentDirectory, path);
    }

    /**
     * Return the end-of-line character(s) used when building multi-line responses
     *
     * @return "\r\n"
     */
    protected String endOfLine() {
        return "\r\n";
    }

    private String getTextForKey(String key) {
        String msgKey = (key != null) ? key : INTERNAL_ERROR_KEY;
        try {
            return getReplyTextBundle().getString(msgKey);
        }
        catch (MissingResourceException e) {
            // No reply text is mapped for the specified key
            LOG.warn("No reply text defined for key [" + msgKey + "]");
            return null;
        }
    }

    // -------------------------------------------------------------------------
    // Login Support (used by USER and PASS commands)
    // -------------------------------------------------------------------------

    /**
     * Validate the UserAccount for the specified username. If valid, return true. If the UserAccount does
     * not exist or is invalid, log an error message, send back a reply code of 530 with an appropriate
     * error message, and return false. A UserAccount is considered invalid if the homeDirectory property
     * is not set or is set to a non-existent directory.
     *
     * @param username - the username
     * @param session  - the session; used to send back an error reply if necessary
     * @return true only if the UserAccount for the named user is valid
     */
    protected boolean validateUserAccount(String username, Session session) {
        UserAccount userAccount = serverConfiguration.getUserAccount(username);
        if (userAccount == null || !userAccount.isValid()) {
            LOG.error("UserAccount missing or not valid for username [" + username + "]: " + userAccount);
            sendReply(session, ReplyCodes.USER_ACCOUNT_NOT_VALID, "login.userAccountNotValid", list(username));
            return false;
        }

        String home = userAccount.getHomeDirectory();
        if (!getFileSystem().isDirectory(home)) {
            LOG.error("Home directory configured for username [" + username + "] is not valid: " + home);
            sendReply(session, ReplyCodes.USER_ACCOUNT_NOT_VALID, "login.homeDirectoryNotValid", list(username, home));
            return false;
        }

        return true;
    }

    /**
     * Log in the specified user for the current session. Send back a reply of 230 with a message indicated
     * by the replyMessageKey and set the UserAccount and current directory (homeDirectory) in the session.
     *
     * @param userAccount     - the userAccount for the user to be logged in
     * @param session         - the session
     * @param replyCode       - the reply code to send
     * @param replyMessageKey - the message key for the reply text
     */
    protected void login(UserAccount userAccount, Session session, int replyCode, String replyMessageKey) {
        sendReply(session, replyCode, replyMessageKey);
        session.setAttribute(SessionKeys.USER_ACCOUNT, userAccount);
        session.setAttribute(SessionKeys.CURRENT_DIRECTORY, userAccount.getHomeDirectory());
    }

    /**
     * Convenience method to return a List with the specified single item
     *
     * @param item - the single item in the returned List
     * @return a new List with that single item
     */
    protected List list(Object item) {
        return Collections.singletonList(item);
    }

    /**
     * Convenience method to return a List with the specified two items
     *
     * @param item1 - the first item in the returned List
     * @param item2 - the second item in the returned List
     * @return a new List with the specified items
     */
    protected List list(Object item1, Object item2) {
        List list = new ArrayList(2);
        list.add(item1);
        list.add(item2);
        return list;
    }

    /**
     * Return true if the specified string is null or empty
     *
     * @param string - the String to check; may be null
     * @return true only if the specified String is null or empyt
     */
    protected boolean notNullOrEmpty(String string) {
        return string != null && string.length() > 0;
    }

    /**
     * Return the string unless it is null or empty, in which case return the defaultString.
     *
     * @param string        - the String to check; may be null
     * @param defaultString - the value to return if string is null or empty
     * @return string if not null and not empty; otherwise return defaultString
     */
    protected String defaultIfNullOrEmpty(String string, String defaultString) {
        return (notNullOrEmpty(string) ? string : defaultString);
    }

}