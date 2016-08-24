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
package org.mockftpserver.stub;

import org.mockftpserver.core.command.CommandHandler;
import org.mockftpserver.core.command.CommandNames;
import org.mockftpserver.core.command.ConnectCommandHandler;
import org.mockftpserver.core.command.ReplyTextBundleUtil;
import org.mockftpserver.core.command.UnsupportedCommandHandler;
import org.mockftpserver.core.server.AbstractFtpServer;
import org.mockftpserver.stub.command.*;

/**
 * <b>StubFtpServer</b> is the top-level class for a "stub" implementation of an FTP Server,
 * suitable for testing FTP client code or standing in for a live FTP server. It supports
 * the main FTP commands by defining handlers for each of the corresponding low-level FTP
 * server commands (e.g. RETR, DELE, LIST). These handlers implement the {@link CommandHandler}
 * interface.
 * <p/>
 * <b>StubFtpServer</b> works out of the box with default command handlers that return
 * success reply codes and empty data (for retrieved files, directory listings, etc.).
 * The command handler for any command can be easily configured to return custom data
 * or reply codes. Or it can be replaced with a custom {@link CommandHandler}
 * implementation. This allows simulation of a complete range of both success and
 * failure scenarios. The command handlers can also be interrogated to verify command
 * invocation data such as command parameters and timestamps.
 * <p/>
 * <b>StubFtpServer</b> can be fully configured programmatically or within the
 * <a href="http://www.springframework.org/">Spring Framework</a> or similar container.
 * <p/>
 * <h4>Starting the StubFtpServer</h4>
 * Here is how to start the <b>StubFtpServer</b> with the default configuration.
 * <pre><code>
 * StubFtpServer stubFtpServer = new StubFtpServer();
 * stubFtpServer.start();
 * </code></pre>
 * <p/>
 * <h4>FTP Server Control Port</h4>
 * By default, <b>StubFtpServer</b> binds to the server control port of 21. You can use a different server control
 * port by setting the <code>serverControlPort</code> property. If you specify a value of <code>0</code>,
 * then a free port number will be chosen automatically; call <code>getServerControlPort()</code> AFTER
 * <code>start()</code> has been called to determine the actual port number being used. Using a non-default
 * port number is usually necessary when running on Unix or some other system where that port number is
 * already in use or cannot be bound from a user process.
 * <p/>
 * <h4>Retrieving Command Handlers</h4>
 * You can retrieve the existing {@link CommandHandler} defined for an FTP server command
 * by calling the {@link #getCommandHandler(String)} method, passing in the FTP server
 * command name. For example:
 * <pre><code>
 * PwdCommandHandler pwdCommandHandler = (PwdCommandHandler) stubFtpServer.getCommandHandler("PWD");
 * </code></pre>
 * <p/>
 * <h4>Replacing Command Handlers</h4>
 * You can replace the existing {@link CommandHandler} defined for an FTP server command
 * by calling the {@link #setCommandHandler(String, CommandHandler)} method, passing
 * in the FTP server command name and {@link CommandHandler} instance. For example:
 * <pre><code>
 * PwdCommandHandler pwdCommandHandler = new PwdCommandHandler();
 * pwdCommandHandler.setDirectory("some/dir");
 * stubFtpServer.setCommandHandler("PWD", pwdCommandHandler);
 * </code></pre>
 * You can also replace multiple command handlers at once by using the {@link #setCommandHandlers(java.util.Map)}
 * method. That is especially useful when configuring the server through the <b>Spring Framework</b>.
 * <h4>FTP Command Reply Text ResourceBundle</h4>
 * <p/>
 * The default text asociated with each FTP command reply code is contained within the
 * "ReplyText.properties" ResourceBundle file. You can customize these messages by providing a
 * locale-specific ResourceBundle file on the CLASSPATH, according to the normal lookup rules of
 * the ResourceBundle class (e.g., "ReplyText_de.properties"). Alternatively, you can
 * completely replace the ResourceBundle file by calling the calling the
 * {@link #setReplyTextBaseName(String)} method.
 *
 * @author Chris Mair
 * @version $Revision$ - $Date$
 */
public class StubFtpServer extends AbstractFtpServer {

    /**
     * Create a new instance. Initialize the default command handlers and
     * reply text ResourceBundle.
     */
    public StubFtpServer() {
        PwdCommandHandler pwdCommandHandler = new PwdCommandHandler();

        // Initialize the default CommandHandler mappings
        setCommandHandler(CommandNames.ABOR, new AborCommandHandler());
        setCommandHandler(CommandNames.ACCT, new AcctCommandHandler());
        setCommandHandler(CommandNames.ALLO, new AlloCommandHandler());
        setCommandHandler(CommandNames.APPE, new AppeCommandHandler());
        setCommandHandler(CommandNames.PWD, pwdCommandHandler);            // same as XPWD
        setCommandHandler(CommandNames.CONNECT, new ConnectCommandHandler());
        setCommandHandler(CommandNames.CWD, new CwdCommandHandler());
        setCommandHandler(CommandNames.CDUP, new CdupCommandHandler());
        setCommandHandler(CommandNames.DELE, new DeleCommandHandler());
        setCommandHandler(CommandNames.EPRT, new EprtCommandHandler());
        setCommandHandler(CommandNames.EPSV, new EpsvCommandHandler());
        setCommandHandler(CommandNames.HELP, new HelpCommandHandler());
        setCommandHandler(CommandNames.LIST, new ListCommandHandler());
        setCommandHandler(CommandNames.MKD, new MkdCommandHandler());
        setCommandHandler(CommandNames.MODE, new ModeCommandHandler());
        setCommandHandler(CommandNames.NOOP, new NoopCommandHandler());
        setCommandHandler(CommandNames.NLST, new NlstCommandHandler());
        setCommandHandler(CommandNames.PASS, new PassCommandHandler());
        setCommandHandler(CommandNames.PASV, new PasvCommandHandler());
        setCommandHandler(CommandNames.PORT, new PortCommandHandler());
        setCommandHandler(CommandNames.RETR, new RetrCommandHandler());
        setCommandHandler(CommandNames.QUIT, new QuitCommandHandler());
        setCommandHandler(CommandNames.REIN, new ReinCommandHandler());
        setCommandHandler(CommandNames.REST, new RestCommandHandler());
        setCommandHandler(CommandNames.RMD, new RmdCommandHandler());
        setCommandHandler(CommandNames.RNFR, new RnfrCommandHandler());
        setCommandHandler(CommandNames.RNTO, new RntoCommandHandler());
        setCommandHandler(CommandNames.SITE, new SiteCommandHandler());
        setCommandHandler(CommandNames.SMNT, new SmntCommandHandler());
        setCommandHandler(CommandNames.STAT, new StatCommandHandler());
        setCommandHandler(CommandNames.STOR, new StorCommandHandler());
        setCommandHandler(CommandNames.STOU, new StouCommandHandler());
        setCommandHandler(CommandNames.STRU, new StruCommandHandler());
        setCommandHandler(CommandNames.SYST, new SystCommandHandler());
        setCommandHandler(CommandNames.TYPE, new TypeCommandHandler());
        setCommandHandler(CommandNames.USER, new UserCommandHandler());
        setCommandHandler(CommandNames.UNSUPPORTED, new UnsupportedCommandHandler());
        setCommandHandler(CommandNames.XPWD, pwdCommandHandler);           // same as PWD
    }

    //-------------------------------------------------------------------------
    // Abstract method implementation
    //-------------------------------------------------------------------------

    protected void initializeCommandHandler(CommandHandler commandHandler) {
        ReplyTextBundleUtil.setReplyTextBundleIfAppropriate(commandHandler, getReplyTextBundle());
    }

}