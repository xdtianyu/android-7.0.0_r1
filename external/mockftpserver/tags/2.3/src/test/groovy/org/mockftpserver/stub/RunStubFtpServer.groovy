/*
 * Copyright 2009 the original author or authors.
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
package org.mockftpserver.stub

import org.mockftpserver.stub.StubFtpServer
import org.mockftpserver.test.PortTestUtil
import org.mockftpserver.stub.command.PwdCommandHandler
import org.mockftpserver.core.command.CommandNames

/**
 * Run the StubFtpServer with a minimal configuration for interactive testing and exploration.
 *
 * @version $Revision$ - $Date$
 *
 * @author Chris Mair
 */
class RunStubFtpServer {

    static main(args) {
        def stubFtpServer = new StubFtpServer();
        stubFtpServer.setServerControlPort(PortTestUtil.getFtpServerControlPort());

        stubFtpServer.getCommandHandler(CommandNames.PWD).setDirectory("/MyDir");

        final LISTING = "11-09-01 12:30PM  406348 File2350.log\n" + "11-01-01 1:30PM <DIR> 0 archive"
        stubFtpServer.getCommandHandler(CommandNames.LIST).setDirectoryListing(LISTING)
        
        stubFtpServer.start();
    }
}