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
import com.google.polo.exception.ProtocolErrorException;
import com.google.polo.json.JSONArray;
import com.google.polo.json.JSONException;
import com.google.polo.json.JSONObject;
import com.google.polo.json.XML;
import com.google.polo.pairing.PoloUtil;
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

/**
 * A collection of methods to convert {@link PoloMessage}s to and from XML
 * format.
 * <p>
 * This wire format was specified by a third party; it uses a proprietary
 * 64-byte message header/delimiter, and message internals are inconsistent with
 * the protocol buffer in several places.
 */
public class XmlMessageBuilder {

  /*
   * Status types.
   * NOTE(mikey): These do not match the values defined by
   * OuterMessage.MessageType in polo.proto.
   */

  public static final int STATUS_OK = 1;
  public static final int STATUS_ERROR = 2;

  /*
   * Key names for XML versions of messages.
   */
  
  // OuterMessage XML key names
  private static final String OUTER_FIELD_TYPE = "msg_type";
  private static final String OUTER_FIELD_STATUS = "status";
  private static final String OUTER_FIELD_MSG_ID = "msg_id";
  private static final String OUTER_FIELD_PAYLOAD = "pairing_msg";

  // PairingRequestMessage XML key names
  private static final String PAIRING_REQUEST_FIELD_PROTOCOL_VERSION =
      "proto_version";
  
  // OptionsMessage XML key names
  private static final String OPTIONS_FIELD_PREFERRED_ROLE = "pref_role";
  private static final String OPTIONS_FIELD_OUTPUT_ENCODINGS = "out_encodings";
  private static final String OPTIONS_FIELD_INPUT_ENCODINGS = "in_encodings";

  // ConfigurationMessage XML key names
  private static final String CONFIG_FIELD_CLIENT_ROLE = "role";
  
  // EncodingOption XML key names
  private static final String ENCODING_FIELD_TYPE = "type";
  private static final String ENCODING_FIELD_SYMBOL_LENGTH = "min_length";
  private static final String ENCODING_FIELD_MAX_LENGTH = "max_length";
  private static final String ENCODING_SUBFIELD_ENCODING = "encoding";

  // SecretMessage XML key names
  private static final String SECRET_FIELD_SECRET = "bytes";

  // Payload container names
  private static final String MESSAGE_CONTAINER_NAME_PAIRING_REQUEST =
      "pairing_req";
  private static final String MESSAGE_CONTAINER_NAME_PAIRING_REQUEST_ACK =
      "pairing_req_ack";
  private static final String MESSAGE_CONTAINER_NAME_OPTIONS = "config_options";
  private static final String MESSAGE_CONTAINER_NAME_CONFIG = "config";
  private static final String MESSAGE_CONTAINER_NAME_SECRET = "secret";
  private static final String PAIRING_REQUEST_FIELD_SERVICE_NAME = "svc_name";
  private static final String PAIRING_REQUEST_FIELD_CLIENT_NAME = "client_name";
  private static final String PAIRING_REQUEST_ACK_FIELD_SERVER_NAME =
      "server_name";

  //
  // Encoding types -- these do not match polo.proto's enum.
  //
  
  public static final int ENCODING_TYPE_NUMERIC = 1;
  public static final int ENCODING_TYPE_HEXADECIMAL = 2;
  public static final int ENCODING_TYPE_ALPHANUMERIC = 3;
  public static final int ENCODING_TYPE_QRCODE = 4;

  /**
   * Cache of the last message id header value received.  The value should be
   * copied to any response.
   */
  private String mLastMessageId;

  public XmlMessageBuilder() {
    mLastMessageId = null;
  }

  /**
   * Builds a {@link PoloMessage} from the XML version of the outer message.
   * 
   * @param outerXml        the outermost XML string
   * @return  a new {@link PoloMessage}
   * @throws PoloException  on error parsing the message
   */
  PoloMessage outerXMLToPoloMessage(String outerXml) throws PoloException {
    JSONObject outerMessage;
    try {
      outerMessage = XML.toJSONObject(outerXml);
    } catch (JSONException e) {
        throw new PoloException(e);
    }

    JSONObject payload;
    PoloMessageType messageType;
    try {
      payload = outerMessage.getJSONObject(OUTER_FIELD_PAYLOAD);

      int status = payload.getInt(OUTER_FIELD_STATUS);
      if (status != STATUS_OK) {
        throw new ProtocolErrorException("Peer reported an error.");
      }
      
      int msgIntVal = payload.getInt(OUTER_FIELD_TYPE);
      messageType = PoloMessageType.fromIntVal(msgIntVal);
    } catch (JSONException e) {
      throw new PoloException("Bad outer message.", e);
    }

    if (outerMessage.has("msg_id")) {
      try {
        mLastMessageId = outerMessage.getString("msg_id");
      } catch (JSONException e) {
      }
    } else {
      mLastMessageId = null;
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

  /*
   * Methods to convert XML inner messages to PoloMessage instances.
   *
   * NOTE(mikey): These methods are implemented in terms of JSONObject
   * as a convenient way to represent hierarchical key->(dict|list|value)
   * structures.
   * 
   * Note that these methods are very similar to those found in
   * JsonWireAdapter.  However, the XML wire format was specified with slight
   * differences compared to the protocol buffer definition.  For example,
   * in the OptionsMessage, encodings are wrapped in an "<options>" container.
   * 
   * Also, many fields names have slight differences compared to the names in
   * the protocol buffer (for example, in PairingRequestMessage, "service_name"
   * is called "svc_name".)
   */

  /**
   * Generates a new {@link PairingRequestMessage} from a JSON payload.
   * 
   * @param  body           the JSON payload
   * @return  the new message
   * @throws PoloException  on error parsing the {@link JSONObject}
   */
  PairingRequestMessage getPairingRequest(JSONObject body)
      throws PoloException {
    try {
      JSONObject jsonObj = body.getJSONObject(
          MESSAGE_CONTAINER_NAME_PAIRING_REQUEST);
      String serviceName = jsonObj.getString(
          PAIRING_REQUEST_FIELD_SERVICE_NAME);
      String clientName = null;
      if (jsonObj.has(PAIRING_REQUEST_FIELD_CLIENT_NAME)) {
        clientName = jsonObj.getString(PAIRING_REQUEST_FIELD_CLIENT_NAME);
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
   * @throws PoloException  on error parsing the {@link JSONObject}
   */
  PairingRequestAckMessage getPairingRequestAck(JSONObject body)
      throws PoloException {
    try {
      JSONObject jsonObj = body.getJSONObject(
          MESSAGE_CONTAINER_NAME_PAIRING_REQUEST_ACK);
      String serverName = null;
      if (jsonObj.has(PAIRING_REQUEST_ACK_FIELD_SERVER_NAME)) {
        serverName = jsonObj.getString(PAIRING_REQUEST_ACK_FIELD_SERVER_NAME);
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
  OptionsMessage getOptionsMessage(JSONObject body) throws PoloException {
    OptionsMessage options = new OptionsMessage();
    JSONObject jsonOptions;
    try {
      jsonOptions = body.getJSONObject(MESSAGE_CONTAINER_NAME_OPTIONS);

      JSONObject inEnc = jsonOptions.getJSONObject(
          OPTIONS_FIELD_INPUT_ENCODINGS);
      JSONObject outEnc = jsonOptions.getJSONObject(
          OPTIONS_FIELD_OUTPUT_ENCODINGS);

      // Input encodings
      JSONArray inEncodings = new JSONArray();
      try {
        inEncodings = inEnc.getJSONArray(ENCODING_SUBFIELD_ENCODING);
      } catch (JSONException e) {
        if (inEnc.has(ENCODING_SUBFIELD_ENCODING)) {
          JSONObject enc = inEnc.getJSONObject(ENCODING_SUBFIELD_ENCODING);
          inEncodings.put(enc);
        }
      }

      for (int i = 0; i < inEncodings.length(); i++) {
        JSONObject enc = inEncodings.getJSONObject(i);
        options.addInputEncoding(getEncodingOption(enc));
      }

      // Output encodings
      JSONArray outEncodings = new JSONArray();
      try {
        outEncodings = outEnc.getJSONArray(ENCODING_SUBFIELD_ENCODING);
      } catch (JSONException e) {
        if (outEnc.has(ENCODING_SUBFIELD_ENCODING)) {
          JSONObject enc = outEnc.getJSONObject(ENCODING_SUBFIELD_ENCODING);
          outEncodings.put(enc);
        }
      }

      for (int i = 0; i < outEncodings.length(); i++) {
        JSONObject enc = outEncodings.getJSONObject(i);
        options.addOutputEncoding(getEncodingOption(enc));
      }

      // Role
      ProtocolRole role = ProtocolRole.fromIntVal(
          jsonOptions.getInt(OPTIONS_FIELD_PREFERRED_ROLE));
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
  ConfigurationMessage getConfigMessage(JSONObject body)
  throws PoloException {
    try {  
      EncodingOption encoding = getEncodingOption(
          body.getJSONObject(MESSAGE_CONTAINER_NAME_CONFIG)
              .getJSONObject(ENCODING_SUBFIELD_ENCODING));
      ProtocolRole role = ProtocolRole.fromIntVal(
          body.getJSONObject(MESSAGE_CONTAINER_NAME_CONFIG)
              .getInt(CONFIG_FIELD_CLIENT_ROLE));
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
  ConfigurationAckMessage getConfigAckMessage(JSONObject body) {
    return new ConfigurationAckMessage();
  }

  /**
   * Generates a new {@link SecretMessage} from a JSON payload.
   * 
   * @param  body           the JSON payload
   * @return                the new message
   * @throws PoloException  on error parsing the {@link JSONObject}
   */
  SecretMessage getSecretMessage(JSONObject body) throws PoloException {
    String secret;
    try {
      secret = body.getJSONObject(MESSAGE_CONTAINER_NAME_SECRET)
          .getString(SECRET_FIELD_SECRET);
    } catch (JSONException e) {
      throw new PoloException("Malformed message.", e);
    }
    byte[] secretBytes = PoloUtil.hexStringToBytes(secret);
    return new SecretMessage(secretBytes);
  }

  /**
   * Generates a new {@link SecretAckMessage} from a JSON payload.
   * 
   * @param body  the JSON payload
   * @return      the new message
   */
  SecretAckMessage getSecretAckMessage(JSONObject body) {
    return new SecretAckMessage(null);
  }

  /**
   * Generates a new {@link EncodingOption} from a JSON sub-dictionary.
   * 
   * @param  option         the JSON sub-dictionary describing the option
   * @return                the new {@link EncodingOption}
   * @throws JSONException  on error parsing the {@link JSONObject}
   */
  EncodingOption getEncodingOption(JSONObject option) throws JSONException {
    int length = option.getInt(ENCODING_FIELD_SYMBOL_LENGTH);
    int intType = option.getInt(ENCODING_FIELD_TYPE);
    EncodingType type = encodingTypeFromIntValue(intType);
    return new EncodingOption(type, length);
  }

  /**
   * Converts a {@link PoloMessage} to an XML string.
   * 
   * @param message         the message to convert
   * @return                the same message, as translated to XML
   */
  public String poloMessageToXML(PoloMessage message) {
    try {
      if (message instanceof PairingRequestMessage) {
        return toXML((PairingRequestMessage) message);
      } else if (message instanceof PairingRequestAckMessage) {
        return toXML((PairingRequestAckMessage) message);
      } else if (message instanceof OptionsMessage) {
        return toXML((OptionsMessage) message);
      } else if (message instanceof ConfigurationMessage) {
        return toXML((ConfigurationMessage) message);
      } else if (message instanceof ConfigurationAckMessage) {
        return toXML((ConfigurationAckMessage) message);
      } else if (message instanceof SecretMessage) {
        return toXML((SecretMessage) message);
      } else if (message instanceof SecretAckMessage) {
        return toXML((SecretAckMessage) message);
      }
      return null;
    } catch (JSONException e) {
      e.printStackTrace();
      return "";
    }

  }

  /**
   * Generates a String corresponding to a full wire message (wrapped in
   * an outer message) for the given payload.
   */
  public String getOuterXML(PoloMessage message, int status) {
    StringBuffer out = new StringBuffer();
    
    
    out.append("<" + OUTER_FIELD_PAYLOAD + ">\n");
    
    // status
    out.append("<" + OUTER_FIELD_STATUS + ">");
    out.append(status);
    out.append("</" + OUTER_FIELD_STATUS + ">\n");
    
    // msg_id (optional)
    if (mLastMessageId != null) {
      out.append("<" + OUTER_FIELD_MSG_ID + ">");
      out.append(mLastMessageId);
      out.append("</" + OUTER_FIELD_MSG_ID + ">\n");
    }
    
    // payload
    if (message != null) {
      int msgType = message.getType().getAsInt();
      out.append("<" + OUTER_FIELD_TYPE + ">");
      out.append(msgType);
      out.append("</" + OUTER_FIELD_TYPE + ">\n");

      out.append(poloMessageToXML(message));
      out.append("\n");
    }
    
    out.append("</" + OUTER_FIELD_PAYLOAD + ">\n");
    return out.toString();
  }
  
  
  
  /**
   * Generates an error payload corresponding to an outer message with an
   * error code in the status field.  The error code is determined by the type
   * of the exception.
   * 
   * @param exception       the {@link Exception} to use to determine the error
   *                        code
   * @return                a string outer message
   * @throws PoloException  on error building the message
   */
  public String getErrorXML(Exception exception)
      throws PoloException {
    return getOuterXML(null, STATUS_ERROR);
  }

  /**
   * Translates a {@link PairingRequestMessage} to an XML string.
   *
   * @throws JSONException  on error generating the {@link String}.
   */
  String toXML(PairingRequestMessage message) throws JSONException {
    JSONObject jsonObj = new JSONObject();
    JSONObject pairingReq = new JSONObject();
    jsonObj.put(MESSAGE_CONTAINER_NAME_PAIRING_REQUEST, pairingReq);
    pairingReq.put(PAIRING_REQUEST_FIELD_SERVICE_NAME,
        message.getServiceName());
    if (message.hasClientName()) {
      pairingReq.put(PAIRING_REQUEST_FIELD_CLIENT_NAME,
          message.getServiceName());
    }
    pairingReq.put(PAIRING_REQUEST_FIELD_PROTOCOL_VERSION, 1);
    return XML.toString(jsonObj);
  }

  /**
   * Translates a {@link PairingRequestAckMessage} to an XML string.
   *
   * @throws JSONException  on error generating the {@link String}.
   */
  String toXML(PairingRequestAckMessage message) throws JSONException {
    JSONObject jsonObj = new JSONObject();
    JSONObject pairingReq = new JSONObject();
    jsonObj.put(MESSAGE_CONTAINER_NAME_PAIRING_REQUEST_ACK, pairingReq);
    if (message.hasServerName()) {
      jsonObj.put(PAIRING_REQUEST_ACK_FIELD_SERVER_NAME,
          message.getServerName());
    }
    pairingReq.put(PAIRING_REQUEST_FIELD_PROTOCOL_VERSION, 1);
    return XML.toString(jsonObj);
  }

  /**
   * Translates a {@link OptionsMessage} to an XML string.
   * 
   * @throws JSONException  on error generating the {@link String}. 
   */
 String toXML(OptionsMessage message) throws JSONException {
    JSONObject jsonObj = new JSONObject();
    JSONObject options = new JSONObject();

    JSONObject inEncs = new JSONObject();
    JSONArray inEncsArray = new JSONArray();
    for (EncodingOption encoding : message.getInputEncodingSet()) {
      inEncsArray.put(encodingToJson(encoding));
    }
    inEncs.put(ENCODING_SUBFIELD_ENCODING, inEncsArray);
    options.put(OPTIONS_FIELD_INPUT_ENCODINGS, inEncs);

    JSONObject outEncs = new JSONObject();
    JSONArray outEncsArray = new JSONArray();
    for (EncodingOption encoding : message.getOutputEncodingSet()) {
      outEncsArray.put(encodingToJson(encoding));
    }
    outEncs.put(ENCODING_SUBFIELD_ENCODING, outEncsArray);
    options.put(OPTIONS_FIELD_OUTPUT_ENCODINGS, outEncs);

    options.put(OPTIONS_FIELD_PREFERRED_ROLE,
        message.getProtocolRolePreference().ordinal());
    jsonObj.put(MESSAGE_CONTAINER_NAME_OPTIONS, options);
    return XML.toString(jsonObj);
  }

  /**
   * Translates a {@link ConfigurationMessage} to an XML string.
   * 
   * @throws JSONException  on error generating the {@link String}. 
   */
  String toXML(ConfigurationMessage message) throws JSONException {
    JSONObject jsonObj = new JSONObject();
    JSONObject config = new JSONObject();
    JSONObject encoding = encodingToJson(message.getEncoding());
    config.put(ENCODING_SUBFIELD_ENCODING, encoding);
    config.put(CONFIG_FIELD_CLIENT_ROLE, message.getClientRole().ordinal());
    jsonObj.put(MESSAGE_CONTAINER_NAME_CONFIG, config);
    return XML.toString(jsonObj);
  }

  /**
   * Translates a {@link ConfigurationAckMessage} to an XML string.
   */
 String toXML(ConfigurationAckMessage message) {
    return "";
  }

 /**
  * Translates a {@link SecretMessage} to an XML string.
  * 
  * @throws JSONException  on error generating the {@link String}. 
  */
  String toXML(SecretMessage message) throws JSONException {
    JSONObject jsonObj = new JSONObject();
    JSONObject secret = new JSONObject();
    String bytesStr = PoloUtil.bytesToHexString(message.getSecret());
    secret.put(SECRET_FIELD_SECRET, bytesStr);
    jsonObj.put(MESSAGE_CONTAINER_NAME_SECRET, secret);
    return XML.toString(jsonObj);
  }

  /**
   * Translates a {@link SecretAckMessage} to an XML string.
   */
  String toXML(SecretAckMessage message) {
    return "";
  }

  /**
   * Translates a {@link EncodingOption} to a {@link JSONObject}. 
   * 
   * @throws JSONException  on error generating the {@link JSONObject}
   */
  JSONObject encodingToJson(EncodingOption encoding) throws JSONException {
    JSONObject result = new JSONObject();
    int intType = encodingTypeToIntVal(encoding.getType());
    result.put(ENCODING_FIELD_TYPE, intType);
    result.put(ENCODING_FIELD_SYMBOL_LENGTH, encoding.getSymbolLength());
    result.put(ENCODING_FIELD_MAX_LENGTH, encoding.getSymbolLength());
    return result;
  }

  /**
   * Converts an {@link EncodingType} to the numeric value used on the wire.
   * <p>
   * Note that in this implementation, the values used on the wire do not match
   * those returned by {@link EncodingType#getAsInt()}, hence the extra method.
   * 
   * @param type  the {@link EncodingType}
   * @return      an integer representation
   */
  private static int encodingTypeToIntVal(EncodingType type) {
    switch (type) {
      case ENCODING_ALPHANUMERIC:
        return ENCODING_TYPE_ALPHANUMERIC;
      case ENCODING_NUMERIC:
        return ENCODING_TYPE_NUMERIC;
      case ENCODING_HEXADECIMAL:
        return ENCODING_TYPE_HEXADECIMAL;
      case ENCODING_QRCODE:
        return ENCODING_TYPE_QRCODE;
      case ENCODING_UNKNOWN:
      default:
        return 0;
    }
  }
  
  /**
   * Converts a numeric value used on the wire to the corresponding
   * {@link EncodingType}.
   * <p>
   * Note that in this implementation, the values used on the wire do not match
   * those returned by {@link EncodingType#getAsInt()}, hence the extra method.
   * 
   * @param intType  the value used on the wire
   * @return         the corresponding {@link EncodingType}
   */
  private static EncodingType encodingTypeFromIntValue(int intType) {
    EncodingType type = EncodingType.ENCODING_UNKNOWN;
    switch (intType) {
      case ENCODING_TYPE_ALPHANUMERIC:
        type = EncodingType.ENCODING_ALPHANUMERIC;
        break;
      case ENCODING_TYPE_NUMERIC:
        type = EncodingType.ENCODING_NUMERIC;
        break;
      case ENCODING_TYPE_HEXADECIMAL:
        type = EncodingType.ENCODING_HEXADECIMAL;
        break;
      case ENCODING_TYPE_QRCODE:
        type = EncodingType.ENCODING_QRCODE;
        break;
      default:
        type = EncodingType.ENCODING_UNKNOWN;
      break;
    }
    return type;
  }

}
