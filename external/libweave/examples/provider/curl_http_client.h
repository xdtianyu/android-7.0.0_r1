// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_EXAMPLES_PROVIDER_CURL_HTTP_CLIENT_H_
#define LIBWEAVE_EXAMPLES_PROVIDER_CURL_HTTP_CLIENT_H_

#include <future>
#include <string>
#include <utility>

#include <base/memory/weak_ptr.h>
#include <weave/provider/http_client.h>

namespace weave {

namespace provider {
class TaskRunner;
}

namespace examples {

// Basic implementation of weave::HttpClient using libcurl. Should be used in
// production code as it's blocking and does not validate server certificates.
class CurlHttpClient : public provider::HttpClient {
 public:
  explicit CurlHttpClient(provider::TaskRunner* task_runner);

  void SendRequest(Method method,
                   const std::string& url,
                   const Headers& headers,
                   const std::string& data,
                   const SendRequestCallback& callback) override;

 private:
  void CheckTasks();

  std::vector<
      std::pair<std::future<std::pair<std::unique_ptr<Response>, ErrorPtr>>,
                SendRequestCallback>>
      pending_tasks_;
  provider::TaskRunner* task_runner_{nullptr};

  base::WeakPtrFactory<CurlHttpClient> weak_ptr_factory_{this};
};

}  // namespace examples
}  // namespace weave

#endif  // LIBWEAVE_EXAMPLES_PROVIDER_CURL_HTTP_CLIENT_H_
