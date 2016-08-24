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
package org.mockftpserver.stub.example;

import org.apache.commons.net.ftp.FTPClient;

import java.io.ByteArrayOutputStream;
import java.io.IOException;

/**
 * Simple FTP client code example.
 *
 * @author Chris Mair
 * @version $Revision$ - $Date$
 */
public class RemoteFile {

    public static final String USERNAME = "user";
    public static final String PASSWORD = "password";

    private String server;
    private int port;

    public String readFile(String filename) throws IOException {

        FTPClient ftpClient = new FTPClient();
        ftpClient.connect(server, port);
        ftpClient.login(USERNAME, PASSWORD);

        ByteArrayOutputStream outputStream = new ByteArrayOutputStream();
        boolean success = ftpClient.retrieveFile(filename, outputStream);
        ftpClient.disconnect();

        if (!success) {
            throw new IOException("Retrieve file failed: " + filename);
        }
        return outputStream.toString();
    }

    /**
     * Set the hostname of the FTP server
     *
     * @param server - the hostname of the FTP server
     */
    public void setServer(String server) {
        this.server = server;
    }

    /**
     * Set the port number for the FTP server
     *
     * @param port - the port number
     */
    public void setPort(int port) {
        this.port = port;
    }

    // Other methods ...
}
