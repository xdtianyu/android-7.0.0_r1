/*
 * Copyright 2010 the original author or authors.
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
package org.mockftpserver.fake

import org.mockftpserver.test.AbstractGroovyTestCase
import org.mockftpserver.test.PortTestUtil

class FakeFtpServer_AlreadyStartedTest extends AbstractGroovyTestCase {
    private FakeFtpServer ftpServer1 = new FakeFtpServer()
    private FakeFtpServer ftpServer2 = new FakeFtpServer()

    void testStartServer_WhenAlreadyStarted() {
        ftpServer1.setServerControlPort(PortTestUtil.getFtpServerControlPort())
        ftpServer1.start();
        Thread.sleep(200L);     // give it some time to get started
        assertEquals("started - after start()", true, ftpServer1.isStarted());

        ftpServer2.setServerControlPort(PortTestUtil.getFtpServerControlPort())
        ftpServer2.start();
        log("started ftpServer2")
        sleep(200L)      // give it a chance to start and terminate
        assert !ftpServer2.isStarted()
    }

    void tearDown() {
        super.tearDown()
        ftpServer1.stop();
        ftpServer2.stop();
    }
}
