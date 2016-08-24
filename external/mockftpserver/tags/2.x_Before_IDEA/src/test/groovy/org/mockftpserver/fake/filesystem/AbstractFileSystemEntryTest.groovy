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

import java.lang.reflect.Constructor

import org.mockftpserver.test.AbstractGroovyTest

/**
 * Abstract test superclass for subclasses of AbstractFileSystemEntry
 * 
 * @version $Revision: $ - $Date: $
 *
 * @author Chris Mair
 */
public abstract class AbstractFileSystemEntryTest extends AbstractGroovyTest {

    protected static final PATH = "c:/test/dir"
    
    /**
     * Test the no-argument constructor 
     */
    void testConstructor_NoArgs() {
        AbstractFileSystemEntry entry = (AbstractFileSystemEntry) getImplementationClass().newInstance()
        assertNull("path", entry.getPath())
        entry.setPath(PATH)
        assert entry.getPath() == PATH
        assert isDirectory() == entry.isDirectory()
    }

    /**
     * Test the constructor that takes a single String parameter
     */
    void testConstructor_Path() {
        Constructor constructor = getImplementationClass().getConstructor([String.class] as Class[])
        AbstractFileSystemEntry entry = (AbstractFileSystemEntry) constructor.newInstance([PATH] as Object[])
        LOG.info(entry)
        assertEquals("path", PATH, entry.getPath())
        entry.setPath("")
        assert entry.getPath() == ""
        assert isDirectory() == entry.isDirectory()
    }

    /**
     * @return the subclass of AbstractFileSystemEntry to be tested
     */
    protected abstract Class getImplementationClass()
    
    /**
     * @return true if the class being tested represents a directory entry 
     */
    protected abstract boolean isDirectory()
    
}
