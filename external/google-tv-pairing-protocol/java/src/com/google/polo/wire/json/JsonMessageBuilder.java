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

import com.google.polo.exception.BadSecretException;
import com.google.polo.exception.NoConfigurationException;
import com.google.polo.exception.PoloException;
import com.google.polo.exception.ProtocolErrorException;
import com.google.polo.json.JSONArray;
import com.google.polo.json.JSONException;
import com.google.polo.json.JSONObject;
import com.google.polo.pairing.message.ConfigurationAckMessage;
import com.google.polo.pairing.message.ConfigurationMessage;
import com.google.polo.pairing.message.EncodingOption;
import com.google.polo.pairing.message.EncodingOption.EncodingType;
import com.google.polo.pairing.message.OptionsMessage;
import com.google.polo.pairing.message.OptionsMessage.ProtocolRole;
import com.google.polo.pairing.message.PairingRequestAckMessage;
import com.google.polo.pairing.message.PairingRequestMessage;
import com.google.polo.pairing.message.PoloMessage;
import com.google.polo.pairing.message.PoloMessage.PoloMessageType;
import com.google.polo.pairing.message.SecretAckMessage;
import com.google.polo.pairing.message.SecretMessage;

import java.io.UnsupportedEncodingException;
import java.nio.charset.Charset;

/**
 * A collection of methods to convert {@link PoloMessage}s to and from JSON
 * format.
 * <p>
 * Messages are based on the descriptions found in the file polo.proto.  This
 * mimics the field name and message compositions found in that file.
 */
public class JsonMessageBuilder {

  public static final int PROTOCOL_VERSION = 1;

  /*
   * Status types. These match the values defined by OuterMessage.MessageType
   * in polo.proto.
   */

  public static final int STATUS_OK = 200;
  public static final int STATUS_ERROR = 400;
  public static final int STATUS_BAD_CONFIGURATION = 401;
  public static final int STATUS_BAD_SECRET = 403;

  /*
   * Key names for JSON versions of messages.
   */

  // OuterMessage JSON key names
  private static final String OUTER_FIELD_PAYLOAD = "payload";
  private static final String OUTER_FIELD_TYPE = "type";
  private static final String OUTER_FIELD_STATUS = "status";
  private static final String OUTER_FIELD_PROTOCOL_VERSION = "protocol_version";

  // PairingRequestMessage JSON key names
  private static final String PAIRING_REQUEST_FIELD_SERVICE_NAME =
      "service_name";
  private static final String PAIRING_REQUEST_FIELD_CLIENT_NAME =
      "client_name";

  // PairingRequestAckMessage JSON key names
  private static final String PAIRING_REQUEST_ACK_FIELD_SERVER_NAME =
      "server_name";

  // OptionsMessage JSON key names
  private static final String OPTIONS_FIELD_PREFERRED_ROLE = "preferred_role";
  private static final String OPTIONS_FIELD_OUTPUT_ENCODINGS =
      "output_encodings";
  private static final String OPTIONS_FIELD_INPUT_ENCODINGS = "input_encodings";

  // ConfigurationMessage JSON key names
  private static final String CONFIG_FIELD_CLIENT_ROLE = "client_role";
  private static final String CONFIG_FIELD_ENCODING = "encoding";

  // EncodingOption JSON key names
  private static final String ENCODING_FIELD_TYPE = "type";
  private static final String ENCODING_FIELD_SYMBOL_LENGTH = "symbol_length";

  // SecretMessage JSON key names
  private static final String SECRET_FIELD_SECRET = "secret";

  // SecretAckMessage JSON key names
  private static final String SECRET_ACK_FIELD_SECRET = "secret";


  /**
   * Builds a {@link PoloMessage} from the JSON version of the outer message.
   *
   * @param outerMessage    a {@link JSONObject} corresponding to the
   *                        outermost wire message
   * @return                a new {@link PoloMessage}
   * @throws PoloException  on error parsing the {@link JSONObject}
   */
  public static PoloMessage outerJsonToPoloMessage(JSONObject outerMessage)
      throws PoloException {
    JSONObject payload;
    int status;
    PoloMessageType messageType;

    try {
      status = outerMessage.getInt(OUTER_FIELD_STATUS);
      if (status != STATUS_OK) {
        throw new ProtocolErrorException("Peer reported an error.");
      }
      payload = outerMessage.getJSONObject(OUTER_FIELD_PAYLOAD);
      int msgIntVal = outerMessage.getInt(OUTER_FIELD_TYPE);
      messageType = PoloMessageType.fromIntVal(msgIntVal);
    } catch (JSONException e) {
      throw new PoloException("Bad outer message.", e);
    }

    switch (messageType) {
      case PAIRING_REQUEST:
        return getPairingRequest(payload);
      case PAIRING_REQUEST_ACK:
        return getPairingRequestAck(payload);
      case OPTIONS:
        return getOptionsMessage(payload);
      case CONFIGURATION:
        return getConfigMessage(payload);
      case CONFIGURATION_ACK:
        return getConfigAckMessage(payload);
      case SECRET:
        return getSecretMessage(payload);
      case SECRET_ACK:
        return getSecretAckMessage(payload);
      default:
        return null;
    }
  }

  //
  // Methods to convert JSON messages to PoloMessage instances
  //

  /**
   * Generates a new {@link PairingRequestMessage} from a JSON payload.
   *
   * @param  body           the JSON payload
   * @return                the new message
   * @throws PoloException  on error parsing the {@link JSONObject}
   */
  static PairingRequestMessage getPairingRequest(JSONObject body)
      throws PoloException {
    try {
      String serviceName = body.getString(PAIRING_REQUEST_FIELD_SERVICE_NAME);
      String clientName = null;
      if (body.has(PAIRING_REQUEST_FIELD_CLIENT_NAME)) {
        clientName = body.getString(PAIRING_REQUEST_FIELD_CLIENT_NAME);
      }
      return new PairingRequestMessage(serviceName, clientName);
    } catch (JSONException e) {
      throw new PoloException("Malformed message.", e);
    }
  }

  /**
   * Generates a new {@link PairingRequestAckMessage} from a JSON payload.
   *
   * @param  body           the JSON payload
   * @return                the new message
   */
  static PairingRequestAckMessage getPairingRequestAck(JSONObject body)
      throws PoloException {
    try {
      String serverName = null;
      if (body.has(PAIRING_REQUEST_ACK_FIELD_SERVER_NAME)) {
        serverName = body.getString(PAIRING_REQUEST_ACK_FIELD_SERVER_NAME);
      }
      return new PairingRequestAckMessage(serverName);
    } catch (JSONException e) {
      throw new PoloException("Malformed message.", e);
    }
  }

  /**
   * Generates a new {@link OptionsMessage} from a JSON payload.
   *
   * @param  body           the JSON payload
   * @return                the new message
   * @throws PoloException  on error parsing the {@link JSONObject}
   */
  static OptionsMessage getOptionsMessage(JSONObject body)
      throws PoloException {
    OptionsMessage options = new OptionsMessage();
    try {
      // Input encodings
      JSONArray inEncodings = new JSONArray();
      try {
        if (body.has(OPTIONS_FIELD_INPUT_ENCODINGS)) {
          inEncodings = body.getJSONArray(OPTIONS_FIELD_INPUT_ENCODINGS);
        }
      } catch (JSONException e) {
        throw new PoloException("Bad input encodings", e);
      }

      for (int i = 0; i < inEncodings.length(); i++) {
        JSONObject enc = inEncodings.getJSONObject(i);
        options.addInputEncoding(getEncodingOption(enc));
      }

      // Output encodings
      JSONArray outEncodings = new JSONArray();
      try {
        if (body.has(OPTIONS_FIELD_OUTPUT_ENCODINGS)) {
          outEncodings = body.getJSONArray(OPTIONS_FIELD_OUTPUT_ENCODINGS);
        }
      } catch (JSONException e) {
        throw new PoloException("Bad output encodings", e);
      }

      for (int i = 0; i < outEncodings.length(); i++) {
        JSONObject enc = outEncodings.getJSONObject(i);
        options.addOutputEncoding(getEncodingOption(enc));
      }

      // Role
      ProtocolRole role = ProtocolRole.fromIntVal(
          body.getInt(OPTIONS_FIELD_PREFERRED_ROLE));
      options.setProtocolRolePreference(role);
    } catch (JSONException e) {
      throw new PoloException("Malformed message.", e);
    }

    return options;
  }

  /**
   * Generates a new {@link ConfigurationMessage} from a JSON payload.
   *
   * @param  body           the JSON payload
   * @return                the new message
   * @throws PoloException  on error parsing the {@link JSONObject}
   */
  static ConfigurationMessage getConfigMessage(JSONObject body)
      throws PoloException {
    try {
      EncodingOption encoding = getEncodingOption(
          body.getJSONObject(CONFIG_FIELD_ENCODING));
      ProtocolRole role = ProtocolRole.fromIntVal(
          body.getInt(CONFIG_FIELD_CLIENT_ROLE));
      return new ConfigurationMessage(encoding, role);
    } catch (JSONException e) {
      throw new PoloException("Malformed message.", e);
    }
  }

  /**
   * Generates a new {@link ConfigurationAckMessage} from a JSON payload.
   *
   * @param  body           the JSON payload
   * @return                the new message
   */
  static ConfigurationAckMessage getConfigAckMessage(JSONObject body) {
    return new ConfigurationAckMessage();
  }

  /**
   * Generates a new {@link SecretMessage} from a JSON payload.
   *
   * @param  body           the JSON payload
   * @return                the new message
   * @throws PoloException  on error parsing the {@link JSONObject}
   */
  static SecretMessage getSecretMessage(JSONObject body) throws PoloException {
    try {
      byte[] secretBytes = Base64.decode(
          body.getString(SECRET_FIELD_SECRET).getBytes());
      return new SecretMessage(secretBytes);
    } catch (JSONException e) {
      throw new PoloException("Malformed message.", e);
    }
  }

  /**
   * Generates a new {@link SecretAckMessage} from a JSON payload.
   *
   * @param body  the JSON payload
   * @return      the new message
   * @throws PoloException  on error parsing the {@link JSONObject}
   */
  static SecretAckMessage getSecretAckMessage(JSONObject body)
      throws PoloException {
    try {
      byte[] secretBytes = Base64.decode(
          body.getString(SECRET_ACK_FIELD_SECRET).getBytes());
      return new SecretAckMessage(secretBytes);
    } catch (JSONException e) {
      throw new PoloException("Malformed message.", e);
    }
  }

  /**
   * Generates a new {@link EncodingOption} from a JSON sub-dictionary.
   *
   * @param  option         the JSON sub-dictionary describing the option
   * @return                the new {@link EncodingOption}
   * @throws JSONException  on error parsing the {@link JSONObject}
   */
  static EncodingOption getEncodingOption(JSONObject option)
      throws JSONException {
    int length = option.getInt(ENCODING_FIELD_SYMBOL_LENGTH);
    int intType = option.getInt(ENCODING_FIELD_TYPE);
    EncodingType type = EncodingType.fromIntVal(intType);
    return new EncodingOption(type, length);
  }

  /**
   * Converts a {@link PoloMessage} to a {@link JSONObject}
   *
   * @param message         the message to convert
   * @return                the same message, as translated to JSON
   * @throws PoloException  if the message could not be generated
   */
  public static JSONObject poloMessageToJson(PoloMessage message)
      throws PoloException {
    try {
      if (message instanceof PairingRequestMessage) {
        return toJson((PairingRequestMessage) message);
      } else if (message instanceof PairingRequestAckMessage) {
        return toJson((PairingRequestAckMessage) message);
      } else if (message instanceof OptionsMessage) {
        return toJson((OptionsMessage) message);
      } else if (message instanceof ConfigurationMessage) {
        return toJson((ConfigurationMessage) message);
      } else if (message instanceof ConfigurationAckMessage) {
        return toJson((ConfigurationAckMessage) message);
      } else if (message instanceof SecretMessage) {
        return toJson((SecretMessage) message);
      } else if (message instanceof SecretAckMessage) {
        return toJson((SecretAckMessage) message);
      }
    } catch (JSONException e) {
      throw new PoloException("Error generating message.", e);
    }
    throw new PoloException("Unknown PoloMessage type.");
  }

  /**
   * Generates a JSONObject corresponding to a full wire message (wrapped in
   * an outer message) for the given payload.
   *
   * @param message         the payload to wrap
   * @return                a {@link JSONObject} corresponding to the complete
   *                        wire message
   * @throws PoloException  on error building the {@link JSONObject}
   */
  public static JSONObject getOuterJson(PoloMessage message)
      throws PoloException {
    JSONObject out = new JSONObject();
    int msgType = message.getType().getAsInt();
    JSONObject innerJson = poloMessageToJson(message);

    try {
      out.put(OUTER_FIELD_PROTOCOL_VERSION, PROTOCOL_VERSION);
      out.put(OUTER_FIELD_STATUS, STATUS_OK);
      out.put(OUTER_FIELD_TYPE, msgType);
      out.put(OUTER_FIELD_PAYLOAD, innerJson);
    } catch (JSONException e) {
      throw new PoloException("Error serializing outer message", e);
    }
    return out;
  }

  /**
   * Generates a {@link JSONObject} corresponding to a wire message with an
   * error code in the status field.  The error code is determined by the type
   * of the exception.
   *
   * @param exception       the {@link Exception} to use to determine the error
   *                        code
   * @return                a {@link JSONObject} corresponding to the complete
   *                        wire message
   * @throws PoloException  on error building the {@link JSONObject}
   */
  public static JSONObject getErrorJson(Exception exception)
      throws PoloException {
    JSONObject out = new JSONObject();

    int errorStatus = STATUS_ERROR;

    if (exception instanceof NoConfigurationException) {
      errorStatus = STATUS_BAD_CONFIGURATION;
    } else if (exception instanceof BadSecretException) {
      errorStatus = STATUS_BAD_SECRET;
    }

    try {
      out.put(OUTER_FIELD_PROTOCOL_VERSION, PROTOCOL_VERSION);
      out.put(OUTER_FIELD_STATUS, errorStatus);
    } catch (JSONException e) {
      throw new PoloException("Error serializing outer message", e);
    }
    return out;

  }

  /**
   * Translates a {@link PairingRequestMessage} to a {@link JSONObject}.
   *
   * @throws JSONException  on error generating the {@link JSONObject}
   */
  static JSONObject toJson(PairingRequestMessage message) throws JSONException {
    JSONObject jsonObj = new JSONObject();
    jsonObj.put(PAIRING_REQUEST_FIELD_SERVICE_NAME, message.getServiceName());
    if (message.hasClientName()) {
      jsonObj.put(PAIRING_REQUEST_FIELD_CLIENT_NAME, message.getClientName());
    }
    return jsonObj;
  }

  /**
   * Translates a {@link PairingRequestAckMessage} to a {@link JSONObject}.
   * @throws JSONException
   */
  static JSONObject toJson(PairingRequestAckMessage message)
      throws JSONException {
    JSONObject jsonObj = new JSONObject();
    if (message.hasServerName()) {
      jsonObj.put(PAIRING_REQUEST_ACK_FIELD_SERVER_NAME,
          message.getServerName());
    }
    return jsonObj;
  }

  /**
   * Translates a {@link OptionsMessage} to a {@link JSONObject}.
   *
   * @throws JSONException  on error generating the {@link JSONObject}
   */
  static JSONObject toJson(OptionsMessage message) throws JSONException {
    JSONObject jsonObj = new JSONObject();

    JSONArray inEncsArray = new JSONArray();
    for (EncodingOption encoding : message.getInputEncodingSet()) {
      inEncsArray.put(toJson(encoding));
    }
    jsonObj.put(OPTIONS_FIELD_INPUT_ENCODINGS, inEncsArray);

    JSONArray outEncsArray = new JSONArray();
    for (EncodingOption encoding : message.getOutputEncodingSet()) {
      outEncsArray.put(toJson(encoding));
    }
    jsonObj.put(OPTIONS_FIELD_OUTPUT_ENCODINGS, outEncsArray);

    int intRole = message.getProtocolRolePreference().getAsInt();
    jsonObj.put(OPTIONS_FIELD_PREFERRED_ROLE, intRole);
    return jsonObj;
  }

  /**
   * Translates a {@link ConfigurationMessage} to a {@link JSONObject}.
   *
   * @throws JSONException  on error generating the {@link JSONObject}
   */
  static JSONObject toJson(ConfigurationMessage message) throws JSONException {
    JSONObject jsonObj = new JSONObject();
    JSONObject encoding = toJson(message.getEncoding());
    jsonObj.put(CONFIG_FIELD_ENCODING, encoding);
    int intRole = message.getClientRole().getAsInt();
    jsonObj.put(CONFIG_FIELD_CLIENT_ROLE, intRole);
    return jsonObj;
  }

  /**
   * Translates a {@link ConfigurationAckMessage} to a {@link JSONObject}.
   */
  static JSONObject toJson(ConfigurationAckMessage message) {
    return new JSONObject();
  }

  /**
   * Translates a {@link SecretMessage} to a {@link JSONObject}.
   *
   * @throws JSONException  on error generating the {@link JSONObject}
   */
  static JSONObject toJson(SecretMessage message) throws JSONException {
    JSONObject jsonObj = new JSONObject();
    String bytesStr;
    String charsetName = Charset.defaultCharset().name();
    try {
      bytesStr = new String(Base64.encode(message.getSecret(), charsetName));
    } catch (UnsupportedEncodingException e) {
      // Should never happen.
      bytesStr = "";
    }
    jsonObj.put(SECRET_FIELD_SECRET, bytesStr);
    return jsonObj;
  }

  /**
   * Translates a {@link SecretAckMessage} to a {@link JSONObject}.
   *
   * @throws JSONException  on error generating the {@link JSONObject}
   */
  static JSONObject toJson(SecretAckMessage message) throws JSONException {
    JSONObject jsonObj = new JSONObject();
    String bytesStr;
    String charsetName = Charset.defaultCharset().name();
    try {
      bytesStr = new String(Base64.encode(message.getSecret(), charsetName));
    } catch (UnsupportedEncodingException e) {
      // Should never happen.
      bytesStr = "";
    }
    jsonObj.put(SECRET_ACK_FIELD_SECRET, bytesStr);
    return jsonObj;
  }

  /**
   * Translates a {@link EncodingOption} to a {@link JSONObject}.
   *
   * @throws JSONException  on error generating the {@link JSONObject}
   */
  static JSONObject toJson(EncodingOption encoding) throws JSONException {
    JSONObject result = new JSONObject();
    int intType = encoding.getType().getAsInt();
    result.put(ENCODING_FIELD_TYPE, intType);
    result.put(ENCODING_FIELD_SYMBOL_LENGTH, encoding.getSymbolLength());
    return result;
  }

}
