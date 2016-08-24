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

import java.io.ByteArrayInputStream
import java.io.ByteArrayOutputStream
import java.io.IOException
import java.io.InputStream
import java.io.OutputStream

/**
 * File system entry representing a file
 * 
 * @version $Revision: $ - $Date: $
 *
 * @author Chris Mair
 */
public class FileEntry extends AbstractFileSystemEntry {

    private static final EMPTY = new byte[0] 
    
    private byte[] bytes = EMPTY
    private ByteArrayOutputStream out
    
    /**
     * Construct a new instance without setting its path 
     */
    FileEntry() {
    }

    /**
     * Construct a new instance with the specified value for its path 
     * @param path - the value for path
     */
    FileEntry(String path) {
        super(path)
    }

    /**
     * Construct a new instance with the specified path and file contents 
     * @param path - the value for path
     * @param contents - the contents of the file, as a String
     */
    FileEntry(String path, String contents) {
        super(path)
        setContents(contents)
    }

    /**
     * Abstract method -- must be implemented within concrete subclasses
     * @return true if this file system entry represents a directory
     */
    boolean isDirectory() {
        return false
    }
    
    /**
     * Return the size of this file
     * @return the file size in bytes
     */
    long getSize() {
        return getCurrentBytes().length
    }
    
    /**
     * Set the contents of the file represented by this entry
     * @param contents - the String whose bytes are used as the contents
     */
    void setContents(String contents) {
        assert contents != null
        setContentsInternal(contents.getBytes())
    }
    
    /**
     * Set the contents of the file represented by this entry
     * @param contents - the byte[] used as the contents
     */
    void setContents(byte[] contents) {
        assert contents != null
        // Copy the bytes[] to guard against subsequent modification of the source array
        setContentsInternal(new String(contents).getBytes())
    }
    
    /**
     * Create and return an InputStream for reading the contents of the file represented by this entry
     * @return an InputStream
     */
    InputStream createInputStream() {
        return new ByteArrayInputStream(getCurrentBytes())
    }

    /**
     * Create and return an OutputStream for writing the contents of the file represented by this entry
     * @param append - true if the OutputStream should append to any existing contents false if 
     *      any existing contents should be overwritten
     * @return an OutputStream
     * @throws FileSystemException - if an error occurs creating or initializing the OutputStream
     */
    OutputStream createOutputStream(boolean append) {
        // If appending and we already have an OutputStream, then continue to use it
        if (append && out != null) {
            return out
        }
        
        out = new ByteArrayOutputStream()
        byte[] initialContents = (append) ? bytes : EMPTY
        try {
            out.write(initialContents)
        }
        catch (IOException e) {
            throw new FileSystemException(e)
        }
        return out
    }

    //-------------------------------------------------------------------------
    // Internal Helper Methods
    //-------------------------------------------------------------------------
    
    /**
     * @return the current contents of this file entry as a byte[]
     */
    private byte[] getCurrentBytes() {
        return (out != null) ? out.toByteArray() : bytes
    }
    
    /**
     * Set the contents of the file represented by this entry
     * @param contents - the byte[] used as the contents
     */
    private void setContentsInternal(byte[] contents) {
        this.bytes = contents
        
        // Get rid of any OutputStream
        this.out = null
    }
    
    /**
     * @see java.lang.Object#toString()
     */
    String toString() {
        "File['${getPath()}' size=${getSize()} lastModified=$lastModified]"
    }
    
}
