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
 * Reply Code constants. 
 * 
 * @version $Revision$ - $Date$
 *
 * @author Chris Mair
 */
public final class ReplyCodes {

    public static final int ABOR_OK = 226;
    public static final int ACCT_OK = 230;
    public static final int ALLO_OK = 200;
    public static final int CDUP_OK = 250;
    public static final int CWD_OK = 250;
    public static final int DELE_OK = 250;
    public static final int HELP_OK = 214;
    public static final int MKD_OK = 257;
    public static final int MODE_OK = 200;
    public static final int NOOP_OK = 200;
    public static final int PASS_OK = 230;
    public static final int PASS_LOG_IN_FAILED = 530;
    public static final int PASV_OK = 227;
    public static final int PORT_OK = 200;
    public static final int PWD_OK = 257;
    public static final int QUIT_OK = 221;
    public static final int REIN_OK = 220;
    public static final int REST_OK = 350;
    public static final int RMD_OK = 250;
    public static final int RNFR_OK = 350;
    public static final int RNTO_OK = 250;
    public static final int SITE_OK = 200;
    public static final int SMNT_OK = 250;
    public static final int STAT_SYSTEM_OK = 211;
    public static final int STAT_FILE_OK = 213;
    public static final int STRU_OK = 200;
    public static final int SYST_OK = 215;
    public static final int TYPE_OK = 200;
    public static final int USER_LOGGED_IN_OK = 230;
    public static final int USER_NEED_PASSWORD_OK = 331;
    public static final int USER_NO_SUCH_USER = 530;
    
    public static final int SEND_DATA_INITIAL_OK = 150;
    public static final int SEND_DATA_FINAL_OK = 226;

    public static final int CONNECT_OK = 220;

    // GENERIC
    public static final int COMMAND_SYNTAX_ERROR = 501;
    public static final int ILLEGAL_STATE = 503;       // Bad sequence
    public static final int NOT_LOGGED_IN = 530;
    public static final int EXISTING_FILE_ERROR = 550;
    public static final int NEW_FILE_ERROR = 553;
    public static final int FILENAME_NOT_VALID = 553;
    
    
    /**
     * Private constructor. This class should not be instantiated.
     */
    private ReplyCodes() {
    }

}
