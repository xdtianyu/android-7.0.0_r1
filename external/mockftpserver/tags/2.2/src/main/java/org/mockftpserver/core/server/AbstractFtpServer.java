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
package org.mockftpserver.core.server;

import org.apache.log4j.Logger;
import org.mockftpserver.core.MockFtpServerException;
import org.mockftpserver.core.command.Command;
import org.mockftpserver.core.command.CommandHandler;
import org.mockftpserver.core.session.DefaultSession;
import org.mockftpserver.core.session.Session;
import org.mockftpserver.core.socket.DefaultServerSocketFactory;
import org.mockftpserver.core.socket.ServerSocketFactory;
import org.mockftpserver.core.util.Assert;

import java.io.IOException;
import java.net.ServerSocket;
import java.net.Socket;
import java.net.SocketTimeoutException;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;
import java.util.ResourceBundle;

/**
 * This is the abstract superclass for "mock" implementations of an FTP Server,
 * suitable for testing FTP client code or standing in for a live FTP server. It supports
 * the main FTP commands by defining handlers for each of the corresponding low-level FTP
 * server commands (e.g. RETR, DELE, LIST). These handlers implement the {@link org.mockftpserver.core.command.CommandHandler}
 * interface.
 * <p/>
 * By default, mock FTP Servers bind to the server control port of 21. You can use a different server
 * control port by setting the the <code>serverControlPort</code> property. This is usually necessary
 * when running on Unix or some other system where that port number is already in use or cannot be bound
 * from a user process.
 * <p/>
 * <h4>Command Handlers</h4>
 * You can set the existing {@link CommandHandler} defined for an FTP server command
 * by calling the {@link #setCommandHandler(String, CommandHandler)} method, passing
 * in the FTP server command name and {@link CommandHandler} instance.
 * You can also replace multiple command handlers at once by using the {@link #setCommandHandlers(Map)}
 * method. That is especially useful when configuring the server through the <b>Spring Framework</b>.
 * <p/>
 * You can retrieve the existing {@link CommandHandler} defined for an FTP server command by
 * calling the {@link #getCommandHandler(String)} method, passing in the FTP server command name.
 * <p/>
 * <h4>FTP Command Reply Text ResourceBundle</h4>
 * The default text asociated with each FTP command reply code is contained within the
 * "ReplyText.properties" ResourceBundle file. You can customize these messages by providing a
 * locale-specific ResourceBundle file on the CLASSPATH, according to the normal lookup rules of
 * the ResourceBundle class (e.g., "ReplyText_de.properties"). Alternatively, you can
 * completely replace the ResourceBundle file by calling the calling the
 * {@link #setReplyTextBaseName(String)} method.
 *
 * @author Chris Mair
 * @version $Revision$ - $Date$
 * @see org.mockftpserver.fake.FakeFtpServer
 * @see org.mockftpserver.stub.StubFtpServer
 */
public abstract class AbstractFtpServer implements Runnable {

    /**
     * Default basename for reply text ResourceBundle
     */
    public static final String REPLY_TEXT_BASENAME = "ReplyText";
    private static final int DEFAULT_SERVER_CONTROL_PORT = 21;

    protected Logger LOG = Logger.getLogger(getClass());

    // Simple value object that holds the socket and thread for a single session
    private static class SessionInfo {
        Socket socket;
        Thread thread;
    }

    protected ServerSocketFactory serverSocketFactory = new DefaultServerSocketFactory();
    private ServerSocket serverSocket = null;
    private ResourceBundle replyTextBundle;
    private volatile boolean terminate = false;
    private Map commandHandlers;
    private Thread serverThread;
    private int serverControlPort = DEFAULT_SERVER_CONTROL_PORT;
    private final Object startLock = new Object();

    // Map of Session -> SessionInfo
    private Map sessions = new HashMap();

    /**
     * Create a new instance. Initialize the default command handlers and
     * reply text ResourceBundle.
     */
    public AbstractFtpServer() {
        replyTextBundle = ResourceBundle.getBundle(REPLY_TEXT_BASENAME);
        commandHandlers = new HashMap();
    }

    /**
     * Start a new Thread for this server instance
     */
    public void start() {
        serverThread = new Thread(this);

        synchronized (startLock) {
            try {
                // Start here in case server thread runs faster than main thread.
                // See https://sourceforge.net/tracker/?func=detail&atid=1006533&aid=1925590&group_id=208647
                serverThread.start();

                // Wait until the server thread is initialized
                startLock.wait();
            }
            catch (InterruptedException e) {
                e.printStackTrace();
                throw new MockFtpServerException(e);
            }
        }
    }

    /**
     * The logic for the server thread
     *
     * @see Runnable#run()
     */
    public void run() {
        try {
            LOG.info("Starting the server on port " + serverControlPort);
            serverSocket = serverSocketFactory.createServerSocket(serverControlPort);

            // Notify to allow the start() method to finish and return
            synchronized (startLock) {
                startLock.notify();
            }

            serverSocket.setSoTimeout(500);
            while (!terminate) {
                try {
                    Socket clientSocket = serverSocket.accept();
                    LOG.info("Connection accepted from host " + clientSocket.getInetAddress());

                    Session session = createSession(clientSocket);
                    Thread sessionThread = new Thread(session);
                    sessionThread.start();

                    SessionInfo sessionInfo = new SessionInfo();
                    sessionInfo.socket = clientSocket;
                    sessionInfo.thread = sessionThread;
                    sessions.put(session, sessionInfo);
                }
                catch (SocketTimeoutException socketTimeoutException) {
                    LOG.trace("Socket accept() timeout");
                }
            }
        }
        catch (IOException e) {
            LOG.error("Error", e);
        }
        finally {

            LOG.debug("Cleaning up server...");

            // Ensure that the start() method is not still blocked
            synchronized (startLock) {
                startLock.notifyAll();
            }

            try {
                if (serverSocket != null) {
                    serverSocket.close();
                }
                closeSessions();
            }
            catch (IOException e) {
                LOG.error("Error cleaning up server", e);
            }
            catch (InterruptedException e) {
                LOG.error("Error cleaning up server", e);
            }
            LOG.info("Server stopped.");
            terminate = false;
        }
    }

    /**
     * Stop this server instance and wait for it to terminate.
     */
    public void stop() {

        LOG.trace("Stopping the server...");
        terminate = true;

        try {
            if (serverThread != null) {
                serverThread.join();
            }
        }
        catch (InterruptedException e) {
            e.printStackTrace();
            throw new MockFtpServerException(e);
        }
    }

    /**
     * Return the CommandHandler defined for the specified command name
     *
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
     * @param commandHandlerMapping - the Map of commandName->CommandHandler; these override the defaults
     * @throws org.mockftpserver.core.util.AssertFailedException
     *          - if the commandHandlerMapping is null
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
     * the {@link org.mockftpserver.core.command.ReplyTextBundleAware} interface and its <code>replyTextBundle</code> attribute
     * is null, then set its <code>replyTextBundle</code> to the <code>replyTextBundle</code> of
     * this StubFtpServer.
     *
     * @param commandName    - the command name to which the CommandHandler will be associated
     * @param commandHandler - the CommandHandler
     * @throws org.mockftpserver.core.util.AssertFailedException
     *          - if the commandName or commandHandler is null
     */
    public void setCommandHandler(String commandName, CommandHandler commandHandler) {
        Assert.notNull(commandName, "commandName");
        Assert.notNull(commandHandler, "commandHandler");
        commandHandlers.put(Command.normalizeName(commandName), commandHandler);
        initializeCommandHandler(commandHandler);
    }

    /**
     * Set the reply text ResourceBundle to a new ResourceBundle with the specified base name,
     * accessible on the CLASSPATH. See {@link java.util.ResourceBundle#getBundle(String)}.
     *
     * @param baseName - the base name of the resource bundle, a fully qualified class name
     */
    public void setReplyTextBaseName(String baseName) {
        replyTextBundle = ResourceBundle.getBundle(baseName);
    }

    /**
     * Return the ReplyText ResourceBundle. Set the bundle through the  {@link #setReplyTextBaseName(String)}  method.
     *
     * @return the reply text ResourceBundle
     */
    public ResourceBundle getReplyTextBundle() {
        return replyTextBundle;
    }

    /**
     * Set the port number to which the server control connection socket will bind. The default value is 21.
     *
     * @param serverControlPort - the port number for the server control connection ServerSocket
     */
    public void setServerControlPort(int serverControlPort) {
        this.serverControlPort = serverControlPort;
    }

    /**
     * Return the port number to which the server control connection socket will bind. The default value is 21.
     *
     * @return the port number for the server control connection ServerSocket
     */
    public int getServerControlPort() {
        return serverControlPort;
    }

    /**
     * Return true if this server is fully shutdown -- i.e., there is no active (alive) threads and
     * all sockets are closed. This method is intended for testing only.
     *
     * @return true if this server is fully shutdown
     */
    public boolean isShutdown() {
        boolean shutdown = !serverThread.isAlive() && serverSocket.isClosed();

        for (Iterator iter = sessions.values().iterator(); iter.hasNext();) {
            SessionInfo sessionInfo = (SessionInfo) iter.next();
            shutdown = shutdown && sessionInfo.socket.isClosed() && !sessionInfo.thread.isAlive();
        }
        return shutdown;
    }

    /**
     * Return true if this server has started -- i.e., there is an active (alive) server threads
     * and non-null server socket. This method is intended for testing only.
     *
     * @return true if this server has started
     */
    public boolean isStarted() {
        return serverThread != null && serverThread.isAlive() && serverSocket != null;
    }

    //-------------------------------------------------------------------------
    // Internal Helper Methods
    //-------------------------------------------------------------------------

    /**
     * Create a new Session instance for the specified client Socket
     *
     * @param clientSocket - the Socket associated with the client
     * @return a Session
     */
    protected Session createSession(Socket clientSocket) {
        return new DefaultSession(clientSocket, commandHandlers);
    }

    private void closeSessions() throws InterruptedException, IOException {
        for (Iterator iter = sessions.entrySet().iterator(); iter.hasNext();) {
            Map.Entry entry = (Map.Entry) iter.next();
            Session session = (Session) entry.getKey();
            SessionInfo sessionInfo = (SessionInfo) entry.getValue();
            session.close();
            sessionInfo.thread.join(500L);
            Socket sessionSocket = sessionInfo.socket;
            if (sessionSocket != null) {
                sessionSocket.close();
            }
        }
    }

    //------------------------------------------------------------------------------------
    // Abstract method declarations
    //------------------------------------------------------------------------------------

    /**
     * Initialize a CommandHandler that has been registered to this server. What "initialization"
     * means is dependent on the subclass implementation.
     *
     * @param commandHandler - the CommandHandler to initialize
     */
    protected abstract void initializeCommandHandler(CommandHandler commandHandler);

}