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

import org.mockftpserver.stub.StubFtpServer;
import org.mockftpserver.test.AbstractTest;

/**
 * Tests for StubFtpServer that require the StubFtpServer thread to be started. 
 * 
 * @version $Revision$ - $Date$
 * 
 * @author Chris Mair
 */
public final class StubFtpServer_StartTest extends AbstractTest {

    private StubFtpServer stubFtpServer;
    
    /**
     * Test the start() and stop() methods. Start the StubFtpServer and then stop it immediately. 
     */
    public void testStartAndStop() throws Exception {
        stubFtpServer = new StubFtpServer();
        assertEquals("started - before", false, stubFtpServer.isStarted());
        
        stubFtpServer.start();
        Thread.sleep(200L);     // give it some time to get started
        assertEquals("started - after start()", true, stubFtpServer.isStarted());
        assertEquals("shutdown - after start()", false, stubFtpServer.isShutdown());

        stubFtpServer.stop();
        
        assertEquals("shutdown - after stop()", true, stubFtpServer.isShutdown());
    }
    
}
