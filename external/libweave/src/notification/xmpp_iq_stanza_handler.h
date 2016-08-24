// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_SRC_NOTIFICATION_XMPP_IQ_STANZA_HANDLER_H_
#define LIBWEAVE_SRC_NOTIFICATION_XMPP_IQ_STANZA_HANDLER_H_

#include <map>
#include <memory>
#include <string>

#include <base/callback_forward.h>
#include <base/macros.h>
#include <base/memory/weak_ptr.h>
#include <base/time/time.h>

#include "src/notification/xmpp_stream_parser.h"

namespace weave {

class XmppChannelInterface;

namespace provider {
class TaskRunner;
}

class IqStanzaHandler {
 public:
  using ResponseCallback = base::Callback<void(std::unique_ptr<XmlNode>)>;
  using TimeoutCallback = base::Closure;

  IqStanzaHandler(XmppChannelInterface* xmpp_channel,
                  provider::TaskRunner* task_runner);

  // Sends <iq> request to the server.
  // |type| is the IQ stanza type, one of "get", "set", "query".
  // |to| is the target of the message. If empty string, 'to' is omitted.
  // |body| the XML snipped to include between <iq>...</iq>
  // |response_callback| is called with result or error XML stanza received
  // from the server in response to the request sent.
  // |timeout_callback| is called when the response to the request hasn't been
  // received within the time allotted.
  void SendRequest(const std::string& type,
                   const std::string& from,
                   const std::string& to,
                   const std::string& body,
                   const ResponseCallback& response_callback,
                   const TimeoutCallback& timeout_callback);

  // |timeout| is the custom time interval after which requests should be
  // considered failed.
  void SendRequestWithCustomTimeout(const std::string& type,
                                    const std::string& from,
                                    const std::string& to,
                                    const std::string& body,
                                    base::TimeDelta timeout,
                                    const ResponseCallback& response_callback,
                                    const TimeoutCallback& timeout_callback);

  // Processes an <iq> stanza is received from the server. This will match the
  // stanza's 'id' attribute with pending request ID and if found, will
  // call the |response_callback|, or if the request is not found, an error
  // stanza fill be sent back to the server.
  // Returns false if some unexpected condition occurred and the stream should
  // be restarted.
  bool HandleIqStanza(std::unique_ptr<XmlNode> stanza);

 private:
  using RequestId = int;
  void OnTimeOut(RequestId id, const TimeoutCallback& timeout_callback);

  XmppChannelInterface* xmpp_channel_;
  provider::TaskRunner* task_runner_{nullptr};
  std::map<RequestId, ResponseCallback> requests_;
  RequestId last_request_id_{0};

  base::WeakPtrFactory<IqStanzaHandler> weak_ptr_factory_{this};
  DISALLOW_COPY_AND_ASSIGN(IqStanzaHandler);
};

}  // namespace weave

#endif  // LIBWEAVE_SRC_NOTIFICATION_XMPP_IQ_STANZA_HANDLER_H_
