/*
 * Copyright 2008 the original author or authors.
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

import org.mockftpserver.fake.ServerConfiguration
import org.mockftpserver.fake.UserAccount
import org.mockftpserver.fake.filesystem.FileSystem

/**
 * Stub implementation of the   {@link org.mockftpserver.fake.ServerConfiguration}   interface for testing
 *
 * @version $Revision$ - $Date$
 *
 * @author Chris Mair
 */
class StubServerConfiguration implements ServerConfiguration {

    Map userAccounts = [:]
    Map helpText = [:]
    FileSystem fileSystem
    String systemName = "WINDOWS"
    String systemStatus

    UserAccount getUserAccount(String username) {
        (UserAccount) userAccounts[username]
    }

    public String getHelpText(String name) {
        def key = name == null ? '' : name
        return helpText[key]
    }

}
