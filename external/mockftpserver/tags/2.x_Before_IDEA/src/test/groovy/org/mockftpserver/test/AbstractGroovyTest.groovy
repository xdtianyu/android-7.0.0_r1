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
package org.mockftpserver.test

import org.mockftpserver.test.LoggingUtil
import org.apache.log4j.Logger/**
 * Abstract superclass for Groovy tests
 * 
 * @version $Revision: $ - $Date: $
 * 
 * @author Chris Mair
 */
abstract class AbstractGroovyTest extends GroovyTestCase {

     protected final Logger LOG = Logger.getLogger(this.class)
     private LoggingUtil testLogger 
         
     /** 
      * Assert that the specified code throws an exception of the specified type. 
      * @param expectedExceptionClass - the Class of exception that is expected 
      * @param code - the Closure containing the code to be executed, which is expected to throw an exception of the specified type 
      * @return the thrown Exception instance 
      * 
      * @throws AssertionError - if no exception is thrown by the code or if the thrown exception is not of the expected type 
      */ 
     protected Throwable shouldThrow(Class expectedExceptionClass, Closure code) { 
         def actualException = null 
         try { 
             code.call() 
         } catch (Throwable thrown) { 
             actualException = thrown 
         } 
         assert actualException, "No exception thrown. Expected [${expectedExceptionClass.getName()}]" 
         assert actualException.class == expectedExceptionClass, "Expected [${expectedExceptionClass.getName()}] but was [${actualException.class.name}]" 
         return actualException 
     } 

     /** 
      * Assert that the specified code throws an exception with an error message
      * containing the specified text. 
      * @param text - the text expected within the exception message 
      * @param code - the Closure containing the code to be executed, which is expected to throw an exception of the specified type
      * @return the message from the thrown Exception 
      * 
      * @throws AssertionError - if no exception is thrown by the code or if the thrown 
      * 	exception message does not contain the expected text 
      */ 
     protected String shouldFailWithMessageContaining(String text, Closure code) { 
          def message = shouldFail(code)
          assert message.contains(text)
          return message
     }
          
     //------------------------------------------------------------------------------------ 
     // Test Setup and Tear Down 
     //------------------------------------------------------------------------------------ 
         
     void setUp() { 
         testLogger = LoggingUtil.getTestCaseLogger(this) 
         testLogger.logStartOfTest() 

         super.setUp() 
     } 

     void tearDown() { 
         super.tearDown(); 
         if (testLogger) { 
             testLogger.logEndOfTest() 
         } 
     } 
         
}