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
package org.mockftpserver.fake.filesystem

/**
 * File system entry representing a directory
 * 
 * @version $Revision: $ - $Date: $
 *
 * @author Chris Mair
 */
class DirectoryEntry extends AbstractFileSystemEntry {

    /**
     * Construct a new instance without setting its path 
     */
    DirectoryEntry() {
    }

    /**
     * Construct a new instance with the specified value for its path 
     * @param path - the value for path
     */
    DirectoryEntry(String path) {
        super(path)
    }

    /**
     * Abstract method -- must be implemented within concrete subclasses
     * @return true if this file system entry represents a directory
     */
    boolean isDirectory() {
        return true
    }
    
    /**
     * @see java.lang.Object#toString()
     */
    String toString() {
        "Directory['${getPath()}' lastModified=$lastModified]"
    }
    
}
