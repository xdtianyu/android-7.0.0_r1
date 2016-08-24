// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/notification/xmpp_iq_stanza_handler.h"

#include <base/bind.h>
#include <base/strings/string_number_conversions.h>
#include <base/strings/stringprintf.h>
#include <weave/provider/task_runner.h>

#include "src/notification/xml_node.h"
#include "src/notification/xmpp_channel.h"

namespace weave {

namespace {

// Default timeout for <iq> requests to the server. If the response hasn't been
// received within this time interval, the request is considered as failed.
const int kTimeoutIntervalSeconds = 30;

// Builds an XML stanza that looks like this:
//  <iq id='${id}' type='${type}' from='${from}' to='${to}'>$body</iq>
// where 'to' and 'from' are optional attributes.
std::string BuildIqStanza(const std::string& id,
                          const std::string& type,
                          const std::string& to,
                          const std::string& from,
                          const std::string& body) {
  std::string to_attr;
  if (!to.empty()) {
    CHECK_EQ(std::string::npos, to.find_first_of("<'>"))
        << "Destination address contains invalid XML characters";
    base::StringAppendF(&to_attr, " to='%s'", to.c_str());
  }
  std::string from_attr;
  if (!from.empty()) {
    CHECK_EQ(std::string::npos, from.find_first_of("<'>"))
        << "Source address contains invalid XML characters";
    base::StringAppendF(&from_attr, " from='%s'", from.c_str());
  }
  return base::StringPrintf("<iq id='%s' type='%s'%s%s>%s</iq>", id.c_str(),
                            type.c_str(), from_attr.c_str(), to_attr.c_str(),
                            body.c_str());
}

}  // anonymous namespace

IqStanzaHandler::IqStanzaHandler(XmppChannelInterface* xmpp_channel,
                                 provider::TaskRunner* task_runner)
    : xmpp_channel_{xmpp_channel}, task_runner_{task_runner} {}

void IqStanzaHandler::SendRequest(const std::string& type,
                                  const std::string& from,
                                  const std::string& to,
                                  const std::string& body,
                                  const ResponseCallback& response_callback,
                                  const TimeoutCallback& timeout_callback) {
  return SendRequestWithCustomTimeout(
      type, from, to, body,
      base::TimeDelta::FromSeconds(kTimeoutIntervalSeconds), response_callback,
      timeout_callback);
}

void IqStanzaHandler::SendRequestWithCustomTimeout(
    const std::string& type,
    const std::string& from,
    const std::string& to,
    const std::string& body,
    base::TimeDelta timeout,
    const ResponseCallback& response_callback,
    const TimeoutCallback& timeout_callback) {
  // Remember the response callback to call later.
  requests_.insert(std::make_pair(++last_request_id_, response_callback));
  // Schedule a time-out callback for this request.
  if (timeout < base::TimeDelta::Max()) {
    task_runner_->PostDelayedTask(
        FROM_HERE,
        base::Bind(&IqStanzaHandler::OnTimeOut, weak_ptr_factory_.GetWeakPtr(),
                   last_request_id_, timeout_callback),
        timeout);
  }

  std::string message =
      BuildIqStanza(std::to_string(last_request_id_), type, to, from, body);
  xmpp_channel_->SendMessage(message);
}

bool IqStanzaHandler::HandleIqStanza(std::unique_ptr<XmlNode> stanza) {
  std::string type;
  if (!stanza->GetAttribute("type", &type)) {
    LOG(ERROR) << "IQ stanza missing 'type' attribute";
    return false;
  }

  std::string id_str;
  if (!stanza->GetAttribute("id", &id_str)) {
    LOG(ERROR) << "IQ stanza missing 'id' attribute";
    return false;
  }

  if (type == "result" || type == "error") {
    // These are response stanzas from the server.
    // Find the corresponding request.
    RequestId id;
    if (!base::StringToInt(id_str, &id)) {
      LOG(ERROR) << "IQ stanza's 'id' attribute is invalid";
      return false;
    }
    auto p = requests_.find(id);
    if (p != requests_.end()) {
      task_runner_->PostDelayedTask(
          FROM_HERE, base::Bind(p->second, base::Passed(std::move(stanza))),
          {});
      requests_.erase(p);
    }
  } else {
    // We do not support server-initiated IQ requests ("set" / "get" / "query").
    // So just reply with "not implemented" error (and swap "to"/"from" attrs).
    std::string error_body =
        "<error type='modify'>"
        "<feature-not-implemented xmlns='urn:ietf:params:xml:ns:xmpp-stanzas'/>"
        "</error>";
    std::string message =
        BuildIqStanza(id_str, "error", stanza->GetAttributeOrEmpty("from"),
                      stanza->GetAttributeOrEmpty("to"), error_body);
    xmpp_channel_->SendMessage(message);
  }
  return true;
}

void IqStanzaHandler::OnTimeOut(RequestId id,
                                const TimeoutCallback& timeout_callback) {
  // Request has not been processed yes, so a real timeout occurred.
  if (requests_.erase(id) > 0)
    timeout_callback.Run();
}

}  // namespace weave
