// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <brillo/streams/stream_utils.h>

#include <limits>

#include <base/bind.h>
#include <brillo/message_loops/fake_message_loop.h>
#include <brillo/message_loops/message_loop.h>
#include <brillo/streams/mock_stream.h>
#include <brillo/streams/stream_errors.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using testing::DoAll;
using testing::InSequence;
using testing::Return;
using testing::StrictMock;
using testing::_;

ACTION_TEMPLATE(InvokeAsyncCallback,
                HAS_1_TEMPLATE_PARAMS(int, k),
                AND_1_VALUE_PARAMS(size)) {
  brillo::MessageLoop::current()->PostTask(
      FROM_HERE, base::Bind(std::get<k>(args), size));
  return true;
}

ACTION_TEMPLATE(InvokeAsyncCallback,
                HAS_1_TEMPLATE_PARAMS(int, k),
                AND_0_VALUE_PARAMS()) {
  brillo::MessageLoop::current()->PostTask(FROM_HERE, std::get<k>(args));
  return true;
}

ACTION_TEMPLATE(InvokeAsyncErrorCallback,
                HAS_1_TEMPLATE_PARAMS(int, k),
                AND_1_VALUE_PARAMS(code)) {
  brillo::ErrorPtr error;
  brillo::Error::AddTo(&error, FROM_HERE, "test", code, "message");
  brillo::MessageLoop::current()->PostTask(
      FROM_HERE, base::Bind(std::get<k>(args), base::Owned(error.release())));
  return true;
}

namespace brillo {

TEST(StreamUtils, ErrorStreamClosed) {
  ErrorPtr error;
  EXPECT_FALSE(stream_utils::ErrorStreamClosed(FROM_HERE, &error));
  EXPECT_EQ(errors::stream::kDomain, error->GetDomain());
  EXPECT_EQ(errors::stream::kStreamClosed, error->GetCode());
  EXPECT_EQ("Stream is closed", error->GetMessage());
}

TEST(StreamUtils, ErrorOperationNotSupported) {
  ErrorPtr error;
  EXPECT_FALSE(stream_utils::ErrorOperationNotSupported(FROM_HERE, &error));
  EXPECT_EQ(errors::stream::kDomain, error->GetDomain());
  EXPECT_EQ(errors::stream::kOperationNotSupported, error->GetCode());
  EXPECT_EQ("Stream operation not supported", error->GetMessage());
}

TEST(StreamUtils, ErrorReadPastEndOfStream) {
  ErrorPtr error;
  EXPECT_FALSE(stream_utils::ErrorReadPastEndOfStream(FROM_HERE, &error));
  EXPECT_EQ(errors::stream::kDomain, error->GetDomain());
  EXPECT_EQ(errors::stream::kPartialData, error->GetCode());
  EXPECT_EQ("Reading past the end of stream", error->GetMessage());
}

TEST(StreamUtils, CheckInt64Overflow) {
  const int64_t max_int64 = std::numeric_limits<int64_t>::max();
  const uint64_t max_uint64 = std::numeric_limits<uint64_t>::max();
  EXPECT_TRUE(stream_utils::CheckInt64Overflow(FROM_HERE, 0, 0, nullptr));
  EXPECT_TRUE(stream_utils::CheckInt64Overflow(
      FROM_HERE, 0, max_int64, nullptr));
  EXPECT_TRUE(stream_utils::CheckInt64Overflow(
      FROM_HERE, max_int64, 0, nullptr));
  EXPECT_TRUE(stream_utils::CheckInt64Overflow(FROM_HERE, 100, -90, nullptr));
  EXPECT_TRUE(stream_utils::CheckInt64Overflow(
      FROM_HERE, 1000, -1000, nullptr));

  ErrorPtr error;
  EXPECT_FALSE(stream_utils::CheckInt64Overflow(FROM_HERE, 100, -101, &error));
  EXPECT_EQ(errors::stream::kDomain, error->GetDomain());
  EXPECT_EQ(errors::stream::kInvalidParameter, error->GetCode());
  EXPECT_EQ("The stream offset value is out of range", error->GetMessage());

  EXPECT_FALSE(stream_utils::CheckInt64Overflow(
      FROM_HERE, max_int64, 1, nullptr));
  EXPECT_FALSE(stream_utils::CheckInt64Overflow(
      FROM_HERE, max_uint64, 0, nullptr));
  EXPECT_FALSE(stream_utils::CheckInt64Overflow(
      FROM_HERE, max_uint64, max_int64, nullptr));
}

TEST(StreamUtils, CalculateStreamPosition) {
  using Whence = Stream::Whence;
  const uint64_t current_pos = 1234;
  const uint64_t end_pos = 2000;
  uint64_t pos = 0;

  EXPECT_TRUE(stream_utils::CalculateStreamPosition(
      FROM_HERE, 0, Whence::FROM_BEGIN, current_pos, end_pos, &pos, nullptr));
  EXPECT_EQ(0u, pos);

  EXPECT_TRUE(stream_utils::CalculateStreamPosition(
      FROM_HERE, 0, Whence::FROM_CURRENT, current_pos, end_pos, &pos, nullptr));
  EXPECT_EQ(current_pos, pos);

  EXPECT_TRUE(stream_utils::CalculateStreamPosition(
      FROM_HERE, 0, Whence::FROM_END, current_pos, end_pos, &pos, nullptr));
  EXPECT_EQ(end_pos, pos);

  EXPECT_TRUE(stream_utils::CalculateStreamPosition(
      FROM_HERE, 10, Whence::FROM_BEGIN, current_pos, end_pos, &pos, nullptr));
  EXPECT_EQ(10u, pos);

  EXPECT_TRUE(stream_utils::CalculateStreamPosition(
      FROM_HERE, 10, Whence::FROM_CURRENT, current_pos, end_pos, &pos,
      nullptr));
  EXPECT_EQ(current_pos + 10, pos);

  EXPECT_TRUE(stream_utils::CalculateStreamPosition(
      FROM_HERE, 10, Whence::FROM_END, current_pos, end_pos, &pos, nullptr));
  EXPECT_EQ(end_pos + 10, pos);

  EXPECT_TRUE(stream_utils::CalculateStreamPosition(
      FROM_HERE, -10, Whence::FROM_CURRENT, current_pos, end_pos, &pos,
      nullptr));
  EXPECT_EQ(current_pos - 10, pos);

  EXPECT_TRUE(stream_utils::CalculateStreamPosition(
      FROM_HERE, -10, Whence::FROM_END, current_pos, end_pos, &pos, nullptr));
  EXPECT_EQ(end_pos - 10, pos);

  ErrorPtr error;
  EXPECT_FALSE(stream_utils::CalculateStreamPosition(
      FROM_HERE, -1, Whence::FROM_BEGIN, current_pos, end_pos, &pos, &error));
  EXPECT_EQ(errors::stream::kInvalidParameter, error->GetCode());
  EXPECT_EQ("The stream offset value is out of range", error->GetMessage());

  EXPECT_FALSE(stream_utils::CalculateStreamPosition(
      FROM_HERE, -1001, Whence::FROM_CURRENT, 1000, end_pos, &pos, nullptr));

  const uint64_t max_int64 = std::numeric_limits<int64_t>::max();
  EXPECT_FALSE(stream_utils::CalculateStreamPosition(
      FROM_HERE, 1, Whence::FROM_CURRENT, max_int64, end_pos, &pos, nullptr));
}

class CopyStreamDataTest : public testing::Test {
 public:
  void SetUp() override {
    fake_loop_.SetAsCurrent();
    in_stream_.reset(new StrictMock<MockStream>{});
    out_stream_.reset(new StrictMock<MockStream>{});
  }

  FakeMessageLoop fake_loop_{nullptr};
  std::unique_ptr<StrictMock<MockStream>> in_stream_;
  std::unique_ptr<StrictMock<MockStream>> out_stream_;
  bool succeeded_{false};
  bool failed_{false};

  void OnSuccess(uint64_t expected,
                 StreamPtr /* in_stream */,
                 StreamPtr /* out_stream */,
                 uint64_t copied) {
    EXPECT_EQ(expected, copied);
    succeeded_ = true;
  }

  void OnError(const std::string& expected_error,
               StreamPtr /* in_stream */,
               StreamPtr /* out_stream */,
               const Error* error) {
    EXPECT_EQ(expected_error, error->GetCode());
    failed_ = true;
  }

  void ExpectSuccess() {
    EXPECT_TRUE(succeeded_);
    EXPECT_FALSE(failed_);
  }

  void ExpectFailure() {
    EXPECT_FALSE(succeeded_);
    EXPECT_TRUE(failed_);
  }
};

TEST_F(CopyStreamDataTest, CopyAllAtOnce) {
  {
    InSequence seq;
    EXPECT_CALL(*in_stream_, ReadAsync(_, 100, _, _, _))
        .WillOnce(InvokeAsyncCallback<2>(100));
    EXPECT_CALL(*out_stream_, WriteAllAsync(_, 100, _, _, _))
        .WillOnce(InvokeAsyncCallback<2>());
  }
  stream_utils::CopyData(
      std::move(in_stream_), std::move(out_stream_), 100, 4096,
      base::Bind(&CopyStreamDataTest::OnSuccess, base::Unretained(this), 100),
      base::Bind(&CopyStreamDataTest::OnError, base::Unretained(this), ""));
  fake_loop_.Run();
  ExpectSuccess();
}

TEST_F(CopyStreamDataTest, CopyInBlocks) {
  {
    InSequence seq;
    EXPECT_CALL(*in_stream_, ReadAsync(_, 100, _, _, _))
        .WillOnce(InvokeAsyncCallback<2>(60));
    EXPECT_CALL(*out_stream_, WriteAllAsync(_, 60, _, _, _))
        .WillOnce(InvokeAsyncCallback<2>());
    EXPECT_CALL(*in_stream_, ReadAsync(_, 40, _, _, _))
        .WillOnce(InvokeAsyncCallback<2>(40));
    EXPECT_CALL(*out_stream_, WriteAllAsync(_, 40, _, _, _))
        .WillOnce(InvokeAsyncCallback<2>());
  }
  stream_utils::CopyData(
      std::move(in_stream_), std::move(out_stream_), 100, 4096,
      base::Bind(&CopyStreamDataTest::OnSuccess, base::Unretained(this), 100),
      base::Bind(&CopyStreamDataTest::OnError, base::Unretained(this), ""));
  fake_loop_.Run();
  ExpectSuccess();
}

TEST_F(CopyStreamDataTest, CopyTillEndOfStream) {
  {
    InSequence seq;
    EXPECT_CALL(*in_stream_, ReadAsync(_, 100, _, _, _))
        .WillOnce(InvokeAsyncCallback<2>(60));
    EXPECT_CALL(*out_stream_, WriteAllAsync(_, 60, _, _, _))
        .WillOnce(InvokeAsyncCallback<2>());
    EXPECT_CALL(*in_stream_, ReadAsync(_, 40, _, _, _))
        .WillOnce(InvokeAsyncCallback<2>(0));
  }
  stream_utils::CopyData(
      std::move(in_stream_), std::move(out_stream_), 100, 4096,
      base::Bind(&CopyStreamDataTest::OnSuccess, base::Unretained(this), 60),
      base::Bind(&CopyStreamDataTest::OnError, base::Unretained(this), ""));
  fake_loop_.Run();
  ExpectSuccess();
}

TEST_F(CopyStreamDataTest, CopyInSmallBlocks) {
  {
    InSequence seq;
    EXPECT_CALL(*in_stream_, ReadAsync(_, 60, _, _, _))
        .WillOnce(InvokeAsyncCallback<2>(60));
    EXPECT_CALL(*out_stream_, WriteAllAsync(_, 60, _, _, _))
        .WillOnce(InvokeAsyncCallback<2>());
    EXPECT_CALL(*in_stream_, ReadAsync(_, 40, _, _, _))
        .WillOnce(InvokeAsyncCallback<2>(40));
    EXPECT_CALL(*out_stream_, WriteAllAsync(_, 40, _, _, _))
        .WillOnce(InvokeAsyncCallback<2>());
  }
  stream_utils::CopyData(
      std::move(in_stream_), std::move(out_stream_), 100, 60,
      base::Bind(&CopyStreamDataTest::OnSuccess, base::Unretained(this), 100),
      base::Bind(&CopyStreamDataTest::OnError, base::Unretained(this), ""));
  fake_loop_.Run();
  ExpectSuccess();
}

TEST_F(CopyStreamDataTest, ErrorRead) {
  {
    InSequence seq;
    EXPECT_CALL(*in_stream_, ReadAsync(_, 60, _, _, _))
        .WillOnce(InvokeAsyncErrorCallback<3>("read"));
  }
  stream_utils::CopyData(
      std::move(in_stream_), std::move(out_stream_), 100, 60,
      base::Bind(&CopyStreamDataTest::OnSuccess, base::Unretained(this), 0),
      base::Bind(&CopyStreamDataTest::OnError, base::Unretained(this), "read"));
  fake_loop_.Run();
  ExpectFailure();
}

TEST_F(CopyStreamDataTest, ErrorWrite) {
  {
    InSequence seq;
    EXPECT_CALL(*in_stream_, ReadAsync(_, 60, _, _, _))
        .WillOnce(InvokeAsyncCallback<2>(60));
    EXPECT_CALL(*out_stream_, WriteAllAsync(_, 60, _, _, _))
        .WillOnce(InvokeAsyncErrorCallback<3>("write"));
  }
  stream_utils::CopyData(
      std::move(in_stream_), std::move(out_stream_), 100, 60,
      base::Bind(&CopyStreamDataTest::OnSuccess, base::Unretained(this), 0),
      base::Bind(&CopyStreamDataTest::OnError, base::Unretained(this),
                 "write"));
  fake_loop_.Run();
  ExpectFailure();
}

}  // namespace brillo
