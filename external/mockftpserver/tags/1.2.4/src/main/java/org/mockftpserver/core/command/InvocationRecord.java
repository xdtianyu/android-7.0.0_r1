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
package org.mockftpserver.core.command;

import java.net.InetAddress;
import java.util.Collections;
import java.util.Date;
import java.util.HashMap;
import java.util.Map;
import java.util.Set;

import org.mockftpserver.core.util.Assert;
import org.mockftpserver.core.util.AssertFailedException;

/**
 * Represents information about a single FTP Command invocation. Manages and provides access to
 * the Command, the host address (<code>InetAddress</code>) of the client that submitted the
 * Command and the timestamp of the Command submission. 
 * <p>
 * This class also supports storing zero or more arbitrary mappings of <i>key</i> to value, where <i>key</i> is 
 * a String and <i>value</i> is any Object. Convenience methods are provided that enable retrieving
 * type-specific data by its <i>key</i>. The data stored in an {@link InvocationRecord} is CommandHandler-specific.
 * <p>
 * The {@link #lock()} method makes an instance of this class immutable. After an instance is locked,
 * calling the {@link #set(String, Object)} method will throw an <code>AssertFailedException</code>. 
 * 
 * @version $Revision$ - $Date$
 * 
 * @author Chris Mair
 */
public class InvocationRecord {

    private Command command;
    private Date time;
    private InetAddress clientHost;
    private Map data = new HashMap();
    private boolean locked = false;
   
    /**
     * Create a new instance
     * @param command - the Command
     * @param clientHost - the client host
     */
    public InvocationRecord(Command command, InetAddress clientHost) {
        this.command = command;
        this.time = new Date();
        this.clientHost = clientHost;
    }

    /**
     * Lock this instance, making it immutable. After an instance is locked,
     * calling the {@link #set(String, Object)} method will throw an
     * <code>AssertFailedException</code>.
     */
    public void lock() {
        locked = true;
    }
    
    /**
     * Return true if this object has been locked, false otherwise. See {@link #lock()}.
     * @return true if this object has been locked, false otherwise.
     */
    public boolean isLocked() {
        return locked;
    }
    
    /**
     * @return the client host that submitted the command, as an InetAddress
     */
    public InetAddress getClientHost() {
        return clientHost;
    }
    
    /**
     * @return the Command
     */
    public Command getCommand() {
        return command;
    }
    
    /**
     * @return the time that the command was processed; this may differ slightly from when the command was received.
     */
    public Date getTime() {
        // Return a copy of the Date object to preserve immutability
        return new Date(time.getTime());
    }
    
    /**
     * Store the value for the specified key. If this object already contained a mapping 
     * for this key, the old value is replaced by the specified value. This method throws
     * an <code>AssertFailedException</code> if this object has been locked. See {@link #lock()}.
     *  
     * @param key - the key; must not be null
     * @param value - the value to store for the specified key
     * 
     * @throws AssertFailedException - if the key is null or this object has been locked.
     */
    public void set(String key, Object value) {
        Assert.notNull(key, "key");
        Assert.isFalse(locked, "The InvocationRecord is locked!");
        data.put(key, value);
    }
    
    /**
     * Returns <code>true</code> if this object contains a mapping for the specified key. 
     *  
     * @param key - the key; must not be null
     * @return <code>true</code> if there is a mapping for the key
     * 
     * @throws AssertFailedException - if the key is null
     */
    public boolean containsKey(String key) {
        Assert.notNull(key, "key");
        return data.containsKey(key);
    }
    
    /**
     * Returns a Set view of the keys for the data stored in this object. 
     * Changes to the returned Set have no effect on the data stored within this object
     * . 
     * @return the Set of keys for the data stored within this object
     */
    public Set keySet() {
        return Collections.unmodifiableSet(data.keySet());
    }
    
    /**
     * Get the String value associated with the specified key. Returns null if there is 
     * no mapping for this key. A return value of null does not necessarily indicate that 
     * this object contains no mapping for the key; it's also possible that the value was
     * explicitly set to null for the key. The containsKey operation may be used to 
     * distinguish these two cases. 
     *  
     * @param key - the key; must not be null 
     * @return the String data stored at the specified key; may be null
     * 
     * @throws ClassCastException - if the object for the specified key is not a String
     * @throws AssertFailedException - if the key is null
     */
    public String getString(String key) {
        Assert.notNull(key, "key");
        return (String) data.get(key);
    }
    
    /**
     * Get the Object value associated with the specified key. Returns null if there is 
     * no mapping for this key. A return value of null does not necessarily indicate that 
     * this object contains no mapping for the key; it's also possible that the value was
     * explicitly set to null for the key. The containsKey operation may be used to 
     * distinguish these two cases. 
     *  
     * @param key - the key; must not be null 
     * @return the data stored at the specified key, as an Object; may be null
     * 
     * @throws AssertFailedException - if the key is null
     */
    public Object getObject(String key) {
        Assert.notNull(key, "key");
        return data.get(key);
    }
    
    /**
     * Return the String representation of this object
     * @see java.lang.Object#toString()
     */
    public String toString() {
        return "InvocationRecord[time=" + time + " client-host=" + clientHost + " command=" + command + " data="+ data + "]";
    }
}
