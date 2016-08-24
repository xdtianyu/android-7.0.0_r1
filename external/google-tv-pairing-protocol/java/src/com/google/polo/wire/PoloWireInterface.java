/*
 * Copyright (C) 2009 Google Inc.  All rights reserved.
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

package com.google.polo.wire;

import java.io.IOException;

import com.google.polo.exception.PoloException;
import com.google.polo.pairing.message.PoloMessage;

/**
 * Public interface for transport-layer implementations of the pairing
 * protocol.
 */
public interface PoloWireInterface {

  /**
   * Returns the next message from the wire.
   * 
   * @return a new PoloMessage instance
   * @throws IOException  if an error occurred while reading
   * @throws PoloException  if a protocol fault occurred
   */
  public PoloMessage getNextMessage() throws IOException, PoloException;
  
  /**
   * Returns the next message from the wire.
   * 
   * @param type  the required message type to be read
   * @return a new PoloMessage instance
   * @throws IOException  if an error occurred while reading
   * @throws PoloException  if the next message did not match the requested
   *                        type.
   */
  public PoloMessage getNextMessage(PoloMessage.PoloMessageType type)
      throws IOException, PoloException;
  
  /**
   * Send a normal message out on the wire.
   * 
   * @param message  the message to send
   * @throws IOException  if an error occurred while sending
   * @throws PoloException  if the message was not well formed
   */
  public void sendMessage(PoloMessage message)
      throws IOException, PoloException;
  
  /**
   * Send an error message out on the wire, based on an exception.
   * 
   * @param e  the exception causing the error
   * @throws IOException  if an error occurred while sending
   */
  public void sendErrorMessage(Exception e) throws IOException;
  
}
