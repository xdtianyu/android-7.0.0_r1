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

package com.google.polo.wire.json;

import com.google.polo.exception.PoloException;
import com.google.polo.json.JSONException;
import com.google.polo.json.JSONObject;
import com.google.polo.pairing.PairingContext;
import com.google.polo.pairing.PoloUtil;
import com.google.polo.pairing.message.PoloMessage;
import com.google.polo.pairing.message.PoloMessage.PoloMessageType;
import com.google.polo.wire.PoloWireInterface;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

/**
 * A {@link PoloWireInterface} which uses JavaScript Object Notation (JSON) for
 * the message representation.
 * <p>
 * Messages are streamed over the wire prepended with an integer which indicates
 * the total length, in bytes, of the message which follows. The format of the
 * message is JSON.
 * <p>
 * See {@link JsonMessageBuilder} for the underlying message translation
 * implementation.
 */
public class JsonWireAdapter implements PoloWireInterface {

  /**
   * The output coming from the peer.
   */
  private final DataInputStream mInputStream;

  /**
   * The input going to the peer.
   */
  private final DataOutputStream mOutputStream;

  /**
   * Constructor.
   *
   * @param input the {@link InputStream} from the peer
   * @param output the {@link OutputStream} to the peer
   */
  public JsonWireAdapter(InputStream input, OutputStream output) {
    mInputStream = new DataInputStream(input);
    mOutputStream = new DataOutputStream(output);
  }

  /**
   * Generates a new instance from a {@link PairingContext}.
   *
   * @param context the {@link PairingContext}
   * @return the new instance
   */
  public static JsonWireAdapter fromContext(PairingContext context) {
    return new JsonWireAdapter(context.getPeerInputStream(), context
        .getPeerOutputStream());
  }

  public PoloMessage getNextMessage() throws IOException, PoloException {
    byte[] payloadLenBytes = new byte[4];
    mInputStream.readFully(payloadLenBytes);
    long payloadLen = PoloUtil.intBigEndianBytesToLong(payloadLenBytes);
    byte[] outerJsonBytes = new byte[(int) payloadLen];
    mInputStream.readFully(outerJsonBytes);
    return parseOuterMessageString(new String(outerJsonBytes));
  }

  public PoloMessage parseOuterMessageString(String outerString)
      throws PoloException {
    JSONObject outerMessage;
    try {
      outerMessage = new JSONObject(outerString);
    } catch (JSONException e) {
      throw new PoloException("Error parsing incoming message", e);
    }
    return JsonMessageBuilder.outerJsonToPoloMessage(outerMessage);
  }

  public PoloMessage getNextMessage(PoloMessageType type) throws IOException,
      PoloException {
    PoloMessage message = getNextMessage();
    if (message.getType() != type) {
      throw new PoloException("Wrong message type (wanted " + type + ", got "
          + message.getType() + ")");
    }
    return message;
  }

  public void sendErrorMessage(Exception exception) throws IOException {
    try {
      writeJson(JsonMessageBuilder.getErrorJson(exception));
    } catch (PoloException e) {
      throw new IOException("Error sending error message");
    }
  }

  public void sendMessage(PoloMessage message) throws IOException {
    String outString;
    JSONObject outerJson;

    try {
      outerJson = JsonMessageBuilder.getOuterJson(message);
    } catch (PoloException e) {
      throw new IOException("Error generating message");
    }

    writeJson(outerJson);
  }

  /**
   * Writes a {@link JSONObject} to the output stream as a {@link String}.
   *
   * @param  message      the message to write
   * @throws IOException  on error generating the serialized message
   */
  private void writeJson(JSONObject message) throws IOException {
    byte[] outBytes = message.toString().getBytes();
    mOutputStream.write(PoloUtil.intToBigEndianIntBytes(outBytes.length));
    mOutputStream.write(outBytes);
  }

}
