// Copyright 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <brillo/http/http_request.h>

#include <string>

#include <base/callback.h>
#include <brillo/bind_lambda.h>
#include <brillo/http/mock_connection.h>
#include <brillo/http/mock_transport.h>
#include <brillo/mime_utils.h>
#include <brillo/streams/mock_stream.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using testing::DoAll;
using testing::Invoke;
using testing::Return;
using testing::SetArgPointee;
using testing::Unused;
using testing::WithArg;
using testing::_;

namespace brillo {
namespace http {

MATCHER_P(ContainsStringData, str, "") {
  if (arg->GetSize() != str.size())
    return false;

  std::string data;
  char buf[100];
  size_t read = 0;
  while (arg->ReadBlocking(buf, sizeof(buf), &read, nullptr) && read > 0) {
    data.append(buf, read);
  }
  return data == str;
}

class HttpRequestTest : public testing::Test {
 public:
  void SetUp() override {
    transport_ = std::make_shared<MockTransport>();
    connection_ = std::make_shared<MockConnection>(transport_);
  }

  void TearDown() override {
    // Having shared pointers to mock objects (some of methods in these tests
    // return shared pointers to connection and transport) could cause the
    // test expectations to hold on to the mock object without releasing them
    // at the end of a test, causing Mock's object leak detection to erroneously
    // detect mock object "leaks". Verify and clear the expectations manually
    // and explicitly to ensure the shared pointer refcounters are not
    // preventing the mocks to be destroyed at the end of each test.
    testing::Mock::VerifyAndClearExpectations(connection_.get());
    connection_.reset();
    testing::Mock::VerifyAndClearExpectations(transport_.get());
    transport_.reset();
  }

 protected:
  std::shared_ptr<MockTransport> transport_;
  std::shared_ptr<MockConnection> connection_;
};

TEST_F(HttpRequestTest, Defaults) {
  Request request{"http://www.foo.bar", request_type::kPost, transport_};
  EXPECT_TRUE(request.GetContentType().empty());
  EXPECT_TRUE(request.GetReferer().empty());
  EXPECT_TRUE(request.GetUserAgent().empty());
  EXPECT_EQ("*/*", request.GetAccept());
  EXPECT_EQ("http://www.foo.bar", request.GetRequestURL());
  EXPECT_EQ(request_type::kPost, request.GetRequestMethod());

  Request request2{"http://www.foo.bar/baz", request_type::kGet, transport_};
  EXPECT_EQ("http://www.foo.bar/baz", request2.GetRequestURL());
  EXPECT_EQ(request_type::kGet, request2.GetRequestMethod());
}

TEST_F(HttpRequestTest, ContentType) {
  Request request{"http://www.foo.bar", request_type::kPost, transport_};
  request.SetContentType(mime::image::kJpeg);
  EXPECT_EQ(mime::image::kJpeg, request.GetContentType());
}

TEST_F(HttpRequestTest, Referer) {
  Request request{"http://www.foo.bar", request_type::kPost, transport_};
  request.SetReferer("http://www.foo.bar/baz");
  EXPECT_EQ("http://www.foo.bar/baz", request.GetReferer());
}

TEST_F(HttpRequestTest, UserAgent) {
  Request request{"http://www.foo.bar", request_type::kPost, transport_};
  request.SetUserAgent("FooBar Browser");
  EXPECT_EQ("FooBar Browser", request.GetUserAgent());
}

TEST_F(HttpRequestTest, Accept) {
  Request request{"http://www.foo.bar", request_type::kPost, transport_};
  request.SetAccept("text/*, text/html, text/html;level=1, */*");
  EXPECT_EQ("text/*, text/html, text/html;level=1, */*", request.GetAccept());
}

TEST_F(HttpRequestTest, GetResponseAndBlock) {
  Request request{"http://www.foo.bar", request_type::kPost, transport_};
  request.SetUserAgent("FooBar Browser");
  request.SetReferer("http://www.foo.bar/baz");
  request.SetAccept("text/*, text/html, text/html;level=1, */*");
  request.AddHeader(request_header::kAcceptEncoding, "compress, gzip");
  request.AddHeaders({
      {request_header::kAcceptLanguage, "da, en-gb;q=0.8, en;q=0.7"},
      {request_header::kConnection, "close"},
  });
  request.AddRange(-10);
  request.AddRange(100, 200);
  request.AddRange(300);
  std::string req_body{"Foo bar baz"};
  request.AddHeader(request_header::kContentType, mime::text::kPlain);

  EXPECT_CALL(*transport_, CreateConnection(
      "http://www.foo.bar",
      request_type::kPost,
      HeaderList{
        {request_header::kAcceptEncoding, "compress, gzip"},
        {request_header::kAcceptLanguage, "da, en-gb;q=0.8, en;q=0.7"},
        {request_header::kConnection, "close"},
        {request_header::kContentType, mime::text::kPlain},
        {request_header::kRange, "bytes=-10,100-200,300-"},
        {request_header::kAccept, "text/*, text/html, text/html;level=1, */*"},
      },
      "FooBar Browser",
      "http://www.foo.bar/baz",
      nullptr)).WillOnce(Return(connection_));

  EXPECT_CALL(*connection_, MockSetRequestData(ContainsStringData(req_body), _))
      .WillOnce(Return(true));

  EXPECT_TRUE(
      request.AddRequestBody(req_body.data(), req_body.size(), nullptr));

  EXPECT_CALL(*connection_, FinishRequest(_)).WillOnce(Return(true));
  auto resp = request.GetResponseAndBlock(nullptr);
  EXPECT_NE(nullptr, resp.get());
}

TEST_F(HttpRequestTest, GetResponse) {
  Request request{"http://foo.bar", request_type::kGet, transport_};

  std::string resp_data{"FooBar response body"};
  auto read_data =
      [&resp_data](void* buffer, Unused, size_t* read, Unused) -> bool {
    memcpy(buffer, resp_data.data(), resp_data.size());
    *read = resp_data.size();
    return true;
  };

  auto success_callback =
      [this, &resp_data](RequestID request_id, std::unique_ptr<Response> resp) {
    EXPECT_EQ(23, request_id);
    EXPECT_CALL(*connection_, GetResponseStatusCode())
        .WillOnce(Return(status_code::Partial));
    EXPECT_EQ(status_code::Partial, resp->GetStatusCode());

    EXPECT_CALL(*connection_, GetResponseStatusText())
        .WillOnce(Return("Partial completion"));
    EXPECT_EQ("Partial completion", resp->GetStatusText());

    EXPECT_CALL(*connection_, GetResponseHeader(response_header::kContentType))
        .WillOnce(Return(mime::text::kHtml));
    EXPECT_EQ(mime::text::kHtml, resp->GetContentType());

    EXPECT_EQ(resp_data, resp->ExtractDataAsString());
  };

  auto finish_request_async =
      [this, &read_data, &resp_data](const SuccessCallback& success_callback) {
    std::unique_ptr<MockStream> mock_stream{new MockStream};
    EXPECT_CALL(*mock_stream, ReadBlocking(_, _, _, _))
        .WillOnce(Invoke(read_data))
        .WillOnce(DoAll(SetArgPointee<2>(0), Return(true)));

    EXPECT_CALL(*connection_, MockExtractDataStream(_))
      .WillOnce(Return(mock_stream.release()));
    std::unique_ptr<Response> resp{new Response{connection_}};
    success_callback.Run(23, std::move(resp));
  };

  EXPECT_CALL(
      *transport_,
      CreateConnection("http://foo.bar", request_type::kGet, _, "", "", _))
      .WillOnce(Return(connection_));

  EXPECT_CALL(*connection_, FinishRequestAsync(_, _))
      .WillOnce(DoAll(WithArg<0>(Invoke(finish_request_async)), Return(23)));

  EXPECT_EQ(23, request.GetResponse(base::Bind(success_callback), {}));
}

}  // namespace http
}  // namespace brillo
