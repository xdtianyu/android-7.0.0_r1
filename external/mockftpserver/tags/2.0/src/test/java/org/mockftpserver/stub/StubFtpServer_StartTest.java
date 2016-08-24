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

import org.mockftpserver.core.server.AbstractFtpServer;
import org.mockftpserver.core.server.AbstractFtpServer_StartTest;

/**
 * Tests for StubFtpServer that require the StubFtpServer thread to be started.
 *
 * @author Chris Mair
 * @version $Revision$ - $Date$
 */
public final class StubFtpServer_StartTest extends AbstractFtpServer_StartTest {

    //-------------------------------------------------------------------------
    // Abstract method implementations
    //-------------------------------------------------------------------------

    protected AbstractFtpServer createFtpServer() {
        return new StubFtpServer();
    }

}
