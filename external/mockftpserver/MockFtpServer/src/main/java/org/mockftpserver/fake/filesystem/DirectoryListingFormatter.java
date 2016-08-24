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

package org.mockftpserver.fake.filesystem;

/**
 * Interface for an object that can format a file system directory listing.
 *
 * @author Chris Mair
 * @version $Revision$ - $Date$
 */
public interface DirectoryListingFormatter {

    /**
     * Format the directory listing for a single file/directory entry.
     *
     * @param fileSystemEntry - the FileSystemEntry for a single file system entry
     * @return the formatted directory listing
     */
    String format(FileSystemEntry fileSystemEntry);

}