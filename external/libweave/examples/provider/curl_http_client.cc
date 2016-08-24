// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "examples/provider/curl_http_client.h"

#include <algorithm>
#include <future>
#include <thread>

#include <base/bind.h>
#include <base/logging.h>
#include <curl/curl.h>
#include <weave/enum_to_string.h>
#include <weave/provider/task_runner.h>

namespace weave {
namespace examples {

namespace {

struct ResponseImpl : public provider::HttpClient::Response {
  int GetStatusCode() const override { return status; }
  std::string GetContentType() const override { return content_type; }
  std::string GetData() const override { return data; }

  long status{0};
  std::string content_type;
  std::string data;
};

size_t WriteFunction(void* contents, size_t size, size_t nmemb, void* userp) {
  static_cast<std::string*>(userp)->append(static_cast<const char*>(contents),
                                           size * nmemb);
  return size * nmemb;
}

size_t HeaderFunction(void* contents, size_t size, size_t nmemb, void* userp) {
  std::string header(static_cast<const char*>(contents), size * nmemb);
  auto pos = header.find(':');
  if (pos != std::string::npos) {
    std::pair<std::string, std::string> header_pair;

    static const char kSpaces[] = " \t\r\n";
    header_pair.first = header.substr(0, pos);
    pos = header.find_first_not_of(kSpaces, pos + 1);
    if (pos != std::string::npos) {
      auto last_non_space = header.find_last_not_of(kSpaces);
      if (last_non_space >= pos)
        header_pair.second = header.substr(pos, last_non_space - pos + 1);
    }

    static_cast<provider::HttpClient::Headers*>(userp)->emplace_back(
        std::move(header_pair));
  }
  return size * nmemb;
}

std::pair<std::unique_ptr<CurlHttpClient::Response>, ErrorPtr>
SendRequestBlocking(CurlHttpClient::Method method,
                    const std::string& url,
                    const CurlHttpClient::Headers& headers,
                    const std::string& data) {
  std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> curl{curl_easy_init(),
                                                           &curl_easy_cleanup};
  CHECK(curl);

  switch (method) {
    case CurlHttpClient::Method::kGet:
      CHECK_EQ(CURLE_OK, curl_easy_setopt(curl.get(), CURLOPT_HTTPGET, 1L));
      break;
    case CurlHttpClient::Method::kPost:
      CHECK_EQ(CURLE_OK, curl_easy_setopt(curl.get(), CURLOPT_HTTPPOST, 1L));
      break;
    case CurlHttpClient::Method::kPatch:
    case CurlHttpClient::Method::kPut:
      CHECK_EQ(CURLE_OK, curl_easy_setopt(curl.get(), CURLOPT_CUSTOMREQUEST,
                                          weave::EnumToString(method).c_str()));
      break;
  }

  CHECK_EQ(CURLE_OK, curl_easy_setopt(curl.get(), CURLOPT_URL, url.c_str()));

  curl_slist* chunk = nullptr;
  for (const auto& h : headers)
    chunk = curl_slist_append(chunk, (h.first + ": " + h.second).c_str());

  CHECK_EQ(CURLE_OK, curl_easy_setopt(curl.get(), CURLOPT_HTTPHEADER, chunk));

  if (!data.empty() || method == CurlHttpClient::Method::kPost) {
    CHECK_EQ(CURLE_OK,
             curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDS, data.c_str()));
  }

  std::unique_ptr<ResponseImpl> response{new ResponseImpl};
  CHECK_EQ(CURLE_OK,
           curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, &WriteFunction));
  CHECK_EQ(CURLE_OK,
           curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &response->data));
  CHECK_EQ(CURLE_OK, curl_easy_setopt(curl.get(), CURLOPT_HEADERFUNCTION,
                                      &HeaderFunction));
  provider::HttpClient::Headers response_headers;
  CHECK_EQ(CURLE_OK,
           curl_easy_setopt(curl.get(), CURLOPT_HEADERDATA, &response_headers));

  CURLcode res = curl_easy_perform(curl.get());
  if (chunk)
    curl_slist_free_all(chunk);

  ErrorPtr error;
  if (res != CURLE_OK) {
    Error::AddTo(&error, FROM_HERE, "curl_easy_perform_error",
                 curl_easy_strerror(res));
    return {nullptr, std::move(error)};
  }

  for (const auto& header : response_headers) {
    if (header.first == "Content-Type")
      response->content_type = header.second;
  }

  CHECK_EQ(CURLE_OK, curl_easy_getinfo(curl.get(), CURLINFO_RESPONSE_CODE,
                                       &response->status));

  return {std::move(response), nullptr};
}

}  // namespace

CurlHttpClient::CurlHttpClient(provider::TaskRunner* task_runner)
    : task_runner_{task_runner} {}

void CurlHttpClient::SendRequest(Method method,
                                 const std::string& url,
                                 const Headers& headers,
                                 const std::string& data,
                                 const SendRequestCallback& callback) {
  pending_tasks_.emplace_back(
      std::async(std::launch::async, SendRequestBlocking, method, url, headers,
                 data),
      callback);
  if (pending_tasks_.size() == 1)  // More means check is scheduled.
    CheckTasks();
}

void CurlHttpClient::CheckTasks() {
  VLOG(4) << "CurlHttpClient::CheckTasks, size=" << pending_tasks_.size();
  auto ready_begin =
      std::partition(pending_tasks_.begin(), pending_tasks_.end(),
                     [](const decltype(pending_tasks_)::value_type& value) {
                       return value.first.wait_for(std::chrono::seconds(0)) !=
                              std::future_status::ready;
                     });

  for (auto it = ready_begin; it != pending_tasks_.end(); ++it) {
    CHECK(it->first.valid());
    auto result = it->first.get();
    VLOG(2) << "CurlHttpClient::CheckTasks done";
    task_runner_->PostDelayedTask(
        FROM_HERE, base::Bind(it->second, base::Passed(&result.first),
                              base::Passed(&result.second)),
        {});
  }

  pending_tasks_.erase(ready_begin, pending_tasks_.end());

  if (pending_tasks_.empty()) {
    VLOG(2) << "No more CurlHttpClient tasks";
    return;
  }

  task_runner_->PostDelayedTask(
      FROM_HERE,
      base::Bind(&CurlHttpClient::CheckTasks, weak_ptr_factory_.GetWeakPtr()),
      base::TimeDelta::FromMilliseconds(100));
}

}  // namespace examples
}  // namespace weave
