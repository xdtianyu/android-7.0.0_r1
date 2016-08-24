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

import com.google.polo.pairing.PairingContext;
import com.google.polo.wire.json.JsonWireAdapter;
import com.google.polo.wire.protobuf.ProtobufWireAdapter;
import com.google.polo.wire.xml.XmlWireAdapter;

/**
 * Represents the various wire formats available.
 */
public enum WireFormat {
  PROTOCOL_BUFFERS,   // Protocol Buffers, implemented by ProtobufWireInterface
  JSON,               // JSON, implemented by JsonWireInterface
  XML;                // XML, implemented by XmlWireInterface
  
  /**
   * Returns a new {@link PoloWireInterface} for this enum value.
   * 
   * @param context  the {@link PairingContext} to use in construction
   * @return         the new {@link PoloWireInterface}
   */
  public PoloWireInterface getWireInterface(PairingContext context) {
    switch (this) {
      case PROTOCOL_BUFFERS:
        return ProtobufWireAdapter.fromContext(context);
      case JSON:
        return JsonWireAdapter.fromContext(context);
      case XML:
        return XmlWireAdapter.fromContext(context);
    }
    return null;
  }
}
