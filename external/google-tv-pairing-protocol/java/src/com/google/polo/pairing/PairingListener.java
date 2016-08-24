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

package com.google.polo.pairing;

import com.google.polo.exception.PoloException;

/**
 * Listener interface for handling events within a pairing session.
 */
public interface PairingListener {
  
  public static enum LogLevel {
    LOG_DEBUG,
    LOG_INFO,
    LOG_ERROR
  }
  
  /**
   * Called at the start of a new pairing session. The session object can be
   * inspected to determine whether in client or server mode.
   * 
   * @param session  the pairing session
   */
  void onSessionCreated(PairingSession session);

  /**
   * Called when the session calls for the local entity to act as the input
   * device.
   * 
   * @param session  the pairing session
   * @throws PoloException  on error receiving the input
   */
  void onPerformInputDeviceRole(PairingSession session) throws PoloException;
  
  /**
   * Called when the session calls for the local entity to act as the display
   * device.
   * 
   * @param session         the pairing session
   * @throws PoloException  on error displaying the secret
   */
  void onPerformOutputDeviceRole(PairingSession session, byte[] gamma)
      throws PoloException;
  
  /**
   * Called at the end of a pairing session.  The session object can be
   * inspected to determine success or failure.
   * 
   * @param session  the pairing session
   */
  void onSessionEnded(PairingSession session);
  
  /**
   * Receives various log messages from the protocol.
   * 
   * @param level    the severity of the message
   * @param message  the message to log
   */
  void onLogMessage(LogLevel level, String message);
  
}
