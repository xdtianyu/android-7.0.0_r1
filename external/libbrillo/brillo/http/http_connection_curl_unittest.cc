// Copyright 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <brillo/http/http_connection_curl.h>

#include <algorithm>
#include <set>

#include <base/callback.h>
#include <brillo/http/http_request.h>
#include <brillo/http/http_transport.h>
#include <brillo/http/mock_curl_api.h>
#include <brillo/http/mock_transport.h>
#include <brillo/streams/memory_stream.h>
#include <brillo/streams/mock_stream.h>
#include <brillo/strings/string_utils.h>
#include <brillo/mime_utils.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using testing::DoAll;
using testing::Invoke;
using testing::Return;
using testing::SetArgPointee;
using testing::_;

namespace brillo {
namespace http {
namespace curl {

namespace {

using ReadWriteCallback =
    size_t(char* ptr, size_t size, size_t num, void* data);

// A helper class to simulate curl_easy_perform action. It invokes the
// read callbacks to obtain the request data from the Connection and then
// calls the header and write callbacks to "send" the response header and body.
class CurlPerformer {
 public:
  // During the tests, use the address of |this| as the CURL* handle.
  // This allows the static Perform() method to obtain the instance pointer
  // having only CURL*.
  CURL* GetCurlHandle() { return reinterpret_cast<CURL*>(this); }

  // Callback to be invoked when mocking out curl_easy_perform() method.
  static CURLcode Perform(CURL* curl) {
    CurlPerformer* me = reinterpret_cast<CurlPerformer*>(curl);
    return me->DoPerform();
  }

  // CURL callback functions and |connection| pointer needed to invoke the
  // callbacks from the Connection class.
  Connection* connection{nullptr};
  ReadWriteCallback* write_callback{nullptr};
  ReadWriteCallback* read_callback{nullptr};
  ReadWriteCallback* header_callback{nullptr};

  // Request body read from the connection.
  std::string request_body;

  // Response data to be sent back to connection.
  std::string status_line;
  HeaderList response_headers;
  std::string response_body;

 private:
  // The actual implementation of curl_easy_perform() fake.
  CURLcode DoPerform() {
    // Read request body.
    char buffer[1024];
    for (;;) {
      size_t size_read = read_callback(buffer, sizeof(buffer), 1, connection);
      if (size_read == CURL_READFUNC_ABORT)
        return CURLE_ABORTED_BY_CALLBACK;
      if (size_read == CURL_READFUNC_PAUSE)
        return CURLE_READ_ERROR;  // Shouldn't happen.
      if (size_read == 0)
        break;
      request_body.append(buffer, size_read);
    }

    // Send the response headers.
    std::vector<std::string> header_lines;
    header_lines.push_back(status_line + "\r\n");
    for (const auto& pair : response_headers) {
      header_lines.push_back(string_utils::Join(": ", pair.first, pair.second) +
                             "\r\n");
    }

    for (const std::string& line : header_lines) {
      CURLcode code = WriteString(header_callback, line);
      if (code != CURLE_OK)
        return code;
    }

    // Send response body.
    return WriteString(write_callback, response_body);
  }

  // Helper method to send a string to a write callback. Keeps calling
  // the callback until all the data is written.
  CURLcode WriteString(ReadWriteCallback* callback, const std::string& str) {
    size_t pos = 0;
    size_t size_remaining = str.size();
    while (size_remaining) {
      size_t size_written = callback(
          const_cast<char*>(str.data() + pos), size_remaining, 1, connection);
      if (size_written == CURL_WRITEFUNC_PAUSE)
        return CURLE_WRITE_ERROR;  // Shouldn't happen.
      CHECK(size_written <= size_remaining) << "Unexpected size returned";
      size_remaining -= size_written;
      pos += size_written;
    }
    return CURLE_OK;
  }
};

// Custom matcher to validate the parameter of CURLOPT_HTTPHEADER CURL option
// which contains the request headers as curl_slist* chain.
MATCHER_P(HeadersMatch, headers, "") {
  std::multiset<std::string> test_headers;
  for (const auto& pair : headers)
    test_headers.insert(string_utils::Join(": ", pair.first, pair.second));

  std::multiset<std::string> src_headers;
  const curl_slist* head = static_cast<const curl_slist*>(arg);
  while (head) {
    src_headers.insert(head->data);
    head = head->next;
  }

  std::vector<std::string> difference;
  std::set_symmetric_difference(src_headers.begin(), src_headers.end(),
                                test_headers.begin(), test_headers.end(),
                                std::back_inserter(difference));
  return difference.empty();
}

// Custom action to save a CURL callback pointer into a member of CurlPerformer.
ACTION_TEMPLATE(SaveCallback,
                HAS_1_TEMPLATE_PARAMS(int, k),
                AND_2_VALUE_PARAMS(performer, mem_ptr)) {
  performer->*mem_ptr = reinterpret_cast<ReadWriteCallback*>(std::get<k>(args));
}

}  // anonymous namespace

class HttpCurlConnectionTest : public testing::Test {
 public:
  void SetUp() override {
    curl_api_ = std::make_shared<MockCurlInterface>();
    transport_ = std::make_shared<MockTransport>();
    EXPECT_CALL(*curl_api_, EasySetOptPtr(handle_, CURLOPT_PRIVATE, _))
        .WillOnce(Return(CURLE_OK));
    connection_ = std::make_shared<Connection>(
        handle_, request_type::kPost, curl_api_, transport_);
    performer_.connection = connection_.get();
  }

  void TearDown() override {
    EXPECT_CALL(*curl_api_, EasyCleanup(handle_)).Times(1);
    connection_.reset();
    transport_.reset();
    curl_api_.reset();
  }

 protected:
  std::shared_ptr<MockCurlInterface> curl_api_;
  std::shared_ptr<MockTransport> transport_;
  CurlPerformer performer_;
  CURL* handle_{performer_.GetCurlHandle()};
  std::shared_ptr<Connection> connection_;
};

TEST_F(HttpCurlConnectionTest, FinishRequestAsync) {
  std::string request_data{"Foo Bar Baz"};
  StreamPtr stream = MemoryStream::OpenRef(request_data, nullptr);
  EXPECT_TRUE(connection_->SetRequestData(std::move(stream), nullptr));
  EXPECT_TRUE(connection_->SendHeaders({{"X-Foo", "bar"}}, nullptr));

  if (VLOG_IS_ON(3)) {
    EXPECT_CALL(*curl_api_,
                EasySetOptCallback(handle_, CURLOPT_DEBUGFUNCTION, _))
        .WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*curl_api_, EasySetOptInt(handle_, CURLOPT_VERBOSE, 1))
        .WillOnce(Return(CURLE_OK));
  }

  EXPECT_CALL(
      *curl_api_,
      EasySetOptOffT(handle_, CURLOPT_POSTFIELDSIZE_LARGE, request_data.size()))
      .WillOnce(Return(CURLE_OK));

  EXPECT_CALL(*curl_api_, EasySetOptCallback(handle_, CURLOPT_READFUNCTION, _))
      .WillOnce(Return(CURLE_OK));
  EXPECT_CALL(*curl_api_, EasySetOptPtr(handle_, CURLOPT_READDATA, _))
      .WillOnce(Return(CURLE_OK));

  EXPECT_CALL(*curl_api_, EasySetOptPtr(handle_, CURLOPT_HTTPHEADER, _))
      .WillOnce(Return(CURLE_OK));

  EXPECT_CALL(*curl_api_, EasySetOptCallback(handle_, CURLOPT_WRITEFUNCTION, _))
      .WillOnce(Return(CURLE_OK));
  EXPECT_CALL(*curl_api_, EasySetOptPtr(handle_, CURLOPT_WRITEDATA, _))
      .WillOnce(Return(CURLE_OK));

  EXPECT_CALL(*curl_api_,
              EasySetOptCallback(handle_, CURLOPT_HEADERFUNCTION, _))
      .WillOnce(Return(CURLE_OK));
  EXPECT_CALL(*curl_api_, EasySetOptPtr(handle_, CURLOPT_HEADERDATA, _))
      .WillOnce(Return(CURLE_OK));

  EXPECT_CALL(*transport_, StartAsyncTransfer(connection_.get(), _, _))
      .Times(1);
  connection_->FinishRequestAsync({}, {});
}

MATCHER_P(MatchStringBuffer, data, "") {
  return data.compare(static_cast<const char*>(arg)) == 0;
}

TEST_F(HttpCurlConnectionTest, FinishRequest) {
  std::string request_data{"Foo Bar Baz"};
  std::string response_data{"<html><body>OK</body></html>"};
  StreamPtr stream = MemoryStream::OpenRef(request_data, nullptr);
  HeaderList headers{
      {request_header::kAccept, "*/*"},
      {request_header::kContentType, mime::text::kPlain},
      {request_header::kContentLength, std::to_string(request_data.size())},
      {"X-Foo", "bar"},
  };
  std::unique_ptr<MockStream> response_stream(new MockStream);
  EXPECT_CALL(*response_stream,
              WriteAllBlocking(MatchStringBuffer(response_data),
                               response_data.size(), _))
      .WillOnce(Return(true));
  EXPECT_CALL(*response_stream, CanSeek())
      .WillOnce(Return(false));
  connection_->SetResponseData(std::move(response_stream));
  EXPECT_TRUE(connection_->SetRequestData(std::move(stream), nullptr));
  EXPECT_TRUE(connection_->SendHeaders(headers, nullptr));

  // Expectations for Connection::FinishRequest() call.
  if (VLOG_IS_ON(3)) {
    EXPECT_CALL(*curl_api_,
                EasySetOptCallback(handle_, CURLOPT_DEBUGFUNCTION, _))
        .WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*curl_api_, EasySetOptInt(handle_, CURLOPT_VERBOSE, 1))
        .WillOnce(Return(CURLE_OK));
  }

  EXPECT_CALL(
      *curl_api_,
      EasySetOptOffT(handle_, CURLOPT_POSTFIELDSIZE_LARGE, request_data.size()))
      .WillOnce(Return(CURLE_OK));

  EXPECT_CALL(*curl_api_, EasySetOptCallback(handle_, CURLOPT_READFUNCTION, _))
      .WillOnce(
          DoAll(SaveCallback<2>(&performer_, &CurlPerformer::read_callback),
                Return(CURLE_OK)));
  EXPECT_CALL(*curl_api_, EasySetOptPtr(handle_, CURLOPT_READDATA, _))
      .WillOnce(Return(CURLE_OK));

  EXPECT_CALL(*curl_api_,
              EasySetOptPtr(handle_, CURLOPT_HTTPHEADER, HeadersMatch(headers)))
      .WillOnce(Return(CURLE_OK));

  EXPECT_CALL(*curl_api_, EasySetOptCallback(handle_, CURLOPT_WRITEFUNCTION, _))
      .WillOnce(
          DoAll(SaveCallback<2>(&performer_, &CurlPerformer::write_callback),
                Return(CURLE_OK)));
  EXPECT_CALL(*curl_api_, EasySetOptPtr(handle_, CURLOPT_WRITEDATA, _))
      .WillOnce(Return(CURLE_OK));

  EXPECT_CALL(*curl_api_,
              EasySetOptCallback(handle_, CURLOPT_HEADERFUNCTION, _))
      .WillOnce(
          DoAll(SaveCallback<2>(&performer_, &CurlPerformer::header_callback),
                Return(CURLE_OK)));
  EXPECT_CALL(*curl_api_, EasySetOptPtr(handle_, CURLOPT_HEADERDATA, _))
      .WillOnce(Return(CURLE_OK));

  EXPECT_CALL(*curl_api_, EasyPerform(handle_))
      .WillOnce(Invoke(&CurlPerformer::Perform));

  EXPECT_CALL(*curl_api_, EasyGetInfoInt(handle_, CURLINFO_RESPONSE_CODE, _))
      .WillOnce(DoAll(SetArgPointee<2>(status_code::Ok), Return(CURLE_OK)));

  // Set up the CurlPerformer with the response data expected to be received.
  HeaderList response_headers{
      {response_header::kContentLength, std::to_string(response_data.size())},
      {response_header::kContentType, mime::text::kHtml},
      {"X-Foo", "baz"},
  };
  performer_.status_line = "HTTP/1.1 200 OK";
  performer_.response_body = response_data;
  performer_.response_headers = response_headers;

  // Perform the request.
  EXPECT_TRUE(connection_->FinishRequest(nullptr));

  // Make sure we sent out the request body correctly.
  EXPECT_EQ(request_data, performer_.request_body);

  // Validate the parsed response data.
  EXPECT_CALL(*curl_api_, EasyGetInfoInt(handle_, CURLINFO_RESPONSE_CODE, _))
      .WillOnce(DoAll(SetArgPointee<2>(status_code::Ok), Return(CURLE_OK)));
  EXPECT_EQ(status_code::Ok, connection_->GetResponseStatusCode());
  EXPECT_EQ("HTTP/1.1", connection_->GetProtocolVersion());
  EXPECT_EQ("OK", connection_->GetResponseStatusText());
  EXPECT_EQ(std::to_string(response_data.size()),
            connection_->GetResponseHeader(response_header::kContentLength));
  EXPECT_EQ(mime::text::kHtml,
            connection_->GetResponseHeader(response_header::kContentType));
  EXPECT_EQ("baz", connection_->GetResponseHeader("X-Foo"));
  auto data_stream = connection_->ExtractDataStream(nullptr);
  ASSERT_NE(nullptr, data_stream.get());
}

}  // namespace curl
}  // namespace http
}  // namespace brillo
