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
package org.mockftpserver.core.session;

import org.apache.log4j.Logger;
import org.mockftpserver.core.MockFtpServerException;
import org.mockftpserver.core.command.Command;
import org.mockftpserver.core.command.CommandHandler;
import org.mockftpserver.core.command.CommandNames;
import org.mockftpserver.core.socket.DefaultServerSocketFactory;
import org.mockftpserver.core.socket.DefaultSocketFactory;
import org.mockftpserver.core.socket.ServerSocketFactory;
import org.mockftpserver.core.socket.SocketFactory;
import org.mockftpserver.core.util.Assert;
import org.mockftpserver.core.util.AssertFailedException;

import java.io.BufferedReader;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.io.PrintWriter;
import java.io.Writer;
import java.net.InetAddress;
import java.net.ServerSocket;
import java.net.Socket;
import java.net.SocketTimeoutException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.StringTokenizer;

/**
 * Default implementation of the {@link Session} interface.
 *
 * @author Chris Mair
 * @version $Revision$ - $Date$
 */
public class DefaultSession implements Session {

    private static final Logger LOG = Logger.getLogger(DefaultSession.class);
    private static final String END_OF_LINE = "\r\n";
    static final int DEFAULT_CLIENT_DATA_PORT = 21;

    SocketFactory socketFactory = new DefaultSocketFactory();
    ServerSocketFactory serverSocketFactory = new DefaultServerSocketFactory();

    private BufferedReader controlConnectionReader;
    private Writer controlConnectionWriter;
    private Socket controlSocket;
    private Socket dataSocket;
    ServerSocket passiveModeDataSocket; // non-private for testing
    private InputStream dataInputStream;
    private OutputStream dataOutputStream;
    private Map commandHandlers;
    private int clientDataPort = DEFAULT_CLIENT_DATA_PORT;
    private InetAddress clientHost;
    private InetAddress serverHost;
    private Map attributes = new HashMap();
    private volatile boolean terminate = false;

    /**
     * Create a new initialized instance
     *
     * @param controlSocket   - the control connection socket
     * @param commandHandlers - the Map of command name -> CommandHandler. It is assumed that the
     *                        command names are all normalized to upper case. See {@link Command#normalizeName(String)}.
     */
    public DefaultSession(Socket controlSocket, Map commandHandlers) {
        Assert.notNull(controlSocket, "controlSocket");
        Assert.notNull(commandHandlers, "commandHandlers");

        this.controlSocket = controlSocket;
        this.commandHandlers = commandHandlers;
        this.serverHost = controlSocket.getLocalAddress();
    }

    /**
     * Return the InetAddress representing the client host for this session
     *
     * @return the client host
     * @see org.mockftpserver.core.session.Session#getClientHost()
     */
    public InetAddress getClientHost() {
        return controlSocket.getInetAddress();
    }

    /**
     * Return the InetAddress representing the server host for this session
     *
     * @return the server host
     * @see org.mockftpserver.core.session.Session#getServerHost()
     */
    public InetAddress getServerHost() {
        return serverHost;
    }

    /**
     * Send the specified reply code and text across the control connection.
     * The reply text is trimmed before being sent.
     *
     * @param code - the reply code
     * @param text - the reply text to send; may be null
     */
    public void sendReply(int code, String text) {
        assertValidReplyCode(code);

        StringBuffer buffer = new StringBuffer(Integer.toString(code));

        if (text != null && text.length() > 0) {
            String replyText = text.trim();
            if (replyText.indexOf("\n") != -1) {
                int lastIndex = replyText.lastIndexOf("\n");
                buffer.append("-");
                for (int i = 0; i < replyText.length(); i++) {
                    char c = replyText.charAt(i);
                    buffer.append(c);
                    if (i == lastIndex) {
                        buffer.append(Integer.toString(code));
                        buffer.append(" ");
                    }
                }
            } else {
                buffer.append(" ");
                buffer.append(replyText);
            }
        }
        LOG.debug("Sending Reply [" + buffer.toString() + "]");
        writeLineToControlConnection(buffer.toString());
    }

    /**
     * @see org.mockftpserver.core.session.Session#openDataConnection()
     */
    public void openDataConnection() {
        try {
            if (passiveModeDataSocket != null) {
                LOG.debug("Waiting for (passive mode) client connection from client host [" + clientHost
                        + "] on port " + passiveModeDataSocket.getLocalPort());
                // TODO set socket timeout
                try {
                    dataSocket = passiveModeDataSocket.accept();
                    LOG.debug("Successful (passive mode) client connection to port "
                            + passiveModeDataSocket.getLocalPort());
                }
                catch (SocketTimeoutException e) {
                    throw new MockFtpServerException(e);
                }
            } else {
                Assert.notNull(clientHost, "clientHost");
                LOG.debug("Connecting to client host [" + clientHost + "] on data port [" + clientDataPort
                        + "]");
                dataSocket = socketFactory.createSocket(clientHost, clientDataPort);
            }
            dataOutputStream = dataSocket.getOutputStream();
            dataInputStream = dataSocket.getInputStream();
        }
        catch (IOException e) {
            throw new MockFtpServerException(e);
        }
    }

    /**
     * Switch to passive mode
     *
     * @return the local port to be connected to by clients for data transfers
     * @see org.mockftpserver.core.session.Session#switchToPassiveMode()
     */
    public int switchToPassiveMode() {
        try {
            passiveModeDataSocket = serverSocketFactory.createServerSocket(0);
            return passiveModeDataSocket.getLocalPort();
        }
        catch (IOException e) {
            throw new MockFtpServerException("Error opening passive mode server data socket", e);
        }
    }

    /**
     * @see org.mockftpserver.core.session.Session#closeDataConnection()
     */
    public void closeDataConnection() {
        try {
            LOG.debug("Flushing and closing client data socket");
            dataOutputStream.flush();
            dataOutputStream.close();
            dataInputStream.close();
            dataSocket.close();
        }
        catch (IOException e) {
            LOG.error("Error closing client data socket", e);
        }
    }

    /**
     * Write a single line to the control connection, appending a newline
     *
     * @param line - the line to write
     */
    private void writeLineToControlConnection(String line) {
        try {
            controlConnectionWriter.write(line + END_OF_LINE);
            controlConnectionWriter.flush();
        }
        catch (IOException e) {
            LOG.error("Error writing to control connection", e);
            throw new MockFtpServerException("Error writing to control connection", e);
        }
    }

    /**
     * @see org.mockftpserver.core.session.Session#close()
     */
    public void close() {
        LOG.trace("close()");
        terminate = true;
    }

    /**
     * @see org.mockftpserver.core.session.Session#sendData(byte[], int)
     */
    public void sendData(byte[] data, int numBytes) {
        Assert.notNull(data, "data");
        try {
            dataOutputStream.write(data, 0, numBytes);
        }
        catch (IOException e) {
            throw new MockFtpServerException(e);
        }
    }

    /**
     * @see org.mockftpserver.core.session.Session#readData()
     */
    public byte[] readData() {

        ByteArrayOutputStream bytes = new ByteArrayOutputStream();

        try {
            while (true) {
                int b = dataInputStream.read();
                if (b == -1) {
                    break;
                }
                bytes.write(b);
            }
            return bytes.toByteArray();
        }
        catch (IOException e) {
            throw new MockFtpServerException(e);
        }
    }

    /**
     * Wait for and read the command sent from the client on the control connection.
     *
     * @return the Command sent from the client; may be null if the session has been closed
     *         <p/>
     *         Package-private to enable testing
     */
    Command readCommand() {

        final long socketReadIntervalMilliseconds = 100L;

        try {
            while (true) {
                if (terminate) {
                    return null;
                }
                // Don't block; only read command when it is available
                if (controlConnectionReader.ready()) {
                    String command = controlConnectionReader.readLine();
                    LOG.info("Received command: [" + command + "]");
                    return parseCommand(command);
                }
                try {
                    Thread.sleep(socketReadIntervalMilliseconds);
                }
                catch (InterruptedException e) {
                    throw new MockFtpServerException(e);
                }
            }
        }
        catch (IOException e) {
            LOG.error("Read failed", e);
            throw new MockFtpServerException(e);
        }
    }

    /**
     * Parse the command String into a Command object
     *
     * @param commandString - the command String
     * @return the Command object parsed from the command String
     */
    Command parseCommand(String commandString) {
        Assert.notNullOrEmpty(commandString, "commandString");

        List parameters = new ArrayList();
        String name;

        int indexOfFirstSpace = commandString.indexOf(" ");
        if (indexOfFirstSpace != -1) {
            name = commandString.substring(0, indexOfFirstSpace);
            StringTokenizer tokenizer = new StringTokenizer(commandString.substring(indexOfFirstSpace + 1),
                    ",");
            while (tokenizer.hasMoreTokens()) {
                parameters.add(tokenizer.nextToken());
            }
        } else {
            name = commandString;
        }

        String[] parametersArray = new String[parameters.size()];
        return new Command(name, (String[]) parameters.toArray(parametersArray));
    }

    /**
     * @see org.mockftpserver.core.session.Session#setClientDataHost(java.net.InetAddress)
     */
    public void setClientDataHost(InetAddress clientHost) {
        this.clientHost = clientHost;
    }

    /**
     * @see org.mockftpserver.core.session.Session#setClientDataPort(int)
     */
    public void setClientDataPort(int dataPort) {
        this.clientDataPort = dataPort;

        // Clear out any passive data connection mode information
        if (passiveModeDataSocket != null) {
            try {
                this.passiveModeDataSocket.close();
            }
            catch (IOException e) {
                throw new MockFtpServerException(e);
            }
            passiveModeDataSocket = null;
        }
    }

    /**
     * @see java.lang.Runnable#run()
     */
    public void run() {
        try {

            InputStream inputStream = controlSocket.getInputStream();
            OutputStream outputStream = controlSocket.getOutputStream();
            controlConnectionReader = new BufferedReader(new InputStreamReader(inputStream));
            controlConnectionWriter = new PrintWriter(outputStream, true);

            LOG.debug("Starting the session...");

            CommandHandler connectCommandHandler = (CommandHandler) commandHandlers.get(CommandNames.CONNECT);
            connectCommandHandler.handleCommand(new Command(CommandNames.CONNECT, new String[0]), this);

            while (!terminate) {
                readAndProcessCommand();
            }
        }
        catch (Exception e) {
            LOG.error(e);
            throw new MockFtpServerException(e);
        }
        finally {
            LOG.debug("Cleaning up the session");
            try {
                controlConnectionReader.close();
                controlConnectionWriter.close();
            }
            catch (IOException e) {
                LOG.error(e);
            }
            LOG.debug("Session stopped.");
        }
    }

    /**
     * Read and process the next command from the control connection
     *
     * @throws Exception - if any error occurs
     */
    private void readAndProcessCommand() throws Exception {

        Command command = readCommand();
        if (command != null) {
            String normalizedCommandName = Command.normalizeName(command.getName());
            CommandHandler commandHandler = (CommandHandler) commandHandlers.get(normalizedCommandName);
            Assert.notNull(commandHandler, "CommandHandler for command [" + normalizedCommandName + "]");
            commandHandler.handleCommand(command, this);
        }
    }

    /**
     * Assert that the specified number is a valid reply code
     *
     * @param replyCode - the reply code to check
     */
    private void assertValidReplyCode(int replyCode) {
        Assert.isTrue(replyCode > 0, "The number [" + replyCode + "] is not a valid reply code");
    }

    /**
     * Return the attribute value for the specified name. Return null if no attribute value
     * exists for that name or if the attribute value is null.
     *
     * @param name - the attribute name; may not be null
     * @return the value of the attribute stored under name; may be null
     * @see org.mockftpserver.core.session.Session#getAttribute(java.lang.String)
     */
    public Object getAttribute(String name) {
        Assert.notNull(name, "name");
        return attributes.get(name);
    }

    /**
     * Store the value under the specified attribute name.
     *
     * @param name  - the attribute name; may not be null
     * @param value - the attribute value; may be null
     * @see org.mockftpserver.core.session.Session#setAttribute(java.lang.String, java.lang.Object)
     */
    public void setAttribute(String name, Object value) {
        Assert.notNull(name, "name");
        attributes.put(name, value);
    }

    /**
     * Return the Set of names under which attributes have been stored on this session.
     * Returns an empty Set if no attribute values are stored.
     *
     * @return the Set of attribute names
     * @see org.mockftpserver.core.session.Session#getAttributeNames()
     */
    public Set getAttributeNames() {
        return attributes.keySet();
    }

    /**
     * Remove the attribute value for the specified name. Do nothing if no attribute
     * value is stored for the specified name.
     *
     * @param name - the attribute name; may not be null
     * @throws AssertFailedException - if name is null
     * @see org.mockftpserver.core.session.Session#removeAttribute(java.lang.String)
     */
    public void removeAttribute(String name) {
        Assert.notNull(name, "name");
        attributes.remove(name);
    }

}
