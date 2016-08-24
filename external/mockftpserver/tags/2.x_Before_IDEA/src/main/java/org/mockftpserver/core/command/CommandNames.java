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

/**
 * FTP command name constants. 
 * 
 * @version $Revision$ - $Date$
 *
 * @author Chris Mair
 */
public final class CommandNames {

    public static final String ABOR = "ABOR";
    public static final String ACCT = "ACCT";
    public static final String ALLO = "ALLO";
    public static final String APPE = "APPE";
    public static final String CDUP = "CDUP";
    public static final String CONNECT = "CONNECT";
    public static final String CWD = "CWD";
    public static final String DELE = "DELE";
    public static final String HELP = "HELP";
    public static final String LIST = "LIST";
    public static final String MKD = "MKD";
    public static final String MODE = "MODE";
    public static final String NLST = "NLST";
    public static final String NOOP = "NOOP";
    public static final String PASS = "PASS";
    public static final String PASV = "PASV";
    public static final String PORT = "PORT";
    public static final String PWD = "PWD";
    public static final String QUIT = "QUIT";
    public static final String REIN = "REIN";
    public static final String REST = "REST";
    public static final String RETR = "RETR";
    public static final String RMD = "RMD";
    public static final String RNFR = "RNFR";
    public static final String RNTO = "RNTO";
    public static final String SITE = "SITE";
    public static final String SMNT = "SMNT";
    public static final String STAT = "STAT";
    public static final String STOR = "STOR";
    public static final String STOU = "STOU";
    public static final String STRU = "STRU";
    public static final String SYST = "SYST";
    public static final String TYPE = "TYPE";
    public static final String USER = "USER";

    public static final String XPWD = "XPWD";

    /**
     * Private constructor. This class should not be instantiated.
     */
    private CommandNames() {
    }

}
