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

import org.mockftpserver.fake.FakeFtpServer
import org.mockftpserver.fake.UserAccount
import org.mockftpserver.fake.filesystem.DirectoryEntry
import org.mockftpserver.fake.filesystem.FileEntry
import org.mockftpserver.fake.filesystem.UnixFakeFileSystem


/**
 * Run the FakeFtpServer with a minimal configuration for interactive testing and exploration.
 *
 * @version $Revision: 83 $ - $Date: 2008-07-16 20:32:04 -0400 (Wed, 16 Jul 2008) $
 *
 * @author Chris Mair
 */
class RunFakeFtpServer {

    static final ANONYMOUS = 'anonymous'
    static final HOME_DIR = '/home'

    static main(args) {
        def fileSystem = new UnixFakeFileSystem()
        fileSystem.createParentDirectoriesAutomatically = true
        fileSystem.add(new DirectoryEntry(HOME_DIR))
        fileSystem.add(new DirectoryEntry("$HOME_DIR/subdir"))
        fileSystem.add(new FileEntry(path: "$HOME_DIR/abc.txt", contents: '1234567890'))
        fileSystem.add(new FileEntry(path: "$HOME_DIR/def.txt", contents: '1234567890'))

        def userAccount = new UserAccount(username: ANONYMOUS, passwordRequiredForLogin: false, homeDirectory: HOME_DIR)

        def ftpServer = new FakeFtpServer()
        ftpServer.fileSystem = fileSystem
        ftpServer.userAccounts = [userAccount]
        ftpServer.run()
    }
}