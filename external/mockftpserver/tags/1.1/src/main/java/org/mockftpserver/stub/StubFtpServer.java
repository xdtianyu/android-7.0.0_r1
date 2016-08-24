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

import java.io.IOException;
import java.net.ServerSocket;
import java.net.Socket;
import java.net.SocketTimeoutException;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;
import java.util.ResourceBundle;

import org.apache.log4j.Logger;
import org.mockftpserver.core.MockFtpServerException;
import org.mockftpserver.core.command.Command;
import org.mockftpserver.core.command.CommandHandler;
import org.mockftpserver.core.command.CommandNames;
import org.mockftpserver.core.command.ReplyTextBundleAware;
import org.mockftpserver.core.command.ReplyTextBundleUtil;
import org.mockftpserver.core.session.DefaultSession;
import org.mockftpserver.core.session.Session;
import org.mockftpserver.core.socket.DefaultServerSocketFactory;
import org.mockftpserver.core.socket.ServerSocketFactory;
import org.mockftpserver.core.util.Assert;
import org.mockftpserver.core.util.AssertFailedException;
import org.mockftpserver.stub.command.AborCommandHandler;
import org.mockftpserver.stub.command.AcctCommandHandler;
import org.mockftpserver.stub.command.AlloCommandHandler;
import org.mockftpserver.stub.command.AppeCommandHandler;
import org.mockftpserver.stub.command.CdupCommandHandler;
import org.mockftpserver.stub.command.ConnectCommandHandler;
import org.mockftpserver.stub.command.CwdCommandHandler;
import org.mockftpserver.stub.command.DeleCommandHandler;
import org.mockftpserver.stub.command.HelpCommandHandler;
import org.mockftpserver.stub.command.ListCommandHandler;
import org.mockftpserver.stub.command.MkdCommandHandler;
import org.mockftpserver.stub.command.ModeCommandHandler;
import org.mockftpserver.stub.command.NlstCommandHandler;
import org.mockftpserver.stub.command.NoopCommandHandler;
import org.mockftpserver.stub.command.PassCommandHandler;
import org.mockftpserver.stub.command.PasvCommandHandler;
import org.mockftpserver.stub.command.PortCommandHandler;
import org.mockftpserver.stub.command.PwdCommandHandler;
import org.mockftpserver.stub.command.QuitCommandHandler;
import org.mockftpserver.stub.command.ReinCommandHandler;
import org.mockftpserver.stub.command.RestCommandHandler;
import org.mockftpserver.stub.command.RetrCommandHandler;
import org.mockftpserver.stub.command.RmdCommandHandler;
import org.mockftpserver.stub.command.RnfrCommandHandler;
import org.mockftpserver.stub.command.RntoCommandHandler;
import org.mockftpserver.stub.command.SiteCommandHandler;
import org.mockftpserver.stub.command.SmntCommandHandler;
import org.mockftpserver.stub.command.StatCommandHandler;
import org.mockftpserver.stub.command.StorCommandHandler;
import org.mockftpserver.stub.command.StouCommandHandler;
import org.mockftpserver.stub.command.StruCommandHandler;
import org.mockftpserver.stub.command.SystCommandHandler;
import org.mockftpserver.stub.command.TypeCommandHandler;
import org.mockftpserver.stub.command.UserCommandHandler;

/**
 * <b>StubFtpServer</b> is the top-level class for a "stub" implementation of an FTP Server, 
 * suitable for testing FTP client code or standing in for a live FTP server. It supports 
 * the main FTP commands by defining handlers for each of the corresponding low-level FTP 
 * server commands (e.g. RETR, DELE, LIST). These handlers implement the {@link CommandHandler} 
 * interface. 
 * <p>
 * <b>StubFtpServer</b> works out of the box with default command handlers that return
 * success reply codes and empty data (for retrieved files, directory listings, etc.). 
 * The command handler for any command can be easily configured to return custom data
 * or reply codes. Or it can be replaced with a custom {@link CommandHandler} 
 * implementation. This allows simulation of a complete range of both success and 
 * failure scenarios. The command handlers can also be interrogated to verify command 
 * invocation data such as command parameters and timestamps.
 * <p>
 * <b>StubFtpServer</b> can be fully configured programmatically or within a Spring Framework 
 * ({@link http://www.springframework.org/}) or similar container.
 * <p>
 * <h4>Starting the StubFtpServer</h4>
 * Here is how to start the <b>StubFtpServer</b> with the default configuration. 
 * <pre><code>
 * StubFtpServer stubFtpServer = new StubFtpServer();
 * stubFtpServer.start();
 * </code></pre>
 * <p>
 * <h4>Retrieving Command Handlers</h4>
 * You can retrieve the existing {@link CommandHandler} defined for an FTP server command 
 * by calling the {@link #getCommandHandler(String)} method, passing in the FTP server 
 * command name. For example:
 * <pre><code>
 * PwdCommandHandler pwdCommandHandler = (PwdCommandHandler) stubFtpServer.getCommandHandler("PWD");
 * </code></pre>
 * <p>
 * <h4>Replacing Command Handlers</h4>
 * You can replace the existing {@link CommandHandler} defined for an FTP server command 
 * by calling the {@link #setCommandHandler(String, CommandHandler)} method, passing 
 * in the FTP server command name and {@link CommandHandler} instance. For example:
 * <pre><code>
 * PwdCommandHandler pwdCommandHandler = new PwdCommandHandler();
 * pwdCommandHandler.setDirectory("some/dir");
 * stubFtpServer.setCommandHandler("PWD", pwdCommandHandler);
 * </code></pre>
 * You can also replace multiple command handlers at once by using the {@link #setCommandHandlers(Map)}
 * method. That is especially useful when configuring the server through the <b>Spring Framework</b>.
 * <h4>FTP Command Reply Text ResourceBundle</h4>
 * <p>
 * The default text asociated with each FTP command reply code is contained within the
 * "ReplyText.properties" ResourceBundle file. You can customize these messages by providing a
 * locale-specific ResourceBundle file on the CLASSPATH, according to the normal lookup rules of 
 * the ResourceBundle class (e.g., "ReplyText_de.properties"). Alternatively, you can 
 * completely replace the ResourceBundle file by calling the calling the 
 * {@link #setReplyTextBaseName(String)} method. 
 * 
 * @version $Revision$ - $Date$
 * 
 * @author Chris Mair
 */
public final class StubFtpServer implements Runnable {

    /** Default basename for reply text ResourceBundle */
    public static final String REPLY_TEXT_BASENAME = "ReplyText"; 
    private static final int DEFAULT_SERVER_CONTROL_PORT = 21;

    private static Logger LOG = Logger.getLogger(StubFtpServer.class);

    // Simple value object that holds the socket and thread for a single session
    private static class SessionInfo {
        private Socket socket;
        private Thread thread;
    }
    
    private ServerSocketFactory serverSocketFactory = new DefaultServerSocketFactory();
    private ServerSocket serverSocket = null;
    ResourceBundle replyTextBundle;             // non-private for testing only
    private volatile boolean terminate = false;
    private Map commandHandlers;
    private Thread serverThread;
    private int serverControlPort = DEFAULT_SERVER_CONTROL_PORT;
        
    // Map of Session -> SessionInfo
    private Map sessions = new HashMap(); 

    /**
     * Create a new instance. Initialize the default command handlers and
     * reply text ResourceBundle.
     */
    public StubFtpServer() {
        replyTextBundle = ResourceBundle.getBundle(REPLY_TEXT_BASENAME);
        
        commandHandlers = new HashMap();
        
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
        setCommandHandler(CommandNames.XPWD, pwdCommandHandler);           // same as PWD
    }

    /**
     * Start a new Thread for this server instance
     */
    public void start() {
        serverThread = new Thread(this);
        serverThread.start();
    }
    
    /**
     * The logic for the server thread
     * @see java.lang.Runnable#run()
     */
    public void run() {
        try {
            LOG.info("Starting the server on port " + serverControlPort);
            serverSocket = serverSocketFactory.createServerSocket(serverControlPort);

            serverSocket.setSoTimeout(500);
            while(!terminate) {
                try {
                    Socket clientSocket = serverSocket.accept();
                    LOG.info("Connection accepted from host " + clientSocket.getInetAddress());

                    DefaultSession session = new DefaultSession(clientSocket, commandHandlers);
                    Thread sessionThread = new Thread(session);
                    sessionThread.start();
                    
                    SessionInfo sessionInfo = new SessionInfo();
                    sessionInfo.socket = clientSocket;
                    sessionInfo.thread = sessionThread;
                    sessions.put(session, sessionInfo);
                } 
                catch(SocketTimeoutException socketTimeoutException) {
                    LOG.trace("Socket accept() timeout");
                }
            }
        }
        catch (IOException e) {
            LOG.error("Error", e);
        }
        finally {
            
            LOG.debug("Cleaning up server...");
            
            try {
                serverSocket.close();
                
                for (Iterator iter = sessions.keySet().iterator(); iter.hasNext();) {
                    Session session = (Session) iter.next();
                    SessionInfo sessionInfo = (SessionInfo) sessions.get(session);
                    session.close();
                    sessionInfo.thread.join(500L);
                    Socket sessionSocket = (Socket) sessionInfo.socket;
                    if (sessionSocket != null) {
                        sessionSocket.close();
                    }
                }
            }
            catch (IOException e) {
                e.printStackTrace();
                throw new MockFtpServerException(e);
            }
            catch (InterruptedException e) {
                e.printStackTrace();
                throw new MockFtpServerException(e);
            }
            LOG.info("Server stopped.");
        }
    }

    /**
     * Stop this server instance and wait for it to terminate.
     */
    public void stop() {

        LOG.trace("Stopping the server...");
        terminate = true;

        try {
            serverThread.join();
        }
        catch (InterruptedException e) {
            e.printStackTrace();
            throw new MockFtpServerException(e);
        }
    }

    /**
     * Return the CommandHandler defined for the specified command name
     * @param name - the command name
     * @return the CommandHandler defined for name
     */
    public CommandHandler getCommandHandler(String name) {
        return (CommandHandler) commandHandlers.get(Command.normalizeName(name));
    }
    
    /**
     * Override the default CommandHandlers with those in the specified Map of 
     * commandName>>CommandHandler. This will only override the default CommandHandlers 
     * for the keys in <code>commandHandlerMapping</code>. All other default CommandHandler 
     * mappings remain unchanged.
     * 
     * @param commandHandlers - the Map of commandName->CommandHandler; these override the defaults
     * 
     * @throws AssertFailedException - if the commandHandlerMapping is null
     */
    public void setCommandHandlers(Map commandHandlerMapping) {
        Assert.notNull(commandHandlerMapping, "commandHandlers");
        for (Iterator iter = commandHandlerMapping.keySet().iterator(); iter.hasNext();) {
            String commandName = (String) iter.next();
            setCommandHandler(commandName, (CommandHandler) commandHandlerMapping.get(commandName));
        }
    }

    /**
     * Set the CommandHandler for the specified command name. If the CommandHandler implements
     * the {@link ReplyTextBundleAware} interface and its <code>replyTextBundle</code> attribute
     * is null, then set its <code>replyTextBundle</code> to the <code>replyTextBundle</code> of
     * this StubFtpServer.
     *  
     * @param commandName - the command name to which the CommandHandler will be associated
     * @param commandHandler - the CommandHandler
     * 
     * @throws AssertFailedException - if the commandName or commandHandler is null
     */
    public void setCommandHandler(String commandName, CommandHandler commandHandler) {
        Assert.notNull(commandName, "commandName");
        Assert.notNull(commandHandler, "commandHandler");
        commandHandlers.put(Command.normalizeName(commandName), commandHandler);
        ReplyTextBundleUtil.setReplyTextBundleIfAppropriate(commandHandler, replyTextBundle);
    }
    
    /**
     * Set the reply text ResourceBundle to a new ResourceBundle with the specified base name,
     * accessible on the CLASSPATH. See {@link ResourceBundle#getBundle(String)}.
     * @param baseName - the base name of the resource bundle, a fully qualified class name
     */
    public void setReplyTextBaseName(String baseName) {
        replyTextBundle = ResourceBundle.getBundle(baseName);
    }
    
    /**
     * Set the port number to which the server control connection socket will bind. The default value is 21.
     * @param serverControlPort - the port number for the server control connection ServerSocket
     */
    public void setServerControlPort(int serverControlPort) {
        this.serverControlPort = serverControlPort;
    }
    
    //-------------------------------------------------------------------------
    // Internal Helper Methods
    //-------------------------------------------------------------------------

    /**
     * Return true if this server is fully shutdown -- i.e., there is no active (alive) threads and 
     * all sockets are closed. This method is intended for testing only.
     * @return true if this server is fully shutdown
     */
    boolean isShutdown() {
        boolean shutdown = !serverThread.isAlive() && serverSocket.isClosed();
        
        for (Iterator iter = sessions.keySet().iterator(); iter.hasNext();) {
            SessionInfo sessionInfo = (SessionInfo) iter.next();
            shutdown = shutdown && sessionInfo.socket.isClosed() && !sessionInfo.thread.isAlive(); 
        }
        return shutdown;
    }
    
    /**
     * Return true if this server has started -- i.e., there is an active (alive) server threads 
     * and non-null server socket. This method is intended for testing only.
     * @return true if this server has started
     */
    boolean isStarted() {
        return serverThread != null && serverThread.isAlive() && serverSocket != null;
    }

}