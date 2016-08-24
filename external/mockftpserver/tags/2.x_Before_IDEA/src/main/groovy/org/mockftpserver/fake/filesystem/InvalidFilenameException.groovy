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
 * Exception thrown when a path/filename is not valid. Causes include:
 * <ul>
 *   <li>The filename contains invalid characters</li>
 *   <li>The path specifies a new filename, but its parent directory does not exist</li>
 *   <li>The path is expected to be a file, but actually specifies an existing directory</li>
 * </ul>
 */
class InvalidFilenameException extends FileSystemException {

     String path
     
     /**
      * @param path
      */
      InvalidFilenameException(String path) {
         super(msg(path))
         this.path = path
     }

     /**
      * @param path
      * @param cause
      */
      InvalidFilenameException(Throwable cause, String path) {
         super(msg(path), cause)
         this.path = path
     }
     
     private static String msg(path) {
         "The path [$path] is not valid"
     }

}