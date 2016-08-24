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

package com.google.polo.wire.xml;

import com.google.polo.exception.PoloException;
import com.google.polo.pairing.HexDump;
import com.google.polo.pairing.PairingContext;
import com.google.polo.pairing.message.PoloMessage;
import com.google.polo.pairing.message.PoloMessage.PoloMessageType;
import com.google.polo.wire.PoloWireInterface;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

public class XmlWireAdapter implements PoloWireInterface {
  
    /**
     * Enables extra verbose debug logging.
     */
    private static final boolean DEBUG_VERBOSE = false;
    
    private static final int STATUS_OK = 1;

    /**
     * The output coming from the peer.
     */
    private final InputStream mInputStream;
    
    /**
     * The input going to the peer.
     */
    private final OutputStream mOutputStream;
    
    private final XmlMessageBuilder mBuilder;

    /**
     * Constructor.
     * 
     * @param input  the {@link InputStream} from the peer
     * @param output  the {@link OutputStream} to the peer
     */
    public XmlWireAdapter(InputStream input, OutputStream output) {
      mInputStream = input;
      mOutputStream = output;
      mBuilder = new XmlMessageBuilder();
    }
    
    /**
     * Generates a new instance from a {@link PairingContext}.
     * @param context  the {@link PairingContext}
     * @return  the new instance
     */
    public static XmlWireAdapter fromContext(PairingContext context) {
      return new XmlWireAdapter(context.getPeerInputStream(),
          context.getPeerOutputStream());
    }
    
    public PoloMessage getNextMessage() throws IOException, PoloException {
        XmlMessageWrapper outerMessage =
                XmlMessageWrapper.fromInputStream(mInputStream);
        if (DEBUG_VERBOSE) {
            debug(">>> Incoming Message:");
            debug(HexDump.dumpHexString(outerMessage.serializeToByteArray()));
        }
        
        String outerXML = new String(outerMessage.getPayload());
        return mBuilder.outerXMLToPoloMessage(outerXML);
    }
    
    public PoloMessage getNextMessage(PoloMessageType type) throws IOException, PoloException {
        PoloMessage message = getNextMessage();
        if (message.getType() != type) {
            throw new PoloException("Wrong message type (wanted " + type +
                ", got " + message.getType() + ")");
          }
          return message;
    }

    public void sendErrorMessage(Exception exception) throws IOException {
        String errorXml;
        try {
          errorXml = mBuilder.getErrorXML(exception);
        } catch (PoloException e) {
          // just ignore it; nothing we can do
          return;
        }
        byte[] outBytes = errorXml.getBytes();
        XmlMessageWrapper message = new XmlMessageWrapper("client", 1,
            (byte) 0, outBytes);
        writeXML(message);
    }
    
    public void sendMessage(PoloMessage poloMessage) throws IOException {
        String outString = mBuilder.getOuterXML(poloMessage, STATUS_OK);
        
        // NOTE(mikey): A particular parser is very sensitive to newline
        // placement. Strip all newlines, then add them to separate adjacent
        // XML entities.
        outString = outString.replace("\n", "");
        outString = outString.replace("><", ">\n<");
        
        byte[] outBytes = outString.getBytes();
        XmlMessageWrapper message = new XmlMessageWrapper("client", 1,
            (byte) 0, outBytes);
        writeXML(message);
    }
    
    /**
     * Writes a {@link XmlMessageWrapper} to the output stream as a
     * {@link String}.
     * 
     * @param message       the message to write
     * @throws IOException  on error generating the serialized message
     */
    private void writeXML(XmlMessageWrapper message) throws IOException {
      if (DEBUG_VERBOSE) {
          debug("<<< Outgoing Message:");
          debug(HexDump.dumpHexString(message.serializeToByteArray()));
      }
      mOutputStream.write(message.serializeToByteArray());
    }
    
    private void debug(String message) {
      System.out.println(message);
    }
 
}
